/*  This file is part of pas.

	pas is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	pas is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with pas.  If not, see <http://www.gnu.org/licenses/>.
 */

/*  pas is Copyright 2017 by Perry Kivolowitz
 */

#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <cctype>
#include <map>
#include <string>
#include <thread>
#include <mutex>
#include <ctime>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <memory.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <unordered_set>
#include <time.h>
#include <assert.h>
#include <curses.h>

#include "../protos/cpp/commands.pb.h"

using namespace std;
using namespace pas;

#define	LOG(s)		(string(__FILE__) + string(":") + string(__FUNCTION__) + string("() line: ") + to_string(__LINE__) + string(" msg: ") + string(s))

// NOTE:
// NOTE: Assumtion about the number of DACS (for sanity checking of user input.
// NOTE:
const int MAX_DACS = 3;

map<char, int> jump_marks;

int dac_number = 0;
string dac_name;

bool keep_going = true;
bool curses_is_active = false;
int server_socket = -1;

WINDOW * top_window = nullptr;
WINDOW * mid_left = nullptr;
WINDOW * mid_right = nullptr;
WINDOW * bottom_window = nullptr;
WINDOW * instruction_window = nullptr;

struct Track
{
	string id;
	string title;
	string artist;
};

vector<Track> tracks;
int index_of_first_visible_track = 0;
int index_of_high_lighted_line = 0;
int number_of_dacs = 0;
int number_of_lines_of_instructions = 1;
int left_width = 32;
int right_width = 0;
int scroll_height = 0;
int top_window_height = 3;

inline int StartLineForBottomWindow()
{
	return LINES - (2 + number_of_dacs) - number_of_lines_of_instructions;
}

inline int MidWindowHeight()
{
	return LINES - number_of_dacs - 2 - top_window_height - number_of_lines_of_instructions;
}

void SIGHandler(int signal_number)
{
	keep_going = false;
}

/*	InitializeNetworkConnection() - This function provides standard network
	client initialization which does include accepting an alternate port
	and IP address.

	The default port and IP, BTW, are: 5077 and 127.0.0.1.

TODO: Calls to perror print directly to the console. These should
be folded into the LOG().

Parameters:
int argc		Taken from main - the usual meaning
char * argv[]	Taken from main - the usual meaning

Returns:
int				The file descriptor of the server socket

Side Effects:

Expceptions:

Error conditions are thrown as strings.
 */

int InitializeNetworkConnection(int argc, char * argv[])
{
	int server_socket = -1;
	int port = 5077;
	char * ip = (char *) ("127.0.0.1");
	int opt;

	while ((opt = getopt(argc, argv, "h:p:")) != -1) 
	{
		switch (opt) 
		{
			case 'h':
				ip = optarg;
				break;

			case 'p':
				port = atoi(optarg);
				break;
		}
	}

	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket < 0)
	{
		perror("Failed to open socket");
		throw string("");
	}

	hostent * server_hostent = gethostbyname(ip);
	if (server_hostent == nullptr)
	{
		close(server_socket);
		throw  "Failed gethostbyname()";
	}

	sockaddr_in server_sockaddr;
	memset(&server_sockaddr, 0, sizeof(sockaddr_in));
	server_sockaddr.sin_family = AF_INET;
	memmove(&server_sockaddr.sin_addr.s_addr, server_hostent->h_addr, server_hostent->h_length);
	server_sockaddr.sin_port = htons(port);

	if (connect(server_socket, (struct sockaddr*) &server_sockaddr, sizeof(sockaddr_in)) == -1)
	{
		perror("Connection failed");
		throw string("");
	}

	return server_socket;
}

void SendPB(string & s, int server_socket)
{
	assert(server_socket >= 0);

	size_t length = s.size();
	size_t ll = length;
	length = htonl(length);
	size_t bytes_sent = send(server_socket, (const void *) &length, sizeof(length), 0);
	if (bytes_sent != sizeof(length))
		throw string("bad bytes_sent for length");

	bytes_sent = send(server_socket, (const void *) s.data(), ll, 0);
	if (bytes_sent != ll)
		throw string("bad bytes_sent for message");
}

