#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <map>
#include <iomanip>

#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <getopt.h>
#include <libgen.h>
#include <omp.h>

#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>

using namespace std;

#define LOG()	(string(__FUNCTION__) + string(" ") + to_string(__LINE__))

struct DIRENT
{
	unsigned int type;
	int me;
	int up;
	string name;
};

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

bool ParseOptions(int argc, char * argv[], bool & real_db, string & path)
{
	bool rv = false;
	int opt;

	path = "";
	real_db = false;

	while ((opt = getopt(argc, argv, "dp:")) != -1) 
	{
		switch (opt) 
		{
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

bool CausesLoop(const char * path)
{
	return (strcmp(path, ".") == 0 || strcmp(path, "..") == 0);
}

void Discover(string path, const vector<string> & allowable_extensions, vector<DIRENT> & t, int parent, int & c)
{
		//sleep(1);
		//cerr << __FUNCTION__ << "\t" << __LINE__ << "\t" << "Discover entered: " << path << endl;

	const string slash("/");
	DIR * d = nullptr;
	dirent * entry;
	int me = c++;

	DIRENT r;
	r.name = string(basename((char *) path.c_str()));
	r.type = DT_DIR;
	r.up = parent;
	r.me = me;
	t.push_back(r);

	try
	{
		if (!(d = opendir(path.c_str())))
			throw path + string("\n");

		while ((entry = readdir(d)) != nullptr)
		{
			if (entry->d_type == DT_DIR)
			{
				if (CausesLoop(entry->d_name))
				{
					continue;
				}

				string next_path = path + slash + string(entry->d_name);
				Discover(next_path, allowable_extensions, t, r.me, c);
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
				f.type = DT_REG;
				f.up = me;
				f.me = -1;
				t.push_back(f);
			}
		}

		if (d != nullptr)
			closedir(d);
	}
	catch (string s)
	{
		cerr << __FUNCTION__ << "\t" << __LINE__ << "\t" << "catch" << endl;
		if (s.size() > 0)
			cerr << s << endl;
	}
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

bool TruncateTables(sql::Connection * connection)
{
	bool rv = false;
	sql::Statement * stmt = connection->createStatement();
	try
	{
		if (stmt)
		{
			stmt->execute("truncate paths;");
			cerr << __FUNCTION__ << "\t" << __LINE__ << endl;
			delete stmt;
			rv = true;
		}
	}
	catch (sql::SQLException & e)
	{
		cout << "# ERR: SQLException in " << __FILE__;
		cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << endl;
		cout << "# ERR: " << e.what();
		cout << " (MySQL error code: " << e.getErrorCode();
		cout << ", SQLState: " << e.getSQLState() << " )" << endl;
	}
	return rv;
}

string SanitizeString(string & s)
{
	string rv;
	rv.reserve(s.size() * 2);
	for (size_t i = 0; i < s.size(); i++)
	{
		switch (s[i])
		{
			case '\'':
			rv += "''";
			break;

			default:
			rv += s[i];
			break;
		}
	}
	return rv;
}

bool InsertDirectory(sql::Connection * connection, DIRENT * d)
{
	bool rv = false;
		//cout << d->me << "\t" << d->up << "\t" << d->name << endl;
		//return true;

	sql::Statement * stmt = connection->createStatement();
	try
	{
		if (stmt)
		{
			string s;
			s = "insert into paths (me, up, name) values(";
			s += to_string(d->me) + ", ";
			s += to_string(d->up) + ", ";
			s += "\'" + SanitizeString(d->name) + "\')";
			s += ";";
			//cout << s << endl;
			stmt->execute(s.c_str());
			rv = true;
		}
	}
	catch (sql::SQLException & e)
	{
		cout << "# ERR: SQLException in " << __FILE__;
		cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << endl;
		cout << "# ERR: " << e.what();
		cout << " (MySQL error code: " << e.getErrorCode();
		cout << ", SQLState: " << e.getSQLState() << " )" << endl;
	}
	return rv;
}

bool InsertTrack(sql::Connection * connection, DIRENT * d)
{
	bool rv = false;
	//cout << d->me << "\t" << d->up << "\t" << d->name << endl;
	//return true;

	sql::Statement * stmt = connection->createStatement();
	try
	{
		if (stmt)
		{
			string s;
			s = "insert into tracks (parent, fname) values(";
			s += to_string(d->up) + ", ";
			s += "\'" + SanitizeString(d->name) + "\')";
			s += ";";
			//cout << s << endl;
			stmt->execute(s.c_str());
			rv = true;
		}
	}
	catch (sql::SQLException & e)
	{
		cout << "# ERR: SQLException in " << __FILE__;
		cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << endl;
		cout << "# ERR: " << e.what();
		cout << " (MySQL error code: " << e.getErrorCode();
		cout << ", SQLState: " << e.getSQLState() << " )" << endl;
	}
	return rv;
}

int main(int argc, char * argv[])
{
	int zero = 0;
	vector<DIRENT> t;
	vector<string> valid_extensions;
	bool do_db = false;
	string root;

	valid_extensions.push_back("mp3");
	valid_extensions.push_back("flac");
	valid_extensions.push_back("wav");
	valid_extensions.push_back("m4a");
	valid_extensions.push_back("ogg");


	if (ParseOptions(argc, argv, do_db, root))
	{
		if (*(root.end() - 1) == '/')
			root.erase(root.end() - 1);

		Discover(root, valid_extensions, t, -1, zero);
	}

	if (do_db)
	{
		omp_set_num_threads(omp_get_max_threads());

		sql::Driver * driver = nullptr;
		sql::Connection * connections[(const int) omp_get_max_threads()] = { nullptr };

		if ((driver = get_driver_instance()))
		{
			cerr << "have driver instance" << endl;
			for (int i = 0; i < omp_get_max_threads(); i++)
			{
				connections[i] = driver->connect("tcp://192.168.1.117:3306", "pas", "pas");
				connections[i]->setSchema("pas2");
				cerr << "have connection" << endl;
			}
			TruncateTables(connections[0]);
			#pragma omp parallel for
			for (size_t i = 0; i < t.size(); i++)
			{
				//cout << t[i].me << "\t" << t[i].up << "\t" << t[i].name << endl;
				if (t[i].type == DT_DIR)
					InsertDirectory(connections[omp_get_thread_num()], &t[i]);
				else
					InsertTrack(connections[omp_get_thread_num()], &t[i]);
			}
			for (int i = 0; i < omp_get_max_threads(); i++)
			{
				connections[i]->close();
				delete connections[i];
				cerr << "deleted connection" << endl;
			}
		}
		else
		{
			cerr << "no driver instance" << endl;
		}
	}

	return 0;
}

	/*
		map<int, DIRENT *> directory_index;
		for (size_t i = 0; i < t.size(); i++)
		{
			if (t.at(i).type == DT_DIR)
			{
				directory_index[t.at(i).me] = &t[i];
			}
		}
	*/
	/*
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
