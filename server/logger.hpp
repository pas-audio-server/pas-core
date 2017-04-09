#pragma once
#include <iostream>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <string>
#include <sstream>
#include <assert.h>

class  LoggedException
{
public:

	LoggedException(std::string s) { msg = s; }
	std::string Msg() { return msg; }

private:
	std::string msg;
};

#define	LOG(l, m)	l.Add(__FILE__, __FUNCTION__, __LINE__, (m))

class Logger
{
public:

	Logger(std::string path);
	~Logger();

	LoggedException Add(const char * file, const char * function, const int line, const std::string message);
	LoggedException Add(const char * file, const char * function, const int line, const char * message = nullptr);
	void Sync();

private:

	std::ofstream log;
	std::mutex m;
};

