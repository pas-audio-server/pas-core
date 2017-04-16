#include "file_system_component.hpp"
#include "logger.hpp"

using namespace std;

extern Logger _log_;

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

/*	HasAllowedExtension() returns true if the given path ends in
	one of the blessed extensions such as mp3, flac, etc.
*/
/*
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
*/

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
	force					of true, "insert or replace" is used on the database
							rather than "insert".

	Returns:
	none

	Note: tid is passed along for debugging purposes only.
*/

/*
static void AddFolders(string path, vector<string> & v)
{
	const string slash("/");
	DIR * d = nullptr;
	dirent * entry;

	try
	{
		if (!(d = opendir(path.c_str())))
			throw LOG(_log_, path);

		if (!(entry = readdir(d)))
			throw LOG(_log_, path);

		do
		{
			if (entry->d_type == DT_DIR)
			{
				if (CausesLoop(entry->d_name))
					continue;
				string next_path = path + slash + string(entry->d_name);
				v.push_back(next_path);
				AddFolders(next_path, v);
			}
		} while ((entry = readdir(d)) != nullptr);

		if (d != nullptr)
			closedir(d);
	}
	catch (LoggedException s)
	{
	}
}

static void EnumerateFolder(string folder, const vector<string> & allowable_extensions)
{
	const string slash("/");
	DIR * d = nullptr;
	dirent * entry;

	DB * db = new DB();
	db->Initialize();
	try
	{
		if (db == nullptr)
			throw LOG(_log_, "Allocating a DB failed");

		if (!db->Initialized())
			throw LOG(_log_, "DB failed to initialize");

		if (!(d = opendir(folder.c_str())))
			throw LOG(_log_, folder);

		if (!(entry = readdir(d)))
			throw LOG(_log_, folder);

		do
		{
			if (entry->d_type == DT_REG)
			{
				if (!HasAllowedExtension(entry->d_name, allowable_extensions))
					continue;

				// Do something with this file.
				string s = folder + slash + string(entry->d_name);
				db->AddMedia(s, false);
			}
		} while ((entry = readdir(d)) != nullptr);
	}
	catch (LoggedException s)
	{
	}

	if (db)
	{
		db->DeInitialize();
		delete db;
	}

	if (d)
		closedir(d);
}

bool Enumerate2(string path, vector<string> & allowed_extensions, bool force)
{
	bool rv = true;
	vector<string> dirs;

	LOG(_log_, "Starting to enumerate folders");
	try
	{
		if (allowed_extensions.size() == 0)
			throw LOG(_log_, "extension vector has size zero");

		dirs.push_back(path);
		AddFolders(path, dirs);
		LOG(_log_, "Folders: " + to_string(dirs.size()));

		#pragma omp parallel for
		for (int i = 0; i < (int) dirs.size(); i++)
		{
			EnumerateFolder(dirs.at(i), (const vector<string>) allowed_extensions);
		}
		LOG(_log_, "Done");
	}
	catch (LoggedException m)
	{
	}

	return rv;
}
*/
