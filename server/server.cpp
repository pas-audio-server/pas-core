/*	This file is part of pas.

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

/*	pas is Copyright 2017 by Perry Kivolowitz
*/

#include <iostream>
#include <string>
#include <vector>
#include <stdio.h>
#include <getopt.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <thread>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <arpa/inet.h>

#include "network_component.hpp"
#include "audio_component.hpp"
#include "audio_device.hpp"
#include "discover_dacs.hpp"
#include "logger.hpp"

using namespace std;
using namespace pas;

Logger _log_("/tmp/paslog.txt", LogLevel::CONVERSATIONAL);

// These errors are checked for so frequently, lets buffer the strings.
string fts = "failed to serialize";
string ftp = "failed to parse";
string dbhost = "127.0.0.1";

bool keep_going = true;
int  port = 5077;

void ProcessOptions(int argc, char * argv[])
{
	int c;

	while ((c = getopt(argc, argv, "h:")) != -1) {
		switch (c)
		{
			case 'h':
				dbhost = string(optarg);
				break;

			default:
				break;
		}
	}
}

int main(int argc, char * argv[])
{
	string path("/home/perryk/perryk/music");
	AudioComponent ** dacs = nullptr;
	vector<AudioDevice> devices;

	ProcessOptions(argc, argv);

/*
	// TESTS LOGGING LEVELS
	LOG2(_log_, "FATAL", LogLevel::FATAL);
	LOG2(_log_, "MINIMAL", LogLevel::MINIMAL);
	LOG2(_log_, "CONVERSATIONAL", LogLevel::CONVERSATIONAL);
	LOG2(_log_, "VERBOSE", LogLevel::VERBOSE);
	LOG2(_log_, string("FATAL"), LogLevel::FATAL);
	LOG2(_log_, string("MINIMAL"), LogLevel::MINIMAL);
	LOG2(_log_, string("CONVERSATIONAL"), LogLevel::CONVERSATIONAL);
	LOG2(_log_, string("VERBOSE"), LogLevel::VERBOSE);
*/
	close(0);
	close(1);
	close(2);
	
	try
	{
		vector<string> valid_extensions = { "mp3", "flac", "wav", "m4a" ,"ogg" };

		devices = DiscoverDACS();
		if (devices.size() == 0) {
			throw LoggedException(LOG2(_log_, "no DACS found", FATAL));
		}
		dacs = (AudioComponent **) malloc(devices.size() * sizeof(AudioComponent **));	

		for (size_t i = 0; i < devices.size(); i++)
		{
			LOG2(_log_, devices[i].device_name + " index: " + to_string(devices[i].index), CONVERSATIONAL);
			if ((dacs[i] = new AudioComponent()) == nullptr)
				throw LOG2(_log_, "DAC " + to_string(i) + " failed to allocate", LogLevel::FATAL);
		}

 
		if (argc > 1)
			path = string(argv[1]);

		NetworkComponent nw;
	
		for (size_t i = 0; i < devices.size(); i++)
		{	
			if (!dacs[i]->Initialize(devices[i]))
			{
				LOG2(_log_, "DAC " + to_string((int) i) + " failed to Initialize()", MINIMAL);
				delete dacs[i];
				dacs[i] = nullptr;
			}
			else
			{
				LOG2(_log_, "DAC " + to_string((int) i) + " initialized", MINIMAL);
				LOG2(_log_, dacs[i]->FriendlyName(), CONVERSATIONAL);
			}
		}	

		//cout << "Monitoring network..." << endl;
		nw.AcceptConnections((void *) dacs, (int) devices.size());
	}
	catch (LoggedException s)
	{
		LOG2(_log_, "LoggedException has made it all the up to main().", FATAL);
		LOG2(_log_, "Its type is: " + to_string(s.Level()), FATAL);
		LOG2(_log_, "Its payload is: " + s.Msg(), FATAL);
	}

	for (size_t i = 0; i < devices.size(); i++)
	{
		if (dacs[i] != nullptr)
			delete dacs[i];
	}

	LOG2(_log_, "pas server exiting", FATAL);
	exit(0);
}
