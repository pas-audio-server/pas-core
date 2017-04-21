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
#include <fstream>
#include <iomanip>
#include <cctype>
#include <map>
#include <string>
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
#include "../protos/cpp/commands.pb.h"

using namespace std;
using namespace pas;

// Template function comment

/*	FUNCTIONNAME() - This function

	Parameters:

	Returns:

	Side Effects:

	Expceptions:

*/

#define	LOG(s)		(string(__FILE__) + string(":") + string(__FUNCTION__) + string("() line: ") + to_string(__LINE__) + string(" msg: ") + string(s))

// NOTE:
// NOTE: Assumtion about the number of DACS (for sanity checking of user input.
// NOTE:
const int MAX_DACS = 4;

bool keep_going = true;

void SIGHandler(int signal_number)
{
	keep_going = false;
	cout << endl;
	throw string("");	
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

// Holdover from old version of code. Useful routine to remains here for
// possible future reuse.
inline bool BeginsWith(string const & fullString, string const & prefix)
{
	bool rv = false;

	if (fullString.length() >= prefix.length())
	{
		rv = (0 == fullString.compare(0, prefix.length(), prefix));
	}
	return rv;
}

// Holdover from old version of code. Useful routine to remains here for
// possible future reuse.
bool HasEnding (string const & fullString, string const & ending)
{
	bool rv = false;

	if (fullString.length() >= ending.length())
	{
		rv = (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
	}
	return rv;
}

/*	GetResponse() - This function abstracts the network code and error checking
	when a response is expected from the pas server. Examples of this include
	sending a SELECT_QUERY command and then GetResponse() to receive back the
	rows resulting from the DB query.

	An important consequence of this function is not only to provide the
	Protocol Buffer string but also to taste the buffer to determine its type.
	The type field is required to be the first value in all messages.

	Yes, this has the side effect of having to decode the string twice.

	Parameters:
	int server_socket	The incoming file descriptor for the server network connection
	Type type			The OUTGOING type sniffed from the incoming message

	Returns:
	string				The binary (couched in a string) PB message.

	Side Effects:

	None

	Expceptions:

	Errors are thrown as strings.
*/
string GetResponse(int server_socket, Type & type)
{
	size_t length = 0;
	size_t bytes_read = recv(server_socket, (void *) &length, sizeof(length), 0);
	if (bytes_read != sizeof(length))
		throw LOG("bad recv getting length: " + to_string(bytes_read));
	//cout << LOG("recv of length: " + to_string(length)) << endl;

	length = ntohl(length);
	string s;
	s.resize(length);
	//cout << LOG("string is now: " + to_string(s.size())) << endl;
	bytes_read = recv(server_socket, (void *) &s[0], length, MSG_WAITALL);
	if (bytes_read != length)
		throw LOG("bad recv getting pbuffer: " + to_string(bytes_read));
	GenericPB g;
	// Start out as generic and return this if a) it IS generic or b) does not parse out.
	type = GENERIC;
	if (g.ParseFromString(s))
		type = g.type();
	return s;
}

/*	GetDeviceNumber() - This function abstracts asking the user for a device
	number. This is done so many times, it is an appropriate abstraction.
	Note that the device number is tested against MAX_DACS which contains
	an arbitrary value. A useful change for the future would be to somehow
	determine from actual hardware how many DACs there are.

	Parameters:
	None

	Returns:
	int				The user entered (and vetted) device number.

	Side Effects:
	None

	Expceptions:
	None
*/

int GetDeviceNumber()
{
	string l;
	cout << "Device number: ";
	getline(cin, l);

	int device_id = atoi(l.c_str());
	if (device_id < 0 || device_id >= MAX_DACS)
		device_id = -1;
	return device_id;
}

void SendPB(string & s, int server_socket);

/*	InnerPlayCommand() - This function abstracts the mechanics of sending
	play commands. It is useful because both PLAY and PLAYLIST need
	this function (so it is shared).

	Parameters:
	int device_id		Which DACS to queue up
	int track			The database index of the track to queue
	int server_socket	How to reach the server

	Returns:
	None

	Side Effects:
	A false assertion could crash the program.

	Expceptions:
	SendPB can send exceptions on network issues.
	This function can throw exceptions due to bad Protocol Buffer outcomes.
	These exceptions are in the form of strings.
*/

/*	This is broken out to make the playlist feature easier.
 */
static void InnerPlayCommand(int device_id, int track, int server_socket)
{
	assert(server_socket >= 0);

	string s;
	PlayTrackCommand c;
	c.set_type(PLAY_TRACK_DEVICE);
	c.set_device_id(device_id);
	c.set_track_id(track);
	bool outcome = c.SerializeToString(&s);
	if (!outcome)
		throw string("InnerPlayCommand() bad serialize");
	SendPB(s, server_socket);
}

/*	FUNCTIONNAME() - This function executes as a consequence of the
	user's selection of a PLAY command. It asks for a DAC number and
	track number and uses InnerPlayCommand to actually queue up
	the track. Note the "queue." If the play queue is empty, the
	newly queued track is played immediately. Else it goes to the
	back of the bus.

	Parameters:
	int server_socket	How to reach the server.

	Returns:
	None

	Side Effects:
	A false assertion can crash the program.

	Expceptions:
	InnerPlayCommand can cause string-based exceptions.
*/

static void PlayCommand(int server_socket)
{
	assert(server_socket >= 0);

	int device_id = GetDeviceNumber();
	if (device_id >= 0)
	{
		string s;
		cout << "Track number: ";
		getline(cin, s);
		int track = atoi(s.c_str());
		InnerPlayCommand(device_id, track, server_socket);
	}
}

/*	SimplePlayList() - This function asks for a file name which 
	contains database track numbers. This is a simple playlist
	in that track numbers are transitory. If the database is
	rebuilt, track numbers change. This is for debugging.

	Parameters:
	int server_socket	How to reach the server

	Returns:
	None

	Side Effects:
	A false assertion can crash the program.

	Expceptions:
	Network related errors can cause string-based exception.
*/

static void SimplePlayList(int server_socket)
{
	assert(server_socket >= 0);

	int device_id = GetDeviceNumber();
	if (device_id >= 0)
	{
		string l;
		cout << "File: ";
		getline(cin, l);
		ifstream f(l);
		if (f.is_open())
		{
			while (getline(f, l))
			{
				if (l.size() == 0)
					continue;
				int track = atoi(l.c_str());
				InnerPlayCommand(device_id, track, server_socket);
				usleep(900000);
			}
		}
		f.close();
	}
}

/*	DacInfoCommand() - This function implements the command of the
	same name. It is not abstracted because the return from the pas
	server is an array of rows which must be interpreted in a unique
	way.

	Parameters:
	int server_socket	How to reach the server

	Returns:
	None

	Side Effects:
	None

	Expceptions:
	Both this function and the network functions it calls can 
	throw string-based exceptions.
*/

void DacInfoCommand(int server_socket)
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
	if (type == Type::SELECT_RESULT)
	{
		//cout << LOG("got select result back") << endl;
		SelectResult sr;
		if (!sr.ParseFromString(s))
		{
			throw string("dacinfocomment parsefromstring failed");
		}
		cout << setw(8) << left << "Index";
		cout << setw(24) << "Name";
		cout << setw(24) << "Who";
		cout << setw(24) << "What";
		cout << setw(10) << "When";
		cout << right << endl;
		for (int i = 0; i < sr.row_size(); i++)
		{
			Row r = sr.row(i);
			google::protobuf::Map<string, string> results = r.results();
			cout << setw(8) << left << results[string("index")];
			cout << setw(24) << results[string("name")];
			cout << setw(24) << results[string("who")];
			cout << setw(24) << results[string("what")];
			cout << setw(10) << results[string("when")];
			cout << right << endl;
		}
	}
	else
	{
		throw string("did not get select_result back");
	}
}

/*	TrackCountCommand() - This function SHOULD BE MERGED WITH ArtistCountCommand.
	It sends a zero-arg message and receives back a one integer message. 
	This is exactly the same as ArtistCount.

	Parameters:
	int server_socket	How to reach the server.

	Returns:
	None

	Side Effects:
	A false assertion can crash the program.
	Something is printed.

	Expceptions:
	Both this function and the network functions it calls can 
	throw string-based exceptions.
*/

void TrackCountCommand(int server_socket)
{
	assert(server_socket >= 0);
	string s;
	TrackCountQuery a;
	a.set_type(TRACK_COUNT);
	bool outcome = a.SerializeToString(&s);
	if (!outcome)
		throw string("TrackCountQuery() bad serialize");
	SendPB(s, server_socket);
	Type type;
	s = GetResponse(server_socket, type);
	if (type == Type::ONE_INT)
	{
		OneInteger o;
		if (o.ParseFromString(s))
		{
			cout << o.value() << endl;
		}
		else
		{
			throw string("track_count parsefromstring failed");
		}
	}
	else
	{
		throw string("track count did not get a ONE_INTEGER back");
	}
}

/*	ArtistCountCommand() - This function SHOULD BE MERGED WITH TrackCountCommand.
	It sends a zero-arg message and receives back a one integer message. 
	This is exactly the same as ArtistCount.

	Parameters:
	int server_socket	How to reach the server.

	Returns:
	None

	Side Effects:
	A false assertion can crash the program.
	Something is printed.

	Expceptions:
	Both this function and the network functions it calls can 
	throw string-based exceptions.
*/

void ArtistCountCommand(int server_socket)
{
	assert(server_socket >= 0);
	string s;
	ArtistCountQuery a;
	a.set_type(ARTIST_COUNT);
	bool outcome = a.SerializeToString(&s);
	if (!outcome)
		throw string("ArtistCountQuery() bad serialize");
	SendPB(s, server_socket);
	Type type;
	s = GetResponse(server_socket, type);
	if (type == Type::ONE_INT)
	{
		OneInteger o;
		if (o.ParseFromString(s))
		{
			cout << o.value() << endl;
		}
		else
		{
			throw string("artist_count parsefromstring failed");
		}
	}
	else
	{
		throw string("artist did not get a ONE_INTEGER back");
	}
}

/*	DevCmdForOneString() - This function abstracts several commands
	in the same what the ArtistCount and TrackCount should be. These
	commands, such as Who, What, and When send a DAC number and
	receive back a string.

	Parameters:
	Type type			The precise command to send
	int server_socket	How to reach the server

	Returns:
	None

	Side Effects:
	A false assertion can crash the program.
	Something is printed.

	Expceptions:
	This and network functions can throw string-based exceptions.
*/

void DevCmdForOneString(Type type, int server_socket)
{
	int device_id = GetDeviceNumber();
	if (device_id >= 0)
	{
		string s;
		// Using a WhatCommand container - doesn't matter.
		WhatDeviceCommand sc;
		sc.set_type(type);
		sc.set_device_id(device_id);
		bool outcome = sc.SerializeToString(&s);
		if (!outcome)
			throw string("bad serialize");
		SendPB(s, server_socket);
		Type type;
		s = GetResponse(server_socket, type);
		if (type == Type::ONE_STRING)
		{
			OneString o;
			if (o.ParseFromString(s))
			{
				cout << o.value() << endl;
			}
			else
			{
				throw string("parsefromstring failed");
			}
		}
		else
		{
			throw string("did not get a ONE_STRING back");
		}
	}
}

/*	SelectCommand() - This function initiates a full blowed
	SQL select statement inside pas. The mount of data returned
	could be large (and will be deserialized twice). 

	This function cannot be abstracted (well, in theory it can
	be combined with DacInfo) due to custom parsing of the result
	data.

	Parameters:
	int server_socket		The usual

	Returns:
	None

	Side Effects:
	A false assertion can crash the program.
	A potentially large amount is printed.

	Expceptions:
	Lots of ways this comman can throw a atring-based exception,
*/

void SelectCommand(int server_socket)
{
	assert(server_socket >= 0);

	string s;
	SelectQuery c;
	c.set_type(SELECT_QUERY);
	cout << "Column: ";
	getline(cin, s);
	c.set_column(s);
	cout << "Pattern: ";
	getline(cin , s);
	c.set_pattern(s);
	bool outcome = c.SerializeToString(&s);
	cout << c.DebugString() << endl;
	if (!outcome)
		throw string("bad serialize");
	cout << LOG(to_string(s.size())) << endl;
	SendPB(s, server_socket);
	Type type;
	s = GetResponse(server_socket, type);
	if (type == Type::SELECT_RESULT)
	{
		SelectResult sr;
		if (!sr.ParseFromString(s))
		{
			throw string("parsefromstring failed");
		}
		cout << left << setw(8) << "id" << setw(24) << "artist" << setw(24) << "title" << setw(10) << "duration" << endl;
		for (int i = 0; i < sr.row_size(); i++)
		{
			Row r = sr.row(i);
			// r.results_size() tells how many are in map.
			google::protobuf::Map<string, string> m = r.results();
			cout << left << setw(8) << m["id"];
			cout << setw(30) << m["artist"];
			cout << setw(36) << m["title"];
			cout << setw(10) << m["duration"];
			cout << endl;
/* KEEP THIS AROUND FOR REFERENCE
			google::protobuf::Map<string, string>::iterator it = m.begin();
			while (it != m.end())
			{
				cout << it->first << "		" << it->second << endl;
				it++;
			}
*/
		}	
	}
	else
	{
		throw string("did not get back a SELECT_RESULT");
	}
}

/*	FUNCTIONNAME() - This function

	Parameters:

	Returns:

	Side Effects:

	Expceptions:

*/

void DevCmdNoReply(Type type, int server_socket)
{
	int device_id = GetDeviceNumber();
	if (device_id >= 0)
	{
		string s;
		ClearDeviceCommand c;
		c.set_type(type);
		c.set_device_id(device_id);
		bool outcome = c.SerializeToString(&s);
		if (!outcome)
			throw string("bad serialize");
		SendPB(s, server_socket);
	}
}

/*	FUNCTIONNAME() - This function

	Parameters:

	Returns:

	Side Effects:

	Expceptions:

*/

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

// NOTE:
// NOTE: This code can be used to form the documentation for the supported
// NOTE: pas server commands by what is sent an received. NOT the user 
// NOTE: supplied strings.
// NOTE:

int main(int argc, char * argv[])
{
	GOOGLE_PROTOBUF_VERIFY_VERSION;

	int server_socket = -1;
	bool connected = false;
	string l;

	try
	{
		if (signal(SIGINT, SIGHandler) == SIG_ERR)
			throw LOG("");

		server_socket = InitializeNetworkConnection(argc, argv);
		connected = true;

		while (keep_going)
		{
			cout << "Command: ";
			getline(cin, l);

			if (l == "stop")
				DevCmdNoReply(STOP_DEVICE, server_socket);
			else if (l == "resume")
				DevCmdNoReply(RESUME_DEVICE, server_socket);
			else if (l == "pause")
				DevCmdNoReply(PAUSE_DEVICE, server_socket);
			else if (l == "play")
				PlayCommand(server_socket);
			else if (l == "playlist")
				SimplePlayList(server_socket);
			else if (l == "next")
				DevCmdNoReply(NEXT_DEVICE, server_socket);
			else if (l == "clear")
				DevCmdNoReply(CLEAR_DEVICE, server_socket);
			else if (l == "select")
				SelectCommand(server_socket);
			else if (l == "what")
				DevCmdForOneString(WHAT_DEVICE, server_socket);
			else if (l == "who")
				DevCmdForOneString(WHO_DEVICE, server_socket);
			else if (l == "tracks")
				TrackCountCommand(server_socket);
			else if (l == "artists")
				ArtistCountCommand(server_socket);
			else if (l == "quit")
				break;
			else if (l == "dacs")
				DacInfoCommand(server_socket);
			else if (l == "timecode")
				DevCmdForOneString(WHEN_DEVICE, server_socket);
			else if (l == "rc")
			{
				if (!connected)
				{
					server_socket = InitializeNetworkConnection(argc, argv);
					if (server_socket < 0)
						break;
					connected = true;
					cout << "Connected" << endl;
				}
				continue;
			}
			else if (l == "qs")
			{
				if (!connected)
					continue;
				close(server_socket);
				cout << "Disconnected" << endl;
				connected = false;
				server_socket = -1;
			}
		}
	}
	catch (string s)
	{
		if (s.size() > 0)
			cerr << s << endl;
	}

	if (server_socket >= 0)
		close(server_socket);

	return 0;
}
