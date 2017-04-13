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
#include "logger.hpp"
#include "../protos/cpp/commands.pb.h"

using namespace std;
using namespace pas;

extern Logger _log_;

static DB * InitDB()
{
	DB * db = new DB();

	if (db == nullptr)
		throw LOG(_log_, "new of DB failed???");

	if (!db->Initialize())
		throw LOG(_log_, "DB failed to initialize: ");

	return db;
}

// d and n supplied only for sanity checking
static void AddCommandWrapper(AUDIO_COMMANDS c, AudioComponent * ac, int d, int n)
{
	AudioCommand cmd;
	cmd.cmd = c;
	if (d >= 0 && d < n)
	{
		ac->AddCommand(cmd);
	}
	else
	{	
		LOG(_log_, "bad device number");
	}
}

static void SendPB(string & s, int server_socket)
{
	assert(server_socket >= 0);

	size_t length = s.size();
	LOG(_log_, to_string(length));
	size_t bytes_sent = send(server_socket, (const void *) &length, sizeof(length), 0);
	if (bytes_sent != sizeof(length))
		throw string("bad bytes_sent for length");
	LOG(_log_, to_string(bytes_sent));

	bytes_sent = send(server_socket, (const void *) s.data(), length, 0);
	if (bytes_sent != length)
		throw string("bad bytes_sent for message");
	LOG(_log_, to_string(bytes_sent));
}