string GetResponse(int server_socket, Type & type)
{
	size_t length = 0;
	size_t bytes_read = recv(server_socket, (void *) &length, sizeof(length), 0);
	if (bytes_read != sizeof(length))
		throw LOG("bad recv getting length: " + to_string(bytes_read));

	string s;
	length = ntohl(length);
	s.resize(length);
	bytes_read = recv(server_socket, (void *) &s[0], length, MSG_WAITALL);
	if (bytes_read != length)
		throw LOG("bad recv getting pbuffer: " + to_string(bytes_read));
	GenericPB g;
	type = GENERIC;
	if (g.ParseFromString(s))
		type = g.type();
	return s;
}

void Sanitize(string & s)
{
	for (auto it = s.begin(); it < s.end(); it++)
	{
		if (*it & 0x80)
			*it = '@';
	}
}

void FetchTracks()
{
	string s;
	SelectQuery c;
	c.set_type(SELECT_QUERY);
	c.set_column(string("artist"));
	c.set_pattern(string("%"));
	bool outcome = c.SerializeToString(&s);
	if (!outcome)
		throw string("bad serialize");
	SendPB(s, server_socket);
	Type type;
	s = GetResponse(server_socket, type);
	// Tracks will come sorted on artist.
	char last_letter = '*';

	if (type == Type::SELECT_RESULT)
	{
		SelectResult sr;
		if (!sr.ParseFromString(s))
		{
			throw string("parsefromstring failed");
		}
		for (int i = 0; i < sr.row_size(); i++)
		{
			Row r = sr.row(i);
			// r.results_size() tells how many are in map.
			google::protobuf::Map<string, string> m = r.results();
			Track t;
			t.id = m["id"];
			t.artist = m["artist"];
			t.title = m["title"];
			if (t.title.size() == 0)
				continue;
			if (t.artist.size() == 0)
				continue;
			if (t.artist[0] != last_letter)
			{
				jump_marks[t.artist[0]] = (int) tracks.size();
				last_letter = t.artist[0];
			}
			Sanitize(t.artist);
			Sanitize(t.title);
			tracks.push_back(t);
		}	
	}
}

void CurrentDACInfo()
{
	werase(top_window);
	wmove(top_window, 1, 2);
	waddstr(top_window, "DAC Number: ");
	string s = to_string(dac_number);
	waddstr(top_window, s.c_str());
	wmove(top_window, 1, 20);
	waddstr(top_window, "Name: ");
	waddstr(top_window, dac_name.c_str());
	wborder(top_window, 0,0,0,0,0,0,0,0);
}

int FindNumberOfDACs()
{
	int rv = -1;
	string s;
	DacInfo a;
	a.set_type(DAC_INFO_COMMAND);
	if (a.SerializeToString(&s))
	{
		SendPB(s, server_socket);
		Type type;
		s = GetResponse(server_socket, type);
		if (type == Type::SELECT_RESULT)
		{
			SelectResult sr;

			if (sr.ParseFromString(s))
				rv = sr.row_size();
		}
	}
	return rv;
}

void DACInfoCommand()
{
	assert(server_socket >= 0);
	string s;
	DacInfo a;
	a.set_type(DAC_INFO_COMMAND);
	bool outcome = a.SerializeToString(&s);
	if (!outcome)
		throw string("DacInfoCommand() bad serialize");

	SendPB(s, server_socket);
	Type type;
	s = GetResponse(server_socket, type);
	werase(bottom_window);
	if (type == Type::SELECT_RESULT)
	{
		SelectResult sr;
		if (!sr.ParseFromString(s))
		{
			throw string("dacinfocomment parsefromstring failed");
		}
		for (int i = 0; i < sr.row_size(); i++)
		{
			Row r = sr.row(i);
			google::protobuf::Map<string, string> results = r.results();

			if (i == dac_number)
				dac_name = results[string("name")];
			wmove(bottom_window, 1 + i, 1);
			waddstr(bottom_window, results[string("index")].c_str());
			wmove(bottom_window, 1 + i, 5);
			string g = results[string("who")];
			if (g.size() > 32)
				g.resize(32);
			waddstr(bottom_window, g.c_str());
			wmove(bottom_window, 1 + i, COLS / 2);
			g = results[string("what")];
			if (g.size() > 40)
				g.resize(40);
			waddstr(bottom_window, g.c_str());
			wmove(bottom_window, 1 + i, COLS - 9);
			waddstr(bottom_window, results[string("when")].c_str());
		}
		wborder(bottom_window, 0, 0, 0, 0, 0, 0, 0, 0);
	}
	else
	{
		throw string("did not get select_result back");
	}
}

