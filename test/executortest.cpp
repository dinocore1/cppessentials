
#include <cppessentials/ExecutorService.hpp>

ExecutorService executor(1);
int order[10];
int orderIndex;

void recursivePost() {
	executor.post([](){
		//printf("doing it\n");
		recursivePost();
	});
}

void logOrder(int num) {
	order[orderIndex++] = num;
}

int main(int argv, char* argc[])
{
	orderIndex = 0;
	int fixedIntervalRun = 0;

	executor.post([](){
		logOrder(2);

		executor.post([](){
			logOrder(3);
		}, chrono::milliseconds(500));

		executor.post([](){
			logOrder(4);
		}, chrono::milliseconds(600));

		executor.post([](){
			recursivePost();
		});


	}, chrono::milliseconds(500));

	executor.post([](){
		logOrder(1);
	}, chrono::milliseconds(300));

	executor.post([](){
		printf("1\n");
		logOrder(0);
	});

	executor.scheduleFixedInterval([&fixedIntervalRun](){
		fixedIntervalRun++;
		printf("fixed\n");
	}, chrono::milliseconds(1000));

	executor.run();

	std::this_thread::sleep_for(std::chrono::milliseconds(10000));

	executor.stop();

	bool success = true;
	for(int i=0;i<4;i++){
		if(order[i] != i){
			success = false;
			break;
		}
	}

	if(success && fixedIntervalRun > 5) {
		printf("success\n");
		return 0;
	} else {
		printf("error");
		return -1;
	}

}
