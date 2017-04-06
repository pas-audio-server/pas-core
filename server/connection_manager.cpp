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

/*  pas is Copyright 2017 by Perry Kivolowitz.
*/

#include <iostream>
#include <string>
#include <sstream>
#include "connection_manager.hpp"
#include "audio_component.hpp"
#include "db_component.hpp"

using namespace std;

static DB * InitDB()
{
	DB * db = new DB();

	if (db == nullptr)
		throw LOG("new of DB failed???");

	if (!db->Initialize())
		throw LOG("DB failed to initialize: ");

	return db;
}

static bool CommandProcessor(int socket, char * buffer, void * dacs, int ndacs)
{
	AudioComponent ** acs = (AudioComponent **) dacs;

	bool rv = true;
	// db will become initialized only when receiving commands that require use of the
	// databse. Commands not requiring DB accress will not initialize the DB, leaving
	// the following pointer uninitalized (preventing the DB from being deleted at the
	// end of processing this command.
	DB * db = nullptr;
	
	int device_index = 0;
	int buffer_offset = 0;
	// We only get here if the buffer has something in it (based on bytes_read).
	if (buffer[0] >= '0' && buffer[0] <= '9')
	{
		device_index = buffer[0] - '0';
		buffer_offset = 2;
	}

	try
	{
		if (device_index < 0 || device_index >= ndacs)
		{
			string s("bad device index");
			send(socket, s.c_str(), s.size(), 0);
			throw LOG(s);
		}

		// We are depending upon the memset of buffer to ensure we have a null terminator.
		stringstream tss(buffer + buffer_offset);
		string token;

		tss >> token;

		// This can happen now. Suppose the string is "0 ". The zero and
		// space would be skipped by buffer_offset being 2. Then tss would
		// have nothing in it. 
		if (token.size() == 0)
			throw LOG("");

		if (token == "Q" || token == "c" || token == "r" || token == "z")
		{
			AudioCommand cmd;
			cmd.cmd = (unsigned char) token[0];
			acs[device_index]->AddCommand(cmd);
			if (cmd.cmd == 'Q')
				throw LOG("quitting");
		}
		else if (token == "ti")
		{
			string s;
			s = acs[device_index]->TimeCode();
			if (send(socket, s.c_str(), s.size(), 0) != (ssize_t) s.size())
				throw LOG("send did not return the correct number of bytes written");
		}
		else if (token == "p")
		{
			// The remaining token should be an index number for a track to play
			unsigned int id;
			tss >> id;
			db = InitDB();
			string path = db->PathFromID(id);
			if (path.size() > 0)
			{
				AudioCommand cmd;
				cmd.cmd = PLAY;
				cmd.argument = path;
				acs[device_index]->AddCommand(cmd);
				// and a miracle happens here?
			}
		} 
		else if (token == string("se"))
		{	
			// search on column col using pattern pat
			db = InitDB();
			string col, pat;
			tss >> col >> pat;
			string answer;
			vector<string> aq_results;
			db->MultiValuedQuery(col, pat, aq_results);
			aq_results.push_back("*************");

			for (size_t i = 0; i < aq_results.size(); i++)
			{
				//cout << "sending: " << aq_results.at(i) << endl;
				if (send(socket, aq_results.at(i).c_str(), aq_results.at(i).size(), 0) != (ssize_t) aq_results.at(i).size())
					throw LOG("send did not return the correct number of bytes written");
			}
		}
		else if (token == string("sq"))
		{
			// requested that we quit
			rv = false;
			throw string("");
		}
		else if (token == string("ac"))
		{
			db = InitDB();
			// Get count of artists
			int artist_count = db->GetArtistCount();
			stringstream ss;
			ss << artist_count << endl;
			string s = ss.str();
			if (send(socket, s.c_str(), s.size(), 0) != (ssize_t) s.size())
				throw LOG("send did not return the correct number of bytes written");
		}
		else if (token == string("tc"))
		{
			db = InitDB();
			// Get count of tracks
			int track_count = db->GetTrackCount();
			stringstream ss;
			ss << track_count << endl;
			string s = ss.str();
			if (send(socket, s.c_str(), s.size(), 0) != (ssize_t) s.size())
				throw LOG("send did not return the correct number of bytes written");
		}
	}
	catch (string s)
	{
		if (s.size() > 0)
			cerr << s << endl;
	}
	
	if (db != nullptr)
		delete db;

	return rv;
}

/*	So a connection happened and will be handled by this thread. I am having a
	conceptual problem about who owns what (with respect to the DACs). I need to
	have some device state - isn;t that what I made AudioComponent for? Oh. Right.
	Thats what the dacs parameter gives us.
*/
void ConnectionHandler(sockaddr_in * sockaddr, int socket, void * dacs, int ndacs, int connection_number)
{

	assert(connection_number >= 0);
	assert(socket >= 0);
	assert(sockaddr != nullptr);

	sockaddr_in sa;
	memcpy(&sa, sockaddr, sizeof(sockaddr_in));

	try
	{

		size_t bytes_read;
		const int BS = 2048;
		char buffer[BS];
	
		memset(buffer, 0, BS);
		cout << "ConnectionHandler(" << connection_number << ") servicing client at " << inet_ntoa(sa.sin_addr) << endl;
		while ((bytes_read = recv(socket, (void *) buffer, BS, 0)) > 0)
		{
			//cerr << "Raw: " << buffer << endl;
			if (!CommandProcessor(socket, buffer, dacs, ndacs))
				break;
			memset(buffer, 0, BS);
		}
	}
	catch (string s)
	{
		if (s.size() > 0)
			cerr << s << endl;
	}
	cerr << "ConnectionHandler(" << connection_number << ") exiting!" << endl;
}