void DisplayTracks()
{
	werase(mid_left);
	werase(mid_right);
	for (int i = 0; i < scroll_height - 2; i++)
	{
		int index = i + index_of_first_visible_track;

		if (i + index_of_first_visible_track >= (int) tracks.size())
			index = index % tracks.size();

		if ((int) i == index_of_high_lighted_line)
		{
			wattron(mid_left, A_STANDOUT);
			wattron(mid_right, A_STANDOUT);
		}
		else
		{
			wattroff(mid_left, A_STANDOUT);
			wattroff(mid_right, A_STANDOUT);
		}

		wmove(mid_left, i + 1, 1);
		string s = tracks.at(index).artist;
		if ((int) s.size() > left_width - 2)
			s.resize(left_width -2);
		waddstr(mid_left, s.c_str());

		wmove(mid_right, i + 1, 1);
		s = tracks.at(index).title;
		if ((int) s.size() > right_width - 2)
			s.resize(right_width -2);
		waddstr(mid_right, s.c_str());
	}
	wattroff(mid_left, A_STANDOUT);
	wattroff(mid_right, A_STANDOUT);
	wborder(mid_left, 0,0,0,0,0,0,0,0);
	wborder(mid_right, 0,0,0,0,0,0,0,0);
}

void TrackCount()
{
	wmove(top_window, 1, COLS - COLS / 2);
	waddstr(top_window, "Number of Tracks Available: ");
	waddstr(top_window, to_string(tracks.size()).c_str());
}

void DevCmdNoReply(Type type, int server_socket)
{
	string s;
	ClearDeviceCommand c;
	c.set_type(type);
	c.set_device_id(dac_number);
	bool outcome = c.SerializeToString(&s);
	if (!outcome)
		throw string("bad serialize");
	SendPB(s, server_socket);
}


