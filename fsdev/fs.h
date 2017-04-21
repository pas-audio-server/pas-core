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

#include "track.hpp"
#include "logger.hpp"

struct DIRENT
{
	unsigned int type;
	int me;
	int up;
	std::string name;
	std::string fullpath;
};

void DoTheDB(std::string & nspace, std::vector<DIRENT> & t, Logger & _log_);
void Discover(std::string path, const std::vector<std::string> & allowable_extensions, std::vector<DIRENT> & t, int parent, int & c, bool is_root, Logger & _log_);

// There's a buffer declared for reading the output of ffprobe.
// Currently used only in db.cpp.

extern const int BSIZE;