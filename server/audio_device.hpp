#pragma once
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

struct AudioDevice
{
	AudioDevice() {}

	AudioDevice(std::string s, std::string n, int i) : index(i), device_spec(s), device_name(n) {}

	int index;
    // device_spec is the "official" hardware name - how the DAC is identified to alsa.
	std::string device_spec;
    // device_name is some mfr supplied string.
	std::string device_name;
};

