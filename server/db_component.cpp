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

#include "db_component.hpp"

using namespace std;

//const int DB::_db_sleep_time = 1000;

string track_column_names[] =
{
	// Order must match select / insert code
	"path",
	"artist",
	"title",
	"album",
	"genre",
	"source",
	"duration",
	"publisher",
	"composer",
	"track",
	"copyright",
	"disc"
};

DB::DB()
{
	driver = nullptr;
	connection = nullptr;
	// I did not want to make this a static on purpose to avoid race conditions on init.
	query_columns = "(";
	parameter_columns = " values (";
	for (size_t i = 0; i < sizeof(track_column_names) / sizeof(string); i++)
	{
		if (i > 0)
		{
			query_columns += ", ";
			parameter_columns += ", ";
		}
		supported_track_column_names.push_back(track_column_names[i]);
		query_columns += track_column_names[i];
		parameter_columns += "?";
	}
	query_columns += ") ";
	parameter_columns += ") ";
}

DB::~DB()
{
	if (connection != nullptr)
	{
		connection->close();
		delete connection;
	}
}

bool DB::Initialize()
{
	bool rv = true;

	driver = get_driver_instance();
	if (driver == nullptr)
	{
		cerr << LOG("get_driver_instance() failed") << endl;
		rv = false;
	}
	else
	{
		connection = driver->connect("tcp://127.0.0.1:3306", "pas", "pas");
		if (connection == nullptr)
		{
			cerr << LOG("connect() failed") << endl;
			rv = false;
		}
		else
		{
			connection->setSchema("pas");
		}
	}
	return rv;
}

void DB::DeInitialize()
{
	if (connection != nullptr)
	{
		connection->close();
		delete connection;
		connection = nullptr;
	}
}

bool DB::AddMedia(std::string & path, bool force)
{
	bool rv = true;
	FILE * p = nullptr;
	sql::PreparedStatement * stmt = nullptr;

	assert(Initialized());
	try
	{
		if (access(path.c_str(), R_OK) < 0)
			throw string("cannot access: ") + path;

		// adding -show_entries format=duration -sexagesimal
		// will get the duration but it isn't in the same format.
		// instead it looks like this:
		// duration=0:04:19.213061
		string cmdline = string("ffprobe -loglevel quiet -show_entries stream_tags:format_tags -show_entries format=duration -sexagesimal \"") + path + string("\"");

		if ((p = popen(cmdline.c_str(), "r")) == nullptr)
			throw LOG(path);

		const int bsize = 1024;
		char buffer[bsize];

		Track track;
		track.SetPath(path);

		while (fgets(buffer, bsize, p) != nullptr)
		{
			string b(buffer);
			track.SetTag(b);
		}

		// NOTE:
		// NOTE: It is a map of tags to values. As more columns are added, change track_column_names.
		// NOTE:

		string sql = string("insert into tracks ") + query_columns + parameter_columns;
		stmt = connection->prepareStatement(sql.c_str());
		if (stmt == nullptr)
			throw LOG("prepareStatement() failed");

		int i = 1;
		for (auto it = supported_track_column_names.begin(); it < supported_track_column_names.end(); it++, i++)
		{
			string v = track.GetTag(*it);
			//cout << *it << "	" << v << endl;
			stmt->setString(i, v.c_str());
		}

		stmt->execute();
	}
	catch (string s)
	{
		if (s.size() > 0)
		{
			cerr << s << endl;
			rv = false;
		}
	}
	catch (sql::SQLException &e)
	{
		PrintMySQLException(e);
	}

	if (stmt != nullptr)
		delete stmt;

	if (p != nullptr)
		pclose(p);

	return rv;
}

int DB::IntegerQuery(string & sql)
{
	int rv = 0;

	sql::Statement * stmt = nullptr;
	sql::ResultSet *res = nullptr;

	assert(connection != nullptr);

	stmt = connection->createStatement();
	if (stmt == nullptr)
	{
		cerr << LOG("createStatement() failed") << endl;
	}
	else
	{
		try
		{
			res = stmt->executeQuery(sql.c_str());
			if (res->next())
			{
				rv = res->getInt(1);
			}
		}
		catch (sql::SQLException &e)
		{
			PrintMySQLException(e);
		}
	}
	if (stmt != nullptr)
		delete stmt;

	if (res != nullptr)
		delete res;

	return rv;
}

int DB::GetTrackCount()
{
	string sql("select count(*) from tracks");
	return IntegerQuery(sql);
}

void DB::PrintMySQLException(sql::SQLException & e)
{
	cerr << "# ERR: SQLException in " << __FILE__;
	cerr << " on line " << __LINE__ << endl;
	cerr << "# ERR: " << e.what();
	cerr << " (MySQL error code: " << e.getErrorCode();
	cerr << ", SQLState: " << e.getSQLState() << " )" << endl;
}

void DB::MultiValuedQuery(string column, string pattern, vector<string> & results)
{
	sql::Statement * stmt = nullptr;
	sql::ResultSet * res = nullptr;

	assert(connection != nullptr);
	if (IsAColumn(column))
	{
		try
		{
			// NOTE:
			// NOTE: SQL Injection vulnerability here.
			// NOTE:
			if ((stmt = connection->createStatement()) == nullptr)
				throw LOG("createStatement() failed");

			string sql("select id,artist,title,album,genre from tracks where ");
			sql += column + " like \"" + pattern + "\" order by " + column + ";";
			res = stmt->executeQuery(sql.c_str());
			while (res->next())
			{
				stringstream ss;
				ss << res->getString(1) << "\t";
				ss << res->getString(2) << "\t";
				ss << res->getString(3) << "\t";
				ss << res->getString(4) << "\t";
				ss << res->getString(5) << endl;
				results.push_back(ss.str());
			}
		}
		catch (string s)
		{
			if (s.size() > 0)
				cerr << s << endl;
		}
		catch (sql::SQLException &e)
		{
			PrintMySQLException(e);
		}
	}

	if (stmt != nullptr)
		delete stmt;

	if (res != nullptr)
		delete res;
}

bool DB::IsAColumn(std::string c)
{
	bool rv = false;
	for (size_t i = 0; i < sizeof(track_column_names) / sizeof(string); i++)
	{
		if (c == track_column_names[i])
		{
			rv = true;
			break;
		}
	}
	return rv;
}

int DB::GetArtistCount()
{
	string sql("select count(*) from (select distinct artist from tracks) as foo;");
	return IntegerQuery(sql);
}

bool DB::Initialized()
{
	return connection != nullptr;
}

string DB::PathFromID(unsigned int id)
{
	string rv;
	assert(Initialized());
	string sql("select path from tracks where id = " + to_string(id) + ";");
	sql::Statement * stmt = connection->createStatement();
	sql::ResultSet *res = nullptr;

	try
	{
		if (stmt == nullptr)
			throw LOG("createStatement() failed");

		res = stmt->executeQuery(sql.c_str());
		if (res->next())
		{
			rv = res->getString(1);
		}
	}
	catch (string s)
	{
		if (s.size() > 0)
		{
			cerr << s << endl;
		}
	}
	catch (sql::SQLException &e)
	{
		PrintMySQLException(e);
	}
	return rv;
}

