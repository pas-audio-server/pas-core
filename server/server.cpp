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

using namespace std;

bool keep_going = true;
int  port = 5077;

int main(int argc, char * argv[])
{
	string db_path("/home/perryk/pas/pas.db");
	string path("/home/perryk/perryk/music");

	if (argc > 1)
		path = string(argv[1]);

	vector<string> valid_extensions;
	NetworkComponent nw;

	valid_extensions.push_back("mp3");
	valid_extensions.push_back("flac");
	valid_extensions.push_back("wav");
	valid_extensions.push_back("m4a");
	valid_extensions.push_back("ogg");

	//Enumerate2(path, valid_extensions, false);

	// Connect to database.
	// Initialize audio.
	nw.AcceptConnections(db_path);
	exit(0);
}

