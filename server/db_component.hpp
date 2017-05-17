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

/*	Copyright 2017 by Perry Kivolowitz
*/


#pragma once
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <omp.h>
#include <assert.h>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include "track.hpp"
#include "utility.hpp"
#include "commands.pb.h"


class DB
{
public:
	DB();
	~DB();
	bool Initialize(std::string dbhost);
	void DeInitialize();
	bool Initialized();
	int  GetRoot(std::string nspace = std::string("default"));
	int  GetTrackCount(std::string nspace = std::string("default"));
	int  GetArtistCount(std::string nspace = std::string("default"));
	void MultiValuedQuery(std::string column, 
		std::string pattern, 
		pas::SelectResult & results, 
		std::string nspace = "default", 
		std::string orderby = ""
	);
	void GetFolder(pas::SelectResult & results, int id, std::string nspace = std::string("default"));
	void GetSubfolders(pas::SelectResult & results, int id, std::string nspace = std::string("default"));
	void GetTracks(pas::SelectResult & results, int id, std::string nspace = std::string("default"));
	void FindIDs(std::string column, std::string pattern, std::vector<std::string> & results);
	std::string PathFromID(unsigned int id, std::string * title, std::string * artist, std::string nspace = "default");
	void GetDeviceInfo(std::string alsa_name, std::string & friendly_name);
	
private:
	void InnerGetTracks(pas::SelectResult & results, std::string & sql);
	void InitPreparedStatement();
	sql::Driver * driver;
	sql::Connection * connection;

	bool IsAColumn(std::string c);
	void PrintMySQLException(sql::SQLException & e);
	int IntegerQuery(std::string & sql);
	std::vector<std::string> supported_track_column_names;
	std::string query_columns;
	std::string parameter_columns;
	std::string select_columns;
};

