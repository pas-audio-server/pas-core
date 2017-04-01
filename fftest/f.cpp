#include <thread>

using namespace std;

int foo()
{
}

int main()
{
	thread t(foo);
	return 0;
}
