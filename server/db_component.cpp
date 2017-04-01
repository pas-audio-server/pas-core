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

#include "db_component.hpp"

using namespace std;

DB::DB()
{
	db = nullptr;
}

DB::~DB()
{
	if (db != nullptr)
		sqlite3_close(db);
}

bool DB::Initialize(string dbname)
{
	bool rv = true;
	int rc = 1;

	assert(db == nullptr);

	if ((rc = sqlite3_open_v2(dbname.c_str(), &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX, NULL)) != SQLITE_OK)
    {
        LOG(sqlite3_errmsg(db));
		sqlite3_close(db);
		db = nullptr;
		rv = false;
        goto bottom;
    }

bottom:

	return rv;
}

bool DB::AddTrack(const Track & t)
{
	bool rv = true;

	assert(db != nullptr);

	return rv;
}

int DB::GetTrackCount()
{
	int rv = -1;
	int rc;

	assert(db != nullptr);

	string sql("select count(*) from tracks;");
	rc = sqlite3_exec(db, sql.c_str(), _cb_GetTrackCount, &rv, nullptr);

	if (rc < 0)
		rv = rc;
 
	return rv;
}

bool DB::Initialized()
{
	return db != nullptr;
}

int DB::_cb_GetTrackCount(void * rv, int argc, char * argv[], char * cols[])
{
	assert(rv != nullptr);
	*((int *) rv) = 0;

	if (argc > 0)
		*((int *) rv) = atoi(argv[0]);

	return 0;
}