int main(int argc, char * argv[])
{
	if (signal(SIGINT, SIGHandler) == SIG_ERR)
		throw LOG("");

	if ((server_socket = InitializeNetworkConnection(argc, argv)) < 0)
		return 0;

	cout << "Fetching tracks..." << endl;
	FetchTracks();
	cout << "Done." << endl;
	cout << "Determining number of DACS..." << endl;
	number_of_dacs = FindNumberOfDACs();
	cout << "There is / are " << number_of_dacs << " DAC(s)" << endl;

	initscr();
	right_width = COLS - left_width;
	noecho();
	cbreak();
	scroll_height = MidWindowHeight();
	top_window = newwin(top_window_height, COLS, 0, 0);
	mid_left = newwin(scroll_height, left_width, top_window_height, 0);
	mid_right = newwin(scroll_height, right_width, top_window_height, left_width);
	bottom_window = newwin(2 + number_of_dacs, COLS, StartLineForBottomWindow(), 0);
	instruction_window = newwin(number_of_lines_of_instructions, COLS, LINES - number_of_lines_of_instructions, 0);

	nodelay(top_window, 1);
	keypad(top_window, 1);
	curs_set(0);

	curses_is_active = true;

	string e_string;
	time_t last_update = time(nullptr);

	try
	{
		wborder(top_window, 0, 0, 0, 0, 0, 0, 0, 0);
		wborder(bottom_window, 0, 0, 0, 0, 0, 0, 0, 0);
		wborder(mid_left, 0, 0, 0, 0, 0, 0, 0, 0);
		wborder(mid_right, 0, 0, 0, 0, 0, 0, 0, 0);

		wmove(instruction_window, 0, 0);
		wattron(instruction_window, A_STANDOUT);
		waddstr(instruction_window, "-/D- +/D+ <RET>/Q ^X/STP ^P/PSE ^R/RSM ^N/NXT ^L/CLR A-Z  UP/T- DN/T+ ^F/PG+ ^B/PG- ^C/ESC/EXIT");
		wattroff(instruction_window, A_STANDOUT);
		wmove(top_window, 0, 2);

		int index, track;
		PlayTrackCommand cmd;
		bool outcome;
		string s;
		while (keep_going && curses_is_active)
		{
			bool display_needs_update = false;
			int c = wgetch(top_window);
			if (c != ERR)
			{
				display_needs_update = true;
				switch (c)
				{
					case '+':
						dac_number++;
						if (dac_number >= MAX_DACS)
							dac_number = 0;
						break;

					case '-':
						dac_number--;
						if (dac_number < 0)
							dac_number = MAX_DACS - 1;
						break;

					case KEY_UP:
						index_of_high_lighted_line--;
						if (index_of_high_lighted_line < 0)
						{
							index_of_high_lighted_line = 0;
							index_of_first_visible_track--;
							if (index_of_first_visible_track < 0)
								index_of_first_visible_track = (int) tracks.size() - 1;
						}
						break;

					case KEY_DOWN:
						index_of_high_lighted_line++;
						if (index_of_high_lighted_line >= scroll_height - 3 )
						{
							index_of_high_lighted_line = scroll_height - 3;
							index_of_first_visible_track++;
							if (index_of_first_visible_track >= (int) tracks.size())
								index_of_first_visible_track = 0;
						}
						break;

					case 14:
						// ^N
						DevCmdNoReply(NEXT_DEVICE, server_socket);
						break;

					case 16:
						// ^P
						DevCmdNoReply(PAUSE_DEVICE, server_socket);
						break;

					case 12:
						// ^L 
						DevCmdNoReply(CLEAR_DEVICE, server_socket);
						break;

					case 18:
						// ^R
						DevCmdNoReply(RESUME_DEVICE, server_socket);
						break;

					case 24:
						// ^X - control S wasn't working (probably S/Q).
						DevCmdNoReply(STOP_DEVICE, server_socket);
						break;

					case 10:
					case KEY_ENTER:
						index = index_of_high_lighted_line + index_of_first_visible_track;
						track = atoi(tracks.at(index).id.c_str());
						cmd.set_type(PLAY_TRACK_DEVICE);
						cmd.set_device_id(dac_number);
						cmd.set_track_id(track);
						outcome = cmd.SerializeToString(&s);
						if (!outcome)
							throw string("InnerPlayCommand() bad serialize");
						SendPB(s, server_socket);
						break;
		
					case 6:
						index_of_first_visible_track += (scroll_height - 2);
						index_of_first_visible_track = index_of_first_visible_track % tracks.size();
						break;

					case 2:
						index_of_first_visible_track -= (scroll_height - 2);
						if (index_of_first_visible_track < 0)
							index_of_first_visible_track += tracks.size();
						break;

					case 27:
						keep_going = false;
						break;

					default:
						display_needs_update = false;
						//wmove(top_window, 2, 2);
						//waddstr(top_window, to_string(c).c_str());
						break;
				}
				if (isalnum(c))
				{
					display_needs_update = true;
					if (jump_marks.find((char) c) != jump_marks.end())
						index_of_first_visible_track = jump_marks[(char) c];
				}
			}
			if (!keep_going)
				break;

			// THESE ARE HAPPENING TOO FREQUENTLY.
			if (display_needs_update || difftime(time(nullptr), last_update) > 0.2) {
				DACInfoCommand();
				CurrentDACInfo();
				TrackCount();
				DisplayTracks();
				wmove(top_window, 1, 1);
				wrefresh(instruction_window);
				wrefresh(top_window);
				wrefresh(bottom_window);
				wrefresh(mid_left);
				wrefresh(mid_right);
				last_update = time(nullptr);
			}
			usleep(1000);
		}
	}
	catch (string s)
	{
		e_string = s;
	}

	if (curses_is_active)
	{
		curses_is_active = false;
		endwin();
	}

	if (server_socket >= 0)
		close(server_socket);

	if (e_string.size() > 0)
		cout << e_string << endl;

	return 0;
}

