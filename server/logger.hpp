#pragma once
/*	This file is part of pas.

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

/*	pas is Copyright 2017 by Perry Kivolowitz
*/

#include <iostream>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <string>
#include <sstream>
#include <assert.h>
#include "commands.pb.h"

class  LoggedException
{
public:
	LoggedException(std::string s) : msg(s), level(pas::LogLevel::VERBOSE) {}
	LoggedException(std::string s, pas::LogLevel ll) : msg(s), level(ll) {}
	
	std::string Msg() { return msg; }
	pas::LogLevel Level() { return level; }
	
private:
	std::string msg;
	pas::LogLevel level;
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

