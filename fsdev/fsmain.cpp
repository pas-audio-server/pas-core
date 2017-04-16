#include "fs.h"

using namespace std;
using namespace pas;

Logger _log_("/tmp/fslog.txt", LogLevel::MINIMAL);

bool ParseOptions(int argc, char * argv[], bool & real_db, string & path, string & nspace)
{
	bool rv = false;
	int opt;

	path = "";
	nspace = "default";
	real_db = false;

	while ((opt = getopt(argc, argv, "n:dp:")) != -1) 
	{
		switch (opt) 
		{
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

	if (!rv)
	{
		cerr << "Usage:" << endl;
		cerr << "-d       enables real DB writing" << endl;
		cerr << "-p path  required - sets root of search" << endl;
	}
	return rv;
}

int main(int argc, char * argv[])
{
	int rv = 0;
	int zero = 0;
	vector<DIRENT> t;
	vector<string> valid_extensions;
	bool do_db = false;
	string root;
	string nspace;

	valid_extensions.push_back("mp3");
	valid_extensions.push_back("flac");
	valid_extensions.push_back("wav");
	valid_extensions.push_back("m4a");
	valid_extensions.push_back("ogg");

	LOG(_log_, "");
	try
	{
		if (ParseOptions(argc, argv, do_db, root, nspace))
		{
			if (*(root.end() - 1) == '/')
				root.erase(root.end() - 1);

			Discover(root, valid_extensions, t, -1, zero, true, _log_);
		}
		LOG(_log_, "");

		LOG(_log_, "");
		if (do_db)
		{
			DoTheDB(nspace, t, _log_);
		}
		LOG(_log_, "");
	}
	catch (LoggedException e)
	{
		rv = 1;
		cerr << e.Msg() << endl;
	}
	return rv;
}

/*
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


	map<int, DIRENT *> directory_index;
	for (size_t i = 0; i < t.size(); i++)
	{
		if (t.at(i).type == DT_DIR)
		{
			directory_index[t.at(i).me] = &t[i];
		}
	}
	for (size_t i = 0; i < t.size(); i++)
	{
		if (t.at(i).type == DT_REG)
		{
			string p = RebuildPath(t, i, directory_index);
			if (p.size() == 0)
				continue;
			cerr << p << endl;
		}
	}
	//cerr << setw(64) << left << it->name << ((it->type == DT_DIR) ? "D" : "F") << right << setw(8) << it->up << right << setw(8) << it->me << endl;
*/
