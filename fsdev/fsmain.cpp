#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <map>
#include <iomanip>
#include <regex>

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

string track_column_names[] =
{
	// Order must match select / insert code
	"parent",
	"fname",
	"namespace",
	"artist",
	"title",
	"album",
	"genre",
	"source",
	"duration",
	"publisher",
	"composer",
	"track",
	"copyright",
	"disc"
};

string query_columns;
string parameter_columns;

void InitPreparedStatement()
{
	query_columns = "(";
	parameter_columns = " values (";
	size_t n = sizeof(track_column_names) / sizeof(string);
	for (size_t i = 0; i < n; i++)
	{
		if (i > 0)
		{
			query_columns += ", ";
			parameter_columns += ", ";
		}
		query_columns += track_column_names[i];
		parameter_columns += "?";
	}
	query_columns += ") ";
	parameter_columns += ") ";
}

class Track
{
public:
	std::map<std::string, std::string> tags;

	std::string GetTag(std::string &);
	void SetTag(std::string & tag, std::string & value);
	void SetTag(std::string & raw);
	void SetPath(std::string & path);
	void PrintTags(int, int);
};

void Track::SetPath(string & path)
{
	string s("path");
	SetTag(s, path);
}

void Track::SetTag(string & raw)
{
	regex c("TAG:.*=.*\n");
	regex t(":.*=");
	regex v("=.*\n");

	if ((raw.size() > 12) && (raw.substr(0, 8) == "duration"))
	{
		string t = "duration";
		string v = raw.substr(9, string::npos);
		SetTag(t, v);
	}
	else if (regex_search(raw, c))
	{
		smatch mtag;
		smatch mval;
		string tag, value;

		regex_search(raw, mtag, t);
		regex_search(raw, mval, v);
		if (mtag.str(0).size() > 2)
			tag = string(mtag.str(0), 1, mtag.str(0).size() - 2);
		if (mval.str(0).size() > 2)
			value = string(mval.str(0), 1, mval.str(0).size() - 2);
		SetTag(tag, value);
	}
}

void Track::SetTag(string & tag, string & value)
{
	// All keys are squashed to lower case.
	transform(tag.begin(), tag.end(), tag.begin(), ::tolower);
	tags[tag] = value;
}

string Track::GetTag(string & tag)
{
	map<string, string>::iterator it;

	it = tags.find(tag);
	return (it != tags.end()) ? it->second : "";
}

struct DIRENT
{
	unsigned int type;
	int me;
	int up;
	string name;
	string fullpath;
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

bool CausesLoop(const char * path)
{
	return (strcmp(path, ".") == 0 || strcmp(path, "..") == 0);
}

void Discover(string path, const vector<string> & allowable_extensions, vector<DIRENT> & t, int parent, int & c, bool is_root)
{
		//sleep(1);
		//cerr << __FUNCTION__ << "\t" << __LINE__ << "\t" << "Discover entered: " << path << endl;

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
				Discover(next_path, allowable_extensions, t, r.me, c, false);
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
			stmt->execute("truncate tracks;");
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

bool InsertDirectory(sql::Connection * connection, DIRENT * d, string & nspace)
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
			s = "insert into paths (me, up, name, namespace) values(";
			s += to_string(d->me) + ", ";
			s += to_string(d->up) + ", ";
			s += "\'" + SanitizeString(d->name) + "\', ";
			s += "\'" + SanitizeString(nspace) + "\')";
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

bool InsertTrack(sql::Connection * connection, DIRENT * d, string & nspace)
{
	bool rv = false;
	FILE * p;

	if (access(d->fullpath.c_str(), R_OK) < 0)
	{
		cerr << "cannot access: " + d->fullpath << endl;
	}

	string cmdline = string("ffprobe -loglevel quiet -show_entries stream_tags:format_tags -show_entries format=duration -sexagesimal \"") + d->fullpath + string("\"");

	if ((p = popen(cmdline.c_str(), "r")) == nullptr)
	{
		perror("pipe to ffprobe failed");
		return rv;
	}

	const int bsize = 1024;
	char buffer[bsize] = { 0 };

	Track track;

	while (fgets(buffer, bsize, p) != nullptr)
	{
		string b(buffer);
		track.SetTag(b);
		memset(buffer, 0, bsize);
	}

	pclose(p);

	try
	{
		size_t n = sizeof(track_column_names) / sizeof(string);
		sql::PreparedStatement * stmt = nullptr;	
		string s = "insert into tracks " + query_columns + parameter_columns + ";";

		stmt = connection->prepareStatement(s.c_str());
		if (stmt == nullptr)
		{
			cerr << "prepareStatement() failed" << endl;
			return rv;
		}
		stmt->setString(1, to_string(d->up));
		stmt->setString(2, d->name);
		stmt->setString(3, nspace);

		for (size_t i = 3; i < n; i++)
		{
			stmt->setString(i + 1, track.GetTag(track_column_names[i]));
		}

		stmt->execute();
		delete stmt;
		rv = true;
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
	string nspace;

	InitPreparedStatement();

	valid_extensions.push_back("mp3");
	valid_extensions.push_back("flac");
	valid_extensions.push_back("wav");
	valid_extensions.push_back("m4a");
	valid_extensions.push_back("ogg");


	if (ParseOptions(argc, argv, do_db, root, nspace))
	{
		if (*(root.end() - 1) == '/')
			root.erase(root.end() - 1);

		Discover(root, valid_extensions, t, -1, zero, true);
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
					InsertDirectory(connections[omp_get_thread_num()], &t[i], nspace);
				else
					InsertTrack(connections[omp_get_thread_num()], &t[i], nspace);
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
