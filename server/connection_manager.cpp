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

using namespace std;
using namespace pas;

extern Logger _log_;

/*	The following are pre-serialized error messages. The idea is that we don't
	want to "double fault." The error might be that we failed to serialize the
	pb - why try to serialize again for the error message?

	These will be initialized in the network_component at a time and place where
	we're not running multiple threads to avoid races.
*/

extern string unknown_message;
extern string invalid_device;
extern string internal_error;
extern string fts;
extern string ftp;
extern string dbhost;



/*	APPROACH TO EXCEPTIONS / LOGGING

	LoggedExceptions now remember their logging level. If a FATAL is caught, it must be
	rethrown up the chain until it causes the ConnectionHandler() to exit. All others
	can be swallowed at the appropriate place.
*/

static DB * InitDB()
{
	DB * db = new DB();

	if (db == nullptr)
		throw LOG2(_log_, "new of DB failed", FATAL);

	if (!db->Initialize(dbhost))
		throw LOG2(_log_, "DB failed to initialize: ", FATAL);

	return db;
}

static void AddCommandWrapper(AUDIO_COMMANDS c, AudioComponent * ac)
{
	AudioCommand cmd;
	cmd.cmd = c;
	assert(ac != nullptr);
	ac->AddCommand(cmd);
}

static void SendPB(string & s, int server_socket)
{
	assert(server_socket >= 0);

	// NOTE:
	// NOTE: length CANNOT be a size_t as size_t can be 64 or 32 bits depending upon
	// NOTE: the machine. unsigned int is 32 bits. This difference in size can cause
	// NOTE: the received length to leave behind (or send extra) bytes.
	// NOTE:
	unsigned int length = (unsigned int) s.size();
	unsigned int ll = length;
	LOG2(_log_, to_string(length), VERBOSE);

	length = htonl(length);
	size_t bytes_sent = send(server_socket, (const void *) &length, sizeof(length), 0);
	if (bytes_sent != sizeof(length))
		throw LOG2(_log_, "bad bytes_sent for length", FATAL);
	LOG(_log_, to_string(bytes_sent));

	bytes_sent = send(server_socket, (const void *) s.data(), ll, 0);
	if (bytes_sent != ll)
		throw LOG2(_log_, "bad bytes_sent for message", FATAL);
	LOG2(_log_, to_string(bytes_sent), VERBOSE);
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

	try
	{
		GenericPB g;
		g.ParseFromString(s);
		switch (g.type())
		{
			case Type::DAC_INFO_COMMAND:
			{
				LOG2(_log_, "DAC_INFO_COMMAND", VERBOSE);
				SelectResult sr;
				sr.set_type(SELECT_RESULT);
				for (int i = 0; i < ndacs; i++)
				{
					if (acs[i] == nullptr)
						continue;

					Row * r = sr.add_row();
					LOG2(_log_, nullptr, VERBOSE);
					r->set_type(ROW);
					google::protobuf::Map<string, string> * result = r->mutable_results();
					LOG2(_log_, nullptr, VERBOSE);
					(*result)[string("index")] = to_string(i);
					(*result)[string("name")] = acs[i]->HumanName();
					(*result)[string("who")] = acs[i]->Who();
					(*result)[string("what")] = acs[i]->What();
					(*result)[string("when")] = acs[i]->TimeCode();
					LOG2(_log_, nullptr, VERBOSE);
				}
				if (!sr.SerializeToString(&s)) {
					s = internal_error;
					LOG2(_log_, fts, FATAL);
				}
				LOG2(_log_, nullptr, VERBOSE);
				SendPB(s, socket);
			}
			break;

			case Type::TRACK_COUNT:
			{
				// If it should come to pass that there is both a DB error
				// AND a failure to serialize, I'd rather report the internal
				// error.
				// Will change due to namespaces.
				LOG2(_log_, "TRACK_COUNT", CONVERSATIONAL);
				db = InitDB();
				if (db == nullptr) {
					s = internal_error;
					LOG2(_log_, "could not allocate a DB", FATAL);
				}
				else {
					OneInteger r;
					r.set_type(ONE_INT);
					r.set_value(db->GetTrackCount());
					if (!r.SerializeToString(&s)) {
						s = internal_error;
						LOG2(_log_, fts, FATAL);
					}
				}
				SendPB(s, socket);
			}
			break;

			case Type::ARTIST_COUNT:
			{
				// Will change due to namespaces.
				LOG2(_log_, "ARTIST_COUNT", CONVERSATIONAL);
				db = InitDB();
				if (db == nullptr) {
					s = internal_error;
					LOG2(_log_, "could not allocate a DB", FATAL);
				}
				OneInteger r;
				r.set_type(ONE_INT);
				r.set_value(db->GetArtistCount());
				if (!r.SerializeToString(&s)) {
					s = internal_error;
					LOG2(_log_, fts, FATAL);
				}
				SendPB(s, socket);
			}
			break;

			case Type::WHEN_DEVICE:
			case Type::WHO_DEVICE:
			case Type::WHAT_DEVICE:
			{
				LOG2(_log_, "WWW", CONVERSATIONAL);
				// Who, What and When are all the same.
				OneInteger w;
				if (!w.ParseFromString(s)) {
					s = internal_error;
					LOG2(_log_, ftp, FATAL);
				}
				else {
					OneString r;
					r.set_type(ONE_STRING);
					if ((int) w.value() < ndacs && acs[w.value()] != nullptr)
					{
						if (g.type() == Type::WHEN_DEVICE)	
							r.set_value(acs[w.value()]->TimeCode());
						else if (g.type() == Type::WHO_DEVICE)	
							r.set_value(acs[w.value()]->Who());
						else if (g.type() == Type::WHAT_DEVICE)	
							r.set_value(acs[w.value()]->What());
						else {
							s = internal_error;
							LOG2(_log_, "impossible else", FATAL);
						}

						if (!r.SerializeToString(&s)) {
							s = internal_error;
							LOG2(_log_, fts, FATAL);
						}
					}
					else {
						// As these messages require a response we must provide
						// one in the event of error.
						s = invalid_device;
					}
				}
				SendPB(s, socket);
			}
			break;

			case Type::CLEAR_DEVICE:
			{
				LOG2(_log_, "CLEAR DEVICE", CONVERSATIONAL);
				OneInteger o;
				if (!o.ParseFromString(s)) {
					LOG2(_log_, ftp, FATAL);
				}
				else if ((int) o.value() < ndacs && acs[o.value()] != nullptr) {
					acs[o.value()]->ClearQueue();
				}
				// This message does not respond. Therefore errors can be eaten.
			}
			break;

			case Type::APPEND_QUEUE:
			{
				// Will change due to namespaces.
				// Appends a copy of B's queue onto A.
				// Not in use yet.
				LOG2(_log_, "APPEND QUEUE", CONVERSATIONAL);
				TwoIntegers o;
				if (!o.ParseFromString(s)) {
					LOG2(_log_, ftp, FATAL);
				}
				else if ((int) o.value_a() < ndacs && acs[o.value_a()] != nullptr &&
						 (int) o.value_b() < ndacs && acs[o.value_b()] != nullptr)
				{
					acs[o.value_b()]->AppendQueue(acs[o.value_a()]);
				}
				// This message does not respond. Therefore errors can be eaten.
			}
			break;

			case Type::NEXT_DEVICE:
			{
				LOG2(_log_, "NEXT_DEVICE", CONVERSATIONAL);
				OneInteger c;
				if (!c.ParseFromString(s)) {
					LOG2(_log_, ftp, FATAL);
				}
				else if ((int) c.value() < ndacs && acs[c.value()] != nullptr) {
					AddCommandWrapper(NEXT, acs[c.value()]);
				}
				// This message does not respond. Therefore errors can be eaten.
			}
			break;

			case Type::STOP_DEVICE:
			{
				LOG2(_log_, "STOP_DEVICE", CONVERSATIONAL);
				OneInteger c;
				if (!c.ParseFromString(s)) {
					LOG2(_log_, ftp, FATAL);
				}
				else if ((int) c.value() < ndacs && acs[c.value()] != nullptr) {
					AddCommandWrapper(STOP, acs[c.value()]);
				}
				// This message does not respond. Therefore errors can be eaten.
			}
			break;

			case Type::RESUME_DEVICE:
			{
				LOG2(_log_, "RESUME_DEVICE", CONVERSATIONAL);
				OneInteger c;
				if (!c.ParseFromString(s)) {
					LOG2(_log_, ftp, FATAL);
				}
				else if ((int) c.value() < ndacs && acs[c.value()] != nullptr) {
					AddCommandWrapper(RESUME, acs[c.value()]);
				}
				// This message does not respond. Therefore errors can be eaten.
			}
			break;

			case Type::PAUSE_DEVICE:
			{
				LOG2(_log_, "PAUSE_DEVICE", CONVERSATIONAL);
				OneInteger c;
				if (!c.ParseFromString(s)) {
					LOG2(_log_, ftp, FATAL);
				}
				else if ((int) c.value() < ndacs && acs[c.value()] != nullptr) {
					AddCommandWrapper(PAUSE, acs[c.value()]);
				}
				// This message does not respond. Therefore errors can be eaten.
			}
			break;

			case Type::PLAY_TRACK_DEVICE:
			{
				// Will change due to namespaces.					
				LOG2(_log_, "PLAY_TRACK_DEVICE", CONVERSATIONAL);
				PlayTrackCommand c;
				if (!c.ParseFromString(s)) {
					LOG2(_log_, ftp, FATAL);
				}
				else if ((int) c.device_id() < ndacs && acs[c.device_id()] != nullptr) {
					acs[c.device_id()]->Play(c.track_id());
				}
				// This message does not respond. Therefore errors can be eaten.
			}
			break;

			case SELECT_QUERY_EXPANDED:
			{
				LOG2(_log_, "SELECT_QUERY_EXPANDED", CONVERSATIONAL);
				SelectQueryExpanded c;
				if (!c.ParseFromString(s)) {
					s = internal_error;
					LOG2(_log_, ftp, FATAL);
				}
				else {
					db = InitDB();
					if (db == nullptr) {
						s = internal_error;
						LOG2(_log_, "db failed to initialize", FATAL);
					}
					else {
						SelectResult r;
						r.set_type(SELECT_RESULT);
						db->MultiValuedQuery(c.column(), c.pattern(), r, c.nspace(), c.orderby());
						if (!r.SerializeToString(&s)) {
							s = internal_error;
							LOG2(_log_, fts, FATAL);
						}
					}
				}
				SendPB(s, socket);
			}
			break;

			case SELECT_QUERY:
			{
				// If namespace support is needed, use SELECT_QUERY_EXPANDED.
				LOG2(_log_, "SELECT_QUERY", CONVERSATIONAL);
				SelectQuery c;
				if (!c.ParseFromString(s)) {
					s = internal_error;
					LOG2(_log_, ftp, FATAL);
				}
				else {
					db = InitDB();
					if (db == nullptr) {
						s = internal_error;
						LOG2(_log_, "db failed to initialize", FATAL);
					}
					else {
						SelectResult r;
						r.set_type(SELECT_RESULT);
						db->MultiValuedQuery(c.column(), c.pattern(), r);
						if (!r.SerializeToString(&s)) {
							s = internal_error;
							LOG2(_log_, fts, FATAL);
						}
					}
				}
				SendPB(s, socket);
			}
			break;

			default:
			{
				SendPB(unknown_message, socket);
				LOG2(_log_, "switch received type: " + to_string((int) g.type()), CONVERSATIONAL);
			}
			break;
		}
	}
	catch (LoggedException s)
	{
		if (s.Level() == FATAL) {
			throw s;
		}
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
	
		LOG2(_log_, "ConnectionHandler(" + to_string(connection_number) + ") servicing client", MINIMAL);
		while (true)
		{
			// NOTE:
			// NOTE: length CANNOT be a size_t as size_t can be 64 or 32 bits depending upon
			// NOTE: the machine. unsigned int is 32 bits. This difference in size can cause
			// NOTE: the received length to leave behind (or send extra) bytes.
			// NOTE:

			unsigned int length;
			bytes_read = recv(socket, (void *) &length, sizeof(length), 0);
			if (bytes_read != sizeof(length))
				throw LOG2(_log_, "bad recv getting length: " + to_string(bytes_read), FATAL);
			LOG2(_log_, "recv of length: " + to_string(length), VERBOSE);
			
			string s;
			length = ntohl(length);
			s.resize(length);
			
			bytes_read = recv(socket, (void *) &s[0], length, 0);
			if (bytes_read != length)
				throw LOG2(_log_, "bad recv getting pbuffer: " + to_string(bytes_read), LogLevel::FATAL);
			if (!CommandProcessor(socket, s, dacs, ndacs))
				break;
		}
	}
	catch (LoggedException s)
	{
		// We've got an unhandled exception from lower down. In a perfect world
		// this would include only FATAL exceptions but this feature is being added
		// after the fact. Therefore we better check for mistakes here and scream
		// about them!
		if (s.Level() != LogLevel::FATAL) {
			LOG2(_log_, "ERROR", LogLevel::FATAL);
			LOG2(_log_, "ERROR - NON FATAL EXCEPTION CAUGHT BY CONNECTIONHANDLER()", LogLevel::FATAL);
			LOG2(_log_, "ERROR", LogLevel::FATAL);
		}
	}
	LOG2(_log_, "ConnectionHandler(" + to_string(connection_number) + ") exiting!", MINIMAL);
}
