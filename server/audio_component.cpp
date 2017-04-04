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
#include <string>
#include <vector>
#include "audio_component.hpp"

using namespace std;

AudioComponent::AudioComponent()
{
	pas = nullptr;
}

AudioComponent::~AudioComponent()
{
	int pulse_error = 0;
	if (pas != nullptr)
	{
		pa_simple_flush(pas, &pulse_error);
		pa_simple_free(pas);
		pas = nullptr;
	}
}

bool AudioComponent::Initialize(string d)
{
	bool rv = true;
	assert(device.size() == 0);
	device = d;
	return rv;
}
