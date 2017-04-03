#include "file_system_component.hpp"

using namespace std;

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

/*	This component manages access to the file system providing
	services such as enumerating all music files in a subtree.

	Enumeration, for example, is a critical piece of the server
	as it is how media is discovered. I have been annoyed at how
	long it takes other systems to do this over thousands of
	songs. The key to speed is parallelism. On my NAS, at least,
	the top level of the music folder is very broad. The idea is
	to enumerate the top level and spin off one thread per top
	level directory which will recursively drill down on its
	own. This sounds tailor made for openmp.
*/

/* 	When discovering media, file extensions are compared to a list of
	blessed values. This function is called to do the comparison.

	based upon http://stackoverflow.com/questions/874134/find-if-string-ends-with-another-string-in-c
*/

inline bool HasEnding (string const & fullString, string const & ending)
{
	bool rv = false;

    if (fullString.length() >= ending.length())
	{
        rv = (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
    }
	return rv;
}

bool HasAllowedExtension(const char * file_name, const vector<string> & allowable_extensions)
{
	bool ok_extension = false;

	string s(file_name);
	for (auto it = allowable_extensions.begin(); it < allowable_extensions.end(); it++)
	{
		if (HasEnding(s, *it))
		{
			ok_extension = true;
			break;
		}
	}
	return ok_extension;
}

// NOTE:
// NOTE: It is likely additional file system loop prevention code will be needed.
// NOTE:
bool CausesLoop(const char * path)
{
	// Ignore . and .. to prevent (simple) loops
	return (strcmp(path, ".") == 0 || strcmp(path, "..") == 0);
}

/*	Assumption: allowable_extensions has already been vetted by caller.
	Assumption: db connection is not null.

	EnumerateForReal is the code that actually drills down into a directory
	hierarchy. It is called within its own thread. It is given its own connection
	to the database - in the case of sqlite this is required as database connections
	are not themselves threadsafe.

	As acceptable files are found (based upon their file extension) they are added
	to the database. My intent is to have this perform minimal sniffing of the media
	to glean things from the media's tags. I intend this to be done lazily later.

	Parameters:
	path					recursively grown path to the directory being evaluated.
	allowable_extensions	vector of file endings considered to be media.
	db						a pointer to a database connection
	tid						thread id

	Returns:
	none

	Note: tid is passed along for debugging purposes only.
*/

void EnumerateForReal(string path, vector<string> & allowable_extensions, DB * db, int tid, bool force)
{
	const string slash("/");
	vector<string> subdirs;
	DIR * d = nullptr;
	dirent * entry;

	assert(db != nullptr);

	try
	{
		if (!(d = opendir(path.c_str())))
			throw(LOG(path));

		if (!(entry = readdir(d)))
			throw(LOG(path));

		do
		{
			// NOTE: Common case - leave as first.
			if (entry->d_type == DT_REG)
			{
				if (!HasAllowedExtension(entry->d_name, allowable_extensions))
					continue;

				// Do something with this file.
				string s = path + slash + string(entry->d_name);
				db->AddMedia(s, force);
			}
			else if (entry->d_type == DT_DIR)
			{
				if (CausesLoop(entry->d_name))
					continue;

				// Possible TODO: Handle conversion of full paths to partial paths

				string next_path = path + slash + string(entry->d_name);
				EnumerateForReal(next_path, allowable_extensions, db, tid, force);
			}
		} while ((entry = readdir(d)) != nullptr);
	}
	catch (string s)
	{
		if (s.size() > 0)
			cerr << s << endl;
	}
	if (d != nullptr)
		closedir(d);
}

bool Enumerate(string path, vector<string> & allowed_extensions, string dbpath, bool force)
{
	bool rv = true;
	vector<string> tl_subdirs;
	DIR * tld;
	dirent * entry;

	try
	{
		if (allowed_extensions.size() == 0)
			throw(LOG("extension vector has size zero"));

		if (!(tld = opendir(path.c_str())))
		{
			rv = false;
			throw(LOG(path));
		}

		if (!(entry = readdir(tld)))
		{
			rv = false;
			throw(LOG(""));
		}

		do
		{
			// NOTE: This may skip links entirely. Is that a good thing?

			if (entry->d_type == DT_REG)
				continue;

			if (entry->d_type == DT_DIR)
			{
				if (CausesLoop(entry->d_name))
					continue;

				tl_subdirs.push_back(path + string("/") + string(entry->d_name));
			}
		} while ((entry = readdir(tld)) != nullptr);

		closedir(tld);

		//omp_set_dynamic(1);
		#pragma omp parallel for
		for (size_t i = 0; i < tl_subdirs.size(); i++)
		{
			// Avoiding throw's inside the parallel for.

			DB * db = new DB();

			if (db != nullptr)
			{
				if (db->Initialize(dbpath))
				{
					EnumerateForReal(tl_subdirs.at(i), allowed_extensions, db, i, force);
				}
				else
					cerr << LOG("") << endl;
				delete db;
			}
			else
			{
				cerr << LOG("DB failed to allocate") << endl;
			}
		}
	}
	catch (string m)
	{
		if (m.size() > 0)
			cerr << m << endl;
	}

	return rv;
}
