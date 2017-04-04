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

#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pulse/simple.h>
#include <pulse/error.h>
#include <aio.h>
#include <cstring>
#include <assert.h>

class AudioComponent
{
public:
	AudioComponent();
	~AudioComponent();
	bool Initialize(std::string d);


private:

	std::string device;
	pa_simple * pas;

	inline int BufferNext(int & bi)
	{
		return bi;
	}

	inline int BufferPrev(int & bi)
	{
		return 1 - bi;
	}

	inline void BufferToggle(int & bi)
	{
		bi = 1 - bi;
	}

};
