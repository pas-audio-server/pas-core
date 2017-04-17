#include "fs.h"
#include "db.hpp"

#include <sstream>

using namespace std;
using namespace pas;

const int BSIZE = 1024;

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

bool TruncateTables(sql::Connection * connection, Logger & _log_)
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
		stringstream ss;
		ss << "# ERR: SQLException in " << __FILE__;
		ss << "(" << __FUNCTION__ << ") on line " << __LINE__ << endl;
		ss << "# ERR: " << e.what();
		ss << " (MySQL error code: " << e.getErrorCode();
		ss << ", SQLState: " << e.getSQLState() << " )" << endl;
		throw LOG2(_log_, ss.str(), LogLevel::FATAL);
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

bool InsertDirectory(sql::Connection * connection, DIRENT * d, string & nspace, Logger & _log_)
{
	bool rv = false;

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
			stmt->execute(s.c_str());
			rv = true;
		}
		else { 
			throw LOG2(_log_, "createStatement failed", LogLevel::FATAL);
		}
	}
	catch (sql::SQLException & e)
	{
		stringstream ss;
		ss << "# ERR: SQLException in " << __FILE__;
		ss << "(" << __FUNCTION__ << ") on line " << __LINE__ << endl;
		ss << "# ERR: " << e.what();
		ss << " (MySQL error code: " << e.getErrorCode();
		ss << ", SQLState: " << e.getSQLState() << " )" << endl;
		throw LOG2(_log_, ss.str(), LogLevel::FATAL);
	}
	return rv;
}

bool InsertTrack(sql::Connection * connection, DIRENT * d, string & nspace, Logger & _log_)
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

	char buffer[BSIZE] = { 0 };

	Track track;

	while (fgets(buffer, BSIZE, p) != nullptr)
	{
		string b(buffer);
		track.SetTag(b);
		memset(buffer, 0, BSIZE);
	}

	pclose(p);

	size_t n = sizeof(track_column_names) / sizeof(string);
	sql::PreparedStatement * stmt = nullptr;	
	string s = "insert into tracks " + query_columns + parameter_columns + ";";

	try
	{

		stmt = connection->prepareStatement(s.c_str());
		if (stmt == nullptr) {
			throw LOG2(_log_, "prepareStatement() failed", LogLevel::FATAL);
		}
		stmt->setString(1, to_string(d->up));
		stmt->setString(2, d->name);
		stmt->setString(3, nspace);

		for (size_t i = 3; i < n; i++) {
			stmt->setString(i + 1, track.GetTag(track_column_names[i]));
		}

		stmt->execute();
		delete stmt;
		rv = true;
	}
	catch (sql::SQLException & e)
	{
		stringstream ss;
		ss << "# ERR: SQLException in " << __FILE__;
		ss << "(" << __FUNCTION__ << ") on line " << __LINE__ << endl;
		ss << "# ERR: " << e.what();
		ss << " (MySQL error code: " << e.getErrorCode();
		ss << ", SQLState: " << e.getSQLState() << " )" << endl;
		throw LOG2(_log_, ss.str(), LogLevel::FATAL);
	}
	return rv;
}

void DoTheDB(string & nspace, vector<DIRENT> & t, Logger & _log_)
{
	InitPreparedStatement();
	omp_set_num_threads(omp_get_max_threads());

	sql::Driver * driver = nullptr;
	sql::Connection * connections[(const int) omp_get_max_threads()] = { nullptr };

	if ((driver = get_driver_instance()))
	{
		cerr << "have driver instance" << endl;
		for (int i = 0; i < omp_get_max_threads(); i++)
		{
			// NOTE:
			// NOTE: Must catch SQL exceptions here but do not. This is a bug.
			// NOTE:
			connections[i] = driver->connect("tcp://192.168.1.117:3306", "pas", "pas");
			connections[i]->setSchema("pas2");
			LOG(_log_, "have connection and schema");
		}
		TruncateTables(connections[0], _log_);
		#pragma omp parallel for
		for (size_t i = 0; i < t.size(); i++)
		{
			if (t[i].type == DT_DIR)
				InsertDirectory(connections[omp_get_thread_num()], &t[i], nspace, _log_);
			else
				InsertTrack(connections[omp_get_thread_num()], &t[i], nspace, _log_);
		}
		for (int i = 0; i < omp_get_max_threads(); i++)
		{
			connections[i]->close();
			delete connections[i];
			LOG(_log_, "deleted connection");
		}
	}
	else
	{
		throw LOG2(_log_, "no driver instance", LogLevel::FATAL);
	}
}
