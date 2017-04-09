#include "logger.hpp"

using namespace std;

Logger log("/tmp/log.txt");

int main()
{
	try
	{
		LOG(log, nullptr);
		LOG(log, "test message");
		throw LOG(log, string("comes from a throw"));
	}
	catch (LoggedException e)
	{
		cerr << e.Msg() << endl;
	}
}
