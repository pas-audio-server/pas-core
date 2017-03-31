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

// based upon http://stackoverflow.com/questions/874134/find-if-string-ends-with-another-string-in-c

inline bool HasEnding (std::string const &fullString, std::string const &ending)
{
	bool rv = false;

    if (fullString.length() >= ending.length()) 
	{
        rv = (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
    }
	return rv;
}

/*	Assumption: allowable_extensions has already been vetted by caller.
*/

void EnumerateForReal(string path, vector<string> & allowable_extensions, string & dbname, DB * db, int tid)
{
	vector<string> subdirs;
	DIR * d;
	dirent * entry;

	assert(db != nullptr);

	if (!(d = opendir(path.c_str())))
	{
		LOG(path);
		goto bottom;
	}

	if (!(entry = readdir(d)))
	{
		LOG(path);
		goto bottom;
	}
	
	do
	{
		if (entry->d_type == DT_REG)
		{
			// NOTE: Common case - leave as first.
			bool ok_extension = false;
			string s(entry->d_name);
			for (auto it = allowable_extensions.begin(); it < allowable_extensions.end(); it++)
			{
				if (HasEnding(s, *it))
				{
					ok_extension = true;
					break;
				}
			}
			if (!ok_extension)
				continue;

			cout << tid << '\t' << path << "/" << s << endl;
		}
		else if (entry->d_type == DT_DIR)
		{
			// Ignore . and .. to prevent (simple) loops
			if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
				continue;

			// TODO: Handle conversion of full paths to partial paths

			string next_path = path + string("/") + string(entry->d_name);
			string s = "";
			EnumerateForReal(next_path, allowable_extensions, s, db, tid);
		}
	} while ((entry = readdir(d)) != nullptr);

	closedir(d);

bottom:

	return;	
}

/* goto's ? relax - Dijkstra said alarm exits are ok. See the last two paragraphs
   of the foundational paper:
   
   http://homepages.cwi.nl/~storm/teaching/reader/Dijkstra68.pdf
   
   The pas yet-to-be-written style guide will specify at most one label can be
   defined for alarm exits so as to provide a clear and simple means of keeping
   the number of return paths in a function to one. In this regard, goto is
   being used as a higher speed throw().
*/

bool Enumerate(string path, vector<string> & allowed_extensions, string dbpath)
{
	bool rv = true;
	vector<string> tl_subdirs;
	DIR * tld;
	dirent * entry;

	if (allowed_extensions.size() == 0)
	{
		LOG("");
		goto bottom;
	}

	if (!(tld = opendir(path.c_str())))
	{
		LOG(path);
		rv = false;
		goto bottom;
	}

	if (!(entry = readdir(tld)))
	{
		LOG("");
		rv = false;
		goto bottom;
	}
	
	do
	{
		// NOTE: This may skip symbolic links entirely. Is that a good thing?

		// The common case short circuit.
		if (entry->d_type == DT_REG)
			continue;

		if (entry->d_type == DT_DIR)
		{
			// Ignore . and .. to prevent (simple) loops

			if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
				continue;
			tl_subdirs.push_back(path + string("/") + string(entry->d_name));
		}
	} while ((entry = readdir(tld)) != nullptr);

	closedir(tld);

//	for (auto it = tl_subdirs.begin(); it < tl_subdirs.end(); it++)
//		cout << *it << endl;

	//omp_set_dynamic(0);
	#pragma omp parallel for num_threads(64)
	for (size_t i = 0; i < tl_subdirs.size(); i++)
	{
		DB * db = new DB();

		if (db != nullptr)
		{ 
			if (db->Initialize(dbpath))
			{
				EnumerateForReal(tl_subdirs.at(i), allowed_extensions, dbpath, db, i);
			}
			else
				LOG("");
		}

		if (db != nullptr)
		{
			delete db;
		}
		else
		{
			LOG("Impossible");
		}
	}

bottom:
	return rv;	
}

