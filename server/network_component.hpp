#pragma once

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

/*  pas is Copyright 2017 by Perry Kivolowitz.
*/
 
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <arpa/inet.h>

class NetworkComponent
{
public:
	NetworkComponent();
	void AcceptConnections(const std::string & db_path);

private:
	static bool keep_going;
	static void SIGINTHandler(int);
	int port;
	const int max_connections = 4;
	int listening_socket;
	std::vector<std::thread *> threads;
};

