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
#include "logger.hpp"

using namespace std;
using namespace pas;

extern Logger _log_;

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
	query_columns_no_path = "id, ";
	size_t n = sizeof(track_column_names) / sizeof(string);
	for (size_t i = 0; i < n; i++)
	{
		if (i > 0)
		{
			query_columns += ", ";
			parameter_columns += ", ";
			query_columns_no_path += track_column_names[i];
			if (i >= 1 && i < n - 1)
			{
				query_columns_no_path += ", ";
			}
		}
		supported_track_column_names.push_back(track_column_names[i]);
		query_columns += track_column_names[i];
		parameter_columns += "?";
	}
	query_columns += ") ";
	parameter_columns += ") ";
	//cout << query_columns_no_path << endl;
	/*
	cout << query_columns << endl;
	cout << parameter_columns << endl;
	*/
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
		LOG(_log_, "get_driver_instance() failed");
		rv = false;
	}
	else
	{
		connection = driver->connect("tcp://127.0.0.1:3306", "pas", "pas");
		if (connection == nullptr)
		{
			LOG(_log_, "connect() failed");
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
			throw LOG(_log_, path);

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
			throw LOG(_log_, "prepareStatement() failed");

		int i = 1;
		for (auto it = supported_track_column_names.begin(); it < supported_track_column_names.end(); it++, i++)
		{
			string v = track.GetTag(*it);
			stmt->setString(i, v.c_str());
		}

		stmt->execute();
	}
	catch (LoggedException s)
	{
		rv = false;
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
		LOG(_log_, "createStatement() failed");
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

void DB::MultiValuedQuery(string column, string pattern, SelectResult & results)
{
	assert(connection != nullptr);

	sql::Statement * stmt = nullptr;
	sql::ResultSet * res = nullptr;

	LOG(_log_, nullptr);
	if (IsAColumn(column))
	{
		try
		{
			// NOTE:
			// NOTE: SQL Injection vulnerability here.
			// NOTE:
			if ((stmt = connection->createStatement()) == nullptr)
				throw LOG(_log_, "createStatement() failed");

			string sql("select " + query_columns_no_path + string(" from tracks where "));
			sql += column + " like \"" + pattern + "\" order by " + column + ";";
			//cout << sql << endl;
			res = stmt->executeQuery(sql.c_str());

			LOG(_log_, nullptr);
			while (res->next())
			{
				//cout << LOG("") << endl;
				Row * r = results.add_row();
				r->set_type(ROW);
				google::protobuf::Map<string, string> * m = r->mutable_results();

				for (size_t i = 0; i < sizeof(track_column_names) / sizeof(string); i++)
				{
					// assumes path is zeroeth column and skips it.
					string col = track_column_names[i];
					if (i == 0)
						col = "id";
					string s;
					s = res->getString(col);
					(*m)[col] = s;
				}
			}
			LOG(_log_, nullptr);
		}
		catch (LoggedException s)
		{
		}
		catch (sql::SQLException &e)
		{
			PrintMySQLException(e);
		}
		LOG(_log_, to_string(results.ByteSize()));
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

	if (!rv && c == "id")
		rv = true;

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

string DB::PathFromID(unsigned int id, string * title, string * artist)
{
	string rv;
	assert(Initialized());
	string sql("select path,title,artist from tracks where id = " + to_string(id) + ";");
	sql::Statement * stmt = connection->createStatement();
	sql::ResultSet *res = nullptr;

	try
	{
		if (stmt == nullptr)
			throw LOG(_log_, "createStatement() failed");

		res = stmt->executeQuery(sql.c_str());
		if (res->next())
		{
			rv = res->getString("path");
			if (title != nullptr)
				*title = res->getString("title");
			if (artist != nullptr)
				*artist = res->getString("artist");
		}
	}
	catch (LoggedException s)
	{
	}
	catch (sql::SQLException &e)
	{
		PrintMySQLException(e);
	}
	return rv;
}

void DB::FindIDs(std::string column, std::string pattern, std::vector<std::string> & results)
{
}

