#include "logger.hpp"
#include <exception>

using namespace std;

Logger::Logger(string path)
{
	log.open(path);
	if (!log.is_open())
		throw std::runtime_error("failed to open log file");
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

LoggedException Logger::Add(const char * file, const char * function, const int line, const string message)
{
	return Add(file, function, line, message.c_str());
}

LoggedException Logger::Add(const char * file, const char * function, const int line, const char * message)
{
	stringstream ss;
	string s;

	ss << setw(24) << left << file << " " << setw(16) << function << " " << setw(6) << line;
	if (message != nullptr)
		ss << " " << message;

	s = ss.str();

	if (log.is_open())
	{
		m.lock();
		log << s << endl;
		m.unlock();
	}
	return LoggedException(s);
}
