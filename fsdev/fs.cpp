#include "fs.h"

using namespace std;
using namespace pas;

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

bool CausesLoop(const char * path)
{
	return (strcmp(path, ".") == 0 || strcmp(path, "..") == 0);
}

void Discover(string path, const vector<string> & allowable_extensions, vector<DIRENT> & t, int parent, int & c, bool is_root, Logger & _log_)
{
	//sleep(1);
	LOG(_log_, "Discover entered: " + path);

	const string slash("/");
	DIR * d = nullptr;
	dirent * entry;
	int me = c++;

	DIRENT r;
	r.name = (is_root) ? path.c_str() : string(basename((char *) path.c_str()));
	r.type = DT_DIR;
	r.up = parent;
	r.me = me;
	t.push_back(r);

	if (!(d = opendir(path.c_str())))
		throw LOG(_log_, "opendir failed: " + path);

	while ((entry = readdir(d)) != nullptr)
	{
		if (entry->d_type == DT_DIR)
		{
			if (CausesLoop(entry->d_name))
			{
				continue;
			}

			string next_path = path + slash + string(entry->d_name);
			Discover(next_path, allowable_extensions, t, r.me, c, false, _log_);
		}
		else if (entry->d_type == DT_REG)
		{
			if (entry->d_name[0] == '.')
				continue;

			if (!HasAllowedExtension(entry->d_name, allowable_extensions))
			{
				continue;
			}

			DIRENT f;
			f.name = string(entry->d_name);
			f.fullpath = path + "/" + f.name;
			f.type = DT_REG;
			f.up = me;
			f.me = -1;
			t.push_back(f);
		}
	}

	if (d != nullptr)
		closedir(d);
}
