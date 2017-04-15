#pragma once
#include <iostream>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <string>
#include <sstream>
#include <assert.h>
#include "../protos/cpp/commands.pb.h"

class  LoggedException
{
public:

	LoggedException(std::string s) { msg = s; }
	std::string Msg() { return msg; }

private:
	std::string msg;
};

#define	LOG(l, m)		l.Add(__FILE__, __FUNCTION__, __LINE__, (m))
#define	LOG2(l, m, v)	l.Add(__FILE__, __FUNCTION__, __LINE__, (m), (v))

class Logger
{
public:

	Logger(std::string path, pas::LogLevel level = pas::LogLevel::FATAL);
	~Logger();

	LoggedException Add(const char * file, const char * function, const int line, const std::string message, pas::LogLevel ll = pas::LogLevel::VERBOSE);
	LoggedException Add(const char * file, const char * function, const int line, const char * message = nullptr, pas::LogLevel ll = pas::LogLevel::VERBOSE);
	void Sync();

private:

	std::ofstream log;
	std::mutex m;
	pas::LogLevel level;
};

