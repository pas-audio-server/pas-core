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
#include "logger.hpp"

using namespace std;
using namespace pas;

Logger _log_("/tmp/paslog.txt", LogLevel::CONVERSATIONAL);

// These errors are checked for so frequently, lets buffer the strings.
string fts = "failed to serialize";
string ftp = "failed to parse";

bool keep_going = true;
int  port = 5077;

int main(int argc, char * argv[])
{
	string path("/home/perryk/perryk/music");
	AudioComponent ** dacs = nullptr;
	vector<AudioDevice> devices;

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
	
	try
	{
		vector<string> valid_extensions;

		devices.push_back(AudioDevice("alsa_output.usb-AudioQuest_AudioQuest_DragonFly_Black_v1.5_AQDFBL0100111808-01.analog-stereo", "dragonFly Black", (int) devices.size()));
		devices.push_back(AudioDevice("alsa_output.usb-Audioengine_Audioengine_D3_Audioengine-00.analog-stereo", "audioengine D3", (int) devices.size()));
		devices.push_back(AudioDevice("alsa_output.usb-FiiO_DigiHug_USB_Audio-01.analog-stereo", "Fiio", (int) devices.size()));

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
	
		valid_extensions.push_back("mp3");
		valid_extensions.push_back("flac");
		valid_extensions.push_back("wav");
		valid_extensions.push_back("m4a");
		valid_extensions.push_back("ogg");

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
			}
		}	

		cout << "Monitoring network..." << endl;
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
