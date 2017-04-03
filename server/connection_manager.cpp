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

/*  pas is Copyright 2017 by Perry Kivolowitz.
*/

#include <iostream>
#include <string>
#include <sstream>
#include "connection_manager.hpp"
#include "db_component.hpp"

using namespace std;

void ConnectionHandler(sockaddr_in * sockaddr, int socket, int connection_number, const string & db_path)
{
	assert(connection_number >= 0);
	assert(socket >= 0);
	assert(sockaddr != nullptr);

	sockaddr_in sa;
	DB * db = nullptr;

	memcpy(&sa, sockaddr, sizeof(sockaddr_in));

	try
	{
		db = new DB();
		if (db == nullptr)
			throw LOG("new of DB failed???");
		if (!db->Initialize(db_path))
			throw LOG(string("DB failed to initialize: ") + db_path);

		size_t bytes_read;
		const int BS = 2048;
		char buffer[BS];
	
		memset(buffer, 0, BS);
		cout << "ConnectionHandler(" << connection_number << ") servicing client at " << inet_ntoa(sa.sin_addr) << endl;
		while ((bytes_read = recv(socket, (void *) buffer, BS, 0)) > 0)
		{
			stringstream ss(buffer);
			string token;

			// Identify the command...
			ss >> token;
			if (token.size() == 0)
				throw LOG("recv returned 0 bytes. This shouldn't happen.");

			if (token == string("tc"))
			{
				int track_count = db->GetTrackCount();
				stringstream ss;
				ss << track_count;
				string s = ss.str();
				if (send(socket, s.c_str(), s.size(), 0) != (ssize_t) s.size())
					throw LOG("send did not return the correct number of bytes written");
			}

			if (token == string("sq"))
				break;

			if (write(1, buffer, bytes_read) != (int) bytes_read)
				throw LOG("write to std out returned wrong number of bytes");

			memset(buffer, 0, BS);
		}
		cout << "ConnectionHandler(" << connection_number << ") exiting." << endl;
	}
	catch (string s)
	{
		if (s.size() > 0)
			cerr << s << endl;
	}
	if (db != nullptr)
		delete db;
	cerr << "ConnectionHandler exiting!" << endl;
}
