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
#include "file_system_component.hpp"
#include "audio_component.hpp"
#include "audio_device.hpp"
#include "logger.hpp"

using namespace std;

Logger _log_("/tmp/paslog.txt");

bool keep_going = true;
int  port = 5077;

int main(int argc, char * argv[])
{
	string path("/home/perryk/perryk/music");
	AudioComponent ** dacs = nullptr;
	vector<AudioDevice> devices;

	try
	{
		vector<string> valid_extensions;

		devices.push_back(AudioDevice("alsa_output.usb-AudioQuest_AudioQuest_DragonFly_Black_v1.5_AQDFBL0100111808-01.analog-stereo", "dragonFly Black", (int) devices.size()));
		devices.push_back(AudioDevice("alsa_output.usb-Audioengine_Audioengine_D3_Audioengine-00.analog-stereo", "audioengine D3", (int) devices.size()));
		devices.push_back(AudioDevice("alsa_output.usb-FiiO_DigiHug_USB_Audio-01.analog-stereo", "Fiio", (int) devices.size()));

		dacs = (AudioComponent **) malloc(devices.size() * sizeof(AudioComponent **));	

		for (size_t i = 0; i < devices.size(); i++)
		{
			LOG(_log_, devices[i].device_name + " index: " + to_string(devices[i].index));
			if ((dacs[i] = new AudioComponent()) == nullptr)
				throw LOG(_log_, "DAC " + to_string(i) + " failed to allocate");
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
				LOG(_log_, "DAC " + to_string((int) i) + " failed to Initialize()");
				delete dacs[i];
				dacs[i] = nullptr;
			}
			else
			{
				LOG(_log_, "DAC " + to_string((int) i) + " initialized");
			}
		}	

		cout << "Entering network monitoring" << endl;
		nw.AcceptConnections((void *) dacs, (int) devices.size());
	}
	catch (LoggedException s)
	{
	}

	for (size_t i = 0; i < devices.size(); i++)
	{
		if (dacs[i] != nullptr)
			delete dacs[i];
	}

	exit(0);
}

