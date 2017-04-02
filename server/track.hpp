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

/*	Copyright 2017 by Perr Kivolowitz
*/

#include <iostream>
#include <map>
#include <algorithm>
#include <iomanip>
#include <regex>
#include <string>
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include <assert.h>

class Track
{
public:
	std::map<std::string, std::string> tags;

	std::string GetTag(std::string &);
	void SetTag(std::string & tag, std::string & value);
	void SetTag(std::string & raw);
	void SetPath(std::string & path);
	void PrintTags(int, int);
};

