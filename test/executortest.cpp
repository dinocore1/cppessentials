
#include <cppessentials/executor.hpp>

ThreadExecutor executor(1);

int main(int argv, char* argc[])
{

	bool itRan = false;

	executor.post([&itRan](){
		itRan = true;
	});

	executor.run();

	std::this_thread::sleep_for(std::chrono::milliseconds(10000));

	executor.stop();

	return itRan ? 0 : -1;
}
