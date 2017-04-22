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

#include "audio_component.hpp"
#include "audio_device.hpp"
#include "logger.hpp"

using namespace std;
using namespace pas;

extern Logger _log_;

static const int BSIZE = 1024;

vector<AudioDevice> DiscoverDACS()
{
	vector<AudioDevice> devices;

	/*   
    devices.push_back(AudioDevice("alsa_output.pci-0000_00_1f.4.analog-stereo", "Macintosh Built-In", "unused", (int) devices.size()));
    devices.push_back(AudioDevice("alsa_output.usb-AudioQuest_AudioQuest_DragonFly_Black_v1.5_AQDFBL0100111808-01.analog-stereo", "dragonFly Black", "unused", (int) devices.size()));
    devices.push_back(AudioDevice("alsa_output.usb-Audioengine_Audioengine_D3_Audioengine-00.analog-stereo", "audioengine D3", "unused", (int) devices.size()));
    devices.push_back(AudioDevice("alsa_output.usb-FiiO_DigiHug_USB_Audio-01.analog-stereo", "Fiio", "unused", (int) devices.size()));
*/

	FILE* p = nullptr;
	string cmdline = string("pacmd list-sinks | grep name");

	if ((p = popen(cmdline.c_str(), "r")) == nullptr) {
		LOG2(_log_, string("pipe to pacmd failed: ") + string(strerror(errno)), FATAL);
		return devices;
	}

	char buffer[BSIZE] = { 0 };
	string device_name = "device.product.name";

	AudioDevice ad;

	while (fgets(buffer, BSIZE, p) != nullptr) {
		string b(buffer);
		if (b.size() == 0)
			break;

		size_t index;

		if ((index = b.find("device.product.name", 0)) != string::npos) {
			if (b.size() < index + device_name.size() + 4) {
				continue;
			}
			b = b.erase(0, index + device_name.size() + 4);
			index = b.find("\"", 0);
			if (index == string::npos) {
				continue;
			}
			b = b.erase(index, string::npos);
			ad.device_name = b;
			if (ad.device_spec.size() > 0) {
				devices.push_back(ad);
				LOG2(_log_, "found DAC: " + ad.device_name + " " + ad.device_spec, CONVERSATIONAL);
				ad = AudioDevice();
			}
			continue;
		}

		index = b.find("<", 0);
		if (index == string::npos) {
			continue;
		}
		b = b.erase(0, index + 1);

		index = b.find(">", 0);
		b = b.erase(index, string::npos);
		ad.device_spec = b;
		memset(buffer, 0, BSIZE);
	}

	pclose(p);
	return devices;
}