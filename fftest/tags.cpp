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

// Prototype for tag handling. Not in pas proper.

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

using namespace std;

class Track
{
public:
	map<string, string> tags;
	
	string GetTag(string);
	void SetTag(string, string);
	void SetTag(string);
	void PrintTags(int, int);
};

void Track::SetTag(string raw)
{
	regex c("TAG:.*=.*\n");
	regex t(":.*=");
	regex v("=.*\n");
	
	if (regex_search(raw, c))
	{
		smatch mtag;
		smatch mval;
		string tag, value;
		
		regex_search(raw, mtag, t);
		regex_search(raw, mval, v);
		if (mtag.str(0).size() > 2)
			tag = string(mtag.str(0), 1, mtag.str(0).size() - 2);
		if (mval.str(0).size() > 2)
			value = string(mval.str(0), 1, mval.str(0).size() - 2);
		SetTag(tag, value);	
	}
}

void Track::SetTag(string tag, string value)
{
	transform(tag.begin(), tag.end(), tag.begin(), ::tolower);
	tags[tag] = value;
}

void Track::PrintTags(int width_1, int width_2)
{
	for (auto it = tags.begin(); it != tags.end(); it++)
	{
		cout << left << setw(width_1) << it->first << setw(width_2) << it->second << endl;
	}
}
 
int main(int argc,  char * argv[])
{
	int rv = 0;
	FILE * p = nullptr;
	
	try
	{
		if (argc < 2)
			throw string("argument must be specified.");

		string cmdline = string("ffprobe -loglevel quiet -show_entries stream_tags:format_tags ") + string(argv[1]);
		cout << "Attempting: " << cmdline << endl;
		
		if ((p = popen(cmdline.c_str(), "r")) == nullptr)
			throw string("pipe failed to open.");

		const int bsize = 1024;
		char buffer[bsize];

		Track track;
		while (fgets(buffer, bsize, p) != nullptr)
		{
			track.SetTag(string(buffer));
		}
		track.PrintTags(18, 50);
	}
	catch (string s)
	{
		if (s.size() > 0)
		{
			cerr << s << endl;
			rv = 1;
		}
	}
	
	if (p != nullptr)
		pclose(p);
		
	return rv;
}

