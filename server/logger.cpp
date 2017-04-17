#include "logger.hpp"
#include <exception>

using namespace std;
using namespace pas;

Logger::Logger(string path, LogLevel l)
{
	log.open(path);
	if (!log.is_open())
		throw std::runtime_error("failed to open log file");
	level = l;
}

Logger::~Logger()
{
	if (log.is_open())
		log.close();
}

void Logger::Sync()
{
	if (log.is_open())
		log.flush();
}

LoggedException Logger::Add(const char * file, const char * function, const int line, const string message, LogLevel ll)
{
	return Add(file, function, line, message.c_str(), ll);
}

LoggedException Logger::Add(const char * file, const char * function, const int line, const char * message, LogLevel ll)
{
	// Reject rediculous messages if we're not on that setting.
	// Reject verbose messages if the log is closed.
	if ((ll == REDICULOUS && ll != level) || (ll == VERBOSE && !log.is_open())) {
		return LoggedException("");
	}

	stringstream ss;
	string s;

	ss << setw(24) << left << file << " " << setw(24) << function << " " << setw(6) << line;
	if (message != nullptr)
		ss << " " << message;

	s = ss.str();

	if (ll <= level && log.is_open())
	{
		m.lock();
		log << s << endl;
		m.unlock();
	}
	return LoggedException(s, ll);
}
