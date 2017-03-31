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

/*
            listdir(path, level + 1);
        }
        else
            printf("%*s- %s\n", level*2, "", entry->d_name);
    } while (entry = readdir(dir));
}
*/

#define	LOG(m)	{ cout << __FILE__ << " " << __LINE__ << " " << m << endl; }

void EnumerateForReal(string path, vector<string> allowable_extensions)
{
	cout << "+" << endl;
}

bool Enumerate(string path, vector<string> allowed_extensions)
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

	#pragma omp parallel for
	for (size_t i = 0; i < tl_subdirs.size(); i++)
	{
		EnumerateForReal(tl_subdirs.at(i), allowed_extensions);
	}

bottom:
	return rv;	
}