static bool CommandProcessor(int socket, string & s, void * dacs, int ndacs)
{
	AudioComponent ** acs = (AudioComponent **) dacs;

	bool rv = true;
	// db will become initialized only when receiving commands that require use of the
	// databse. Commands not requiring DB accress will not initialize the DB, leaving
	// the following pointer uninitalized (preventing the DB from being deleted at the
	// end of processing this command.
	DB * db = nullptr;

/*
		LOG(_log_, nullptr);
					google::protobuf::Map<string, string> * result = r.mutable_results();
		LOG(_log_, nullptr);
		// LEFT OFF HERE
					(*result)[string("happy")] = string("joy");
		LOG(_log_, nullptr);
					string s;
					bool outcome = r.SerializeToString(&s);
					if (!outcome)
						throw string("WHO_DEVICE bad serialize");
		LOG(_log_, nullptr);
*/
	
	
	try
	{
		GenericPB g;
		//LOG(_log_, nullptr);
		g.ParseFromString(s);
		//LOG(_log_, nullptr);
		switch (g.type())
		{
			case Type::DAC_INFO_COMMAND:
			{
				SelectResult sr;
				sr.set_type(SELECT_RESULT);
				//LOG(_log_, nullptr);
				for (int i = 0; i < ndacs; i++)
				{
					Row * r = sr.add_row();
					//LOG(_log_, nullptr);
					r->set_type(ROW);
					google::protobuf::Map<string, string> * result = r->mutable_results();
					//LOG(_log_, nullptr);
					(*result)[string("index")] = to_string(i);
					(*result)[string("name")] = acs[i]->HumanName();
					(*result)[string("who")] = acs[i]->Who();
					(*result)[string("what")] = acs[i]->What();
					(*result)[string("when")] = acs[i]->TimeCode();
					//LOG(_log_, nullptr);
				}
				if (!sr.SerializeToString(&s))
						throw LOG(_log_, "t_c could not serialize");
				//LOG(_log_, nullptr);
				SendPB(s, socket);
			}
			break;

			case Type::TRACK_COUNT:
				{
					db = InitDB();
					if (db == nullptr)
						throw LOG(_log_, "could not allocate a DB");
					OneInteger r;
					r.set_type(ONE_INT);
					r.set_value(db->GetTrackCount());
					if (!r.SerializeToString(&s))
						throw LOG(_log_, "t_c could not serialize");
					SendPB(s, socket);
				}
				break;

			case Type::ARTIST_COUNT:
				{
					db = InitDB();
					if (db == nullptr)
						throw LOG(_log_, "could not allocate a DB");
					OneInteger r;
					r.set_type(ONE_INT);
					r.set_value(db->GetArtistCount());
					if (!r.SerializeToString(&s))
						throw LOG(_log_, "a_c could not serialize");
					SendPB(s, socket);
				}
				break;

			case Type::WHEN_DEVICE:
			case Type::WHO_DEVICE:
			case Type::WHAT_DEVICE:
				{
					//LOG(_log_, nullptr);
					// Who, What and When are all the same.
					WhenDeviceCommand w;
					if (!w.ParseFromString(s))
						throw LOG(_log_, "what failed to parse");

					OneString r;
					r.set_type(ONE_STRING);
					if ((int) w.device_id() < ndacs)
					{
						if (g.type() == Type::WHEN_DEVICE)	
							r.set_value(acs[w.device_id()]->TimeCode());
						else if (g.type() == Type::WHO_DEVICE)	
							r.set_value(acs[w.device_id()]->Who());
						else if (g.type() == Type::WHAT_DEVICE)	
							r.set_value(acs[w.device_id()]->What());
						else
							throw LOG(_log_, "impossible else");
					}
					if (!r.SerializeToString(&s))
					{
						throw LOG(_log_, "failed to serialize");
					}
					SendPB(s, socket);
				}
				break;

			case Type::NEXT_DEVICE:
				{
					LOG(_log_, "NEXT_DEVICE");
					// Doesn't matter which command - clean this up someday.
					StopDeviceCommand c;
					if (!c.ParseFromString(s))
						throw LOG(_log_, "next failed to parse");
					AddCommandWrapper(NEXT, acs[c.device_id()], c.device_id(), ndacs);
				}
				break;

			case Type::STOP_DEVICE:
				{
					LOG(_log_, "STOP_DEVICE");
					StopDeviceCommand c;
					if (!c.ParseFromString(s))
						throw LOG(_log_, "stop failed to parse");
					AddCommandWrapper(STOP, acs[c.device_id()], c.device_id(), ndacs);
				}
				break;

			case Type::RESUME_DEVICE:
				{
					LOG(_log_, "RESUME_DEVICE");
					ResumeDeviceCommand c;
					if (!c.ParseFromString(s))
						throw LOG(_log_, "resume failed to parse");
					AddCommandWrapper(RESUME, acs[c.device_id()], c.device_id(), ndacs);
				}
				break;

			case Type::PAUSE_DEVICE:
				{
					LOG(_log_, "PAUSE_DEVICE");
					PauseDeviceCommand c;
					if (!c.ParseFromString(s))
						throw LOG(_log_, "pause failed to parse");
					AddCommandWrapper(PAUSE, acs[c.device_id()], c.device_id(), ndacs);
				}
				break;

			case Type::PLAY_TRACK_DEVICE:
				{
					LOG(_log_, "PLAY_TRACK_DEVICE");
					PlayTrackCommand c;
					if (!c.ParseFromString(s))
						throw LOG(_log_, "play failed to parse");
					if ((int) c.device_id() < ndacs)
					{
						acs[c.device_id()]->Play(c.track_id());
					}
					else
					{
						LOG(_log_, "PLAY_TRACK_DEVICE: bad device number: " + to_string(c.device_id()));
					}
				}
				break;

			case Type::SELECT_QUERY:
				{
					LOG(_log_, "SELECT_QUERY");
					SelectQuery c;
					if (!c.ParseFromString(s))
						throw LOG(_log_, "select failed to parse");
					db = InitDB();
					if (db == nullptr)
						throw LOG(_log_, "db failed to initialize");
					SelectResult r;
					r.set_type(SELECT_RESULT);
					db->MultiValuedQuery(c.column(), c.pattern(), r);
					if (!r.SerializeToString(&s))
					{
						throw LOG(_log_, "failed to serialize");
					}
					LOG(_log_, to_string(s.size()));
					SendPB(s, socket);
				}
				break;

			default:
				LOG(_log_, "switch received type: " + to_string((int) g.type()));
				break;
		}
/*
		else if (token == string("se"))
		{	
			// search on column col using pattern pat
			db = InitDB();
			string col, pat;
			tss >> col >> pat;
			string aq_results;
			db->MultiValuedQuery(col, pat, aq_results);
			//cout << __FUNCTION__ << " " << __LINE__ << " " << col << " " << pat << "\t" << aq_results << endl;
			size_t t = aq_results.size();
			if (send(socket, &t, sizeof(size_t), 0) != sizeof(size_t))
				throw LOG(_log_, "bad send");
			if (send(socket, aq_results.c_str(), aq_results.size(), 0) != (ssize_t) aq_results.size())
				throw LOG(_log_, "send did not return the correct number of bytes written");
		}
*/
	}
	catch (LoggedException s)
	{
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
	
		LOG(_log_, "ConnectionHandler(" + to_string(connection_number) + ") servicing client");
		while (true)
		{
			size_t length = 0;
			bytes_read = recv(socket, (void *) &length, sizeof(length), 0);
			if (bytes_read != sizeof(length))
				throw LOG(_log_, "bad recv getting length: " + to_string(bytes_read));
			LOG(_log_, "recv of length: " + to_string(length));
			string s;
			s.resize(length);
			bytes_read = recv(socket, (void *) &s[0], length, 0);
			if (bytes_read != length)
				throw LOG(_log_, "bad recv getting pbuffer: " + to_string(bytes_read));
			if (!CommandProcessor(socket, s, dacs, ndacs))
				break;
		}
	}
	catch (LoggedException s)
	{
	}
	LOG(_log_, "ConnectionHandler(" + to_string(connection_number) + ") exiting!");
}
