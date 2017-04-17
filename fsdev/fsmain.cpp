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

/*  pas is Copyright 2017 by Perry Kivolowitz
 */

#include "fs.h"

using namespace std;
using namespace pas;

Logger _log_("/tmp/fslog.txt", LogLevel::MINIMAL);

extern const int BSIZE;

// Experimenting with new indentation style - I haven't changed in 40 years. Change is good.

bool ParseOptions(int argc, char * argv[], bool & real_db, string & path, string & nspace, bool & do_checksum_analysis)
{
	bool rv = false;
	int opt;

	path = "";
	nspace = "default";
	real_db = false;
	do_checksum_analysis = false;

	while ((opt = getopt(argc, argv, "cn:dp:")) != -1) {
		switch (opt) {
			case 'c':
			do_checksum_analysis = true;
			break;

			case 'n':
			nspace = string(optarg);
			break;

			case 'd':
			real_db = true;
			break;

			case 'p':
			path = string(optarg);
			rv = true;
			break;
		}
	}

	if (real_db && do_checksum_analysis) {
		cerr << "-c and -d are mutually exclusive while -c is experimental" << endl;
		rv = false;
	}

	if (!rv) {
		cerr << "Usage:" << endl;
		cerr << "-c         perform checksum analysis - experimental - not for production." << endl;
		cerr << "-d         enables real DB writing" << endl;
		cerr << "-n nspace  defines which \"namespace\" will be added in the DB." << endl;
		cerr << "           If omitted, \"default\" is used." << endl;
		cerr << "-p path    required - sets root of search" << endl;
	}
	return rv;
}

string RebuildPath(vector<DIRENT> & t, int i, map<int, DIRENT *> & m)
{
	string rv;

	if (t[i].type == DT_REG)
	{
		rv = t[i].name;
		DIRENT * p = &t[i];

		while (p->up != -1)
		{
			p = m[p->up];
			rv = p->name + "/" + rv;
		}
	}
	return rv;
}

/*	This is experimental code. It is not intended for production.
*/

void AnalyzeChecksums(vector<DIRENT> & t, Logger & _log_)
{
	char buffer[BSIZE] = { 0 };
	map<string, string> checksum_crosswalk;

	// Build a map crosswalking DIRENT indexes to pointers to the DIRENT.
	map<int, DIRENT *> directory_index;
	for (size_t i = 0; i < t.size(); i++)
	{
		if (t.at(i).type == DT_DIR)
		{
			directory_index[t.at(i).me] = &t[i];
		}
	}

	// Visit every file - rebuilding the original path.
	for (size_t i = 0; i < t.size(); i++)
	{
		if (t.at(i).type == DT_REG)
		{
			string path = RebuildPath(t, i, directory_index);
			if (path.size() == 0)
				continue;
			string command_line = "md5sum -b \"" + path + "\"";
			cerr << command_line << endl;
			FILE * p = popen(command_line.c_str(), "r");
			if (!p) {
				cerr << "pipe failed" << endl;
				continue;
			}
			if (buffer == fgets(buffer, BSIZE, p)) {
				stringstream ss;
				ss << string(buffer);
				string check_sum;
				ss >> check_sum;
				if (checksum_crosswalk[check_sum] == "") {
					checksum_crosswalk[check_sum] = t.at(i).name;
				}
				else {
					cerr << "checksum match: " << check_sum << "'t" << t.at(i).name << "\t" << checksum_crosswalk[check_sum] << endl;
				}
			}
			pclose(p);
		}
	}
}

int main(int argc, char * argv[])
{
	int rv = 0;
	int zero = 0;
	vector<DIRENT> t;
	vector<string> valid_extensions { "mp3", "flac", "wav", "m4a", "ogg" };
	bool do_db = false;
	string root;
	string nspace;
	bool do_checksum_analysis = false;

	LOG(_log_, "");
	try {
		if (ParseOptions(argc, argv, do_db, root, nspace, do_checksum_analysis)) {
			if (*(root.end() - 1) == '/')
				root.erase(root.end() - 1);

			Discover(root, valid_extensions, t, -1, zero, true, _log_);
		}
		LOG(_log_, "");

		if (do_db && !do_checksum_analysis) {
			DoTheDB(nspace, t, _log_);
		}
		LOG(_log_, "");

		if (do_checksum_analysis) {
			AnalyzeChecksums(t, _log_);
		}
	}
	catch (LoggedException e) {
		rv = 1;
		cerr << e.Msg() << endl;
	}
	return rv;
}
