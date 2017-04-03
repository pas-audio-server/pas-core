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
	"source"
};

DB::DB()
{
	db = nullptr;
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
	if (db != nullptr)
		sqlite3_close(db);
}

bool DB::Initialize(string dbname)
{
	bool rv = true;
	int rc;

	assert(db == nullptr);

	if ((rc = sqlite3_open_v2(dbname.c_str(), &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX, NULL)) != SQLITE_OK)
    {
        cerr << LOG(sqlite3_errmsg(db)) << endl;
		sqlite3_close(db);
		db = nullptr;
		rv = false;
    }

	return rv;
}

bool DB::AddMedia(std::string & path, bool force)
{
	bool rv = true;
	FILE * p = nullptr;
	int rc;

	try
	{
		if (access(path.c_str(), R_OK) < 0)
			throw string("cannot access: ") + path;

		string cmdline = string("ffprobe -loglevel quiet -show_entries stream_tags:format_tags \"") + path + string("\"");
		//cout << "Attempting: " << cmdline << endl;

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
		//track.PrintTags(18, 50);

		// NOTE:
		// NOTE: It is a map of tags to values. As more cpulmns are added, change track_column_names.
		// NOTE:

		string sql = string("insert") + (force ? string(" or replace") : string("")) + " into tracks " + query_columns + parameter_columns;
		sqlite3_stmt * stmt;

		while ((rc = sqlite3_prepare_v2(db, sql.c_str(), sql.size(), &stmt, nullptr)) != SQLITE_OK)
		{
			if (rc == SQLITE_BUSY)
				usleep(_db_sleep_time);
			else
				throw LOG("prepare failed " + ::to_string(rc));
		}

		int i = 1;
		for (auto it = supported_track_column_names.begin(); it < supported_track_column_names.end(); it++, i++)
		{
			string v = track.GetTag(*it);
			//cout << *it << "	" << v << endl;

			if (v.size() > 0)
				rc = sqlite3_bind_text(stmt, i, v.c_str(), -1, SQLITE_TRANSIENT);
			else
				rc = sqlite3_bind_null(stmt, i);

			if (rc != SQLITE_OK)
				throw LOG("bind failed");
		}

		while ((rc = sqlite3_step(stmt)) != SQLITE_DONE)
		{
			// The step may not have completed due to contention for sqlite. If so,
			// the return will be BUSY. If it isn't BUSY and we're
			if (rc == SQLITE_BUSY)
			{
				usleep(_db_sleep_time);
				continue;
			}
			if (!force && rc == SQLITE_CONSTRAINT)
			{
				// I wonder if this is too slow.
				throw(string(""));
			}
			throw LOG("step failed " + ::to_string(rc));
		}

		if (sqlite3_finalize(stmt) != SQLITE_OK)
			throw LOG("finalize failed");
	}
	catch (string s)
	{
		if (s.size() > 0)
		{
			cerr << s << endl;
			rv = false;
		}
	}

	if (p != nullptr)
		pclose(p);

	return rv;
}

int DB::GetTrackCount()
{
	int rv = -1;
	int rc;

	assert(db != nullptr);

	string sql("select count(*) from tracks;");
	rc = sqlite3_exec(db, sql.c_str(), _db_GetTrackCount, &rv, nullptr);

	if (rc < 0)
		rv = rc;

	return rv;
}

bool DB::Initialized()
{
	return db != nullptr;
}

int DB::_db_GetTrackCount(void * rv, int argc, char * argv[], char * cols[])
{
	assert(rv != nullptr);
	*((int *) rv) = 0;

	if (argc > 0)
		*((int *) rv) = atoi(argv[0]);

	return 0;
}
