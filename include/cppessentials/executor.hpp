

#ifndef CPPE_EXECUTOR_H_
#define CPPE_EXECUTOR_H_

#include <cppessentials/config.h>

#if !defined(CPPES_HAVE_CXX0X)
#warning "executor is NOT used because no C++11 support detected"
#else

#include <queue>
#include <deque>
#include <thread>
#include <mutex>
#include <memory>
#include <functional>
#include <condition_variable>
#include <chrono>

using namespace std;

struct RunItem {
	chrono::steady_clock::duration mDelay;
	chrono::time_point<chrono::steady_clock> mCreateTime;
	bool mRepeat;
	

	RunItem(chrono::steady_clock::duration delay, bool repeat) 
	: mCreateTime(chrono::steady_clock::now())
	, mDelay(delay)
	, mRepeat(repeat) {}

	RunItem()
	: RunItem(chrono::seconds(0), false) {}	

	virtual ~RunItem() {}

	virtual void run() = 0;

	chrono::time_point<chrono::steady_clock> getRunTimepoint() {
		return mCreateTime + mDelay;
	}

	void setRepeat(bool repeat) {
		mRepeat = repeat;
	}

};

template<typename F>
struct RunItemT : public RunItem {
	F function;

	RunItemT(const F& f)
	: RunItem()
	, function(f) {}

	RunItemT(const F& f, chrono::steady_clock::duration delay, bool repeat)
	: RunItem(delay, repeat)
	, function(f) {}

	void run() {
		function();
	}

	
};

struct RunItemCompare {
	bool operator()(shared_ptr<RunItem> a, shared_ptr<RunItem> b){
		return a->getRunTimepoint() < b->getRunTimepoint();
	}
};

class ThreadExecutor{
public:
	ThreadExecutor(int numThreads)
	{
		this->running = false;
		this->numThreads = numThreads;
	}
	~ThreadExecutor(){}

	template<typename T>
	void post(T task) {

		lock_guard<mutex> lock(m);
		nodelayoperations.push(new RunItemT<T>(task));
		s.notify_one();
	}

	template<typename T>
	void post(T task, chrono::steady_clock::duration delay) {
		lock_guard<mutex> lock(m);
		delayoperations.push(new RunItemT<T>(task, delay, false));
		s.notify_one();
	}

	template<typename T>
	shared_ptr<RunItem> scheduleFixedInterval(T task, chrono::steady_clock::duration delay) {
		shared_ptr<RunItem> retval(new RunItemT<T>(task, delay, true));
		lock_guard<mutex> lock(m);
		delayoperations.push(retval);
		s.notify_one();
		return retval;
	}

	void run() {

		lock_guard<mutex> lock(m);

		if(running){
			return;
		}	
		running = true;


		for(int i=0;i<numThreads;i++){
			threads.push_back(thread([this](){workerfun();}));
		}

	}

	bool isRunning() {
		lock_guard<mutex> lock(m);
		return running;
	}

	void stop() {
		{
			lock_guard<mutex> lock(m);
			running = false;
			s.notify_all();
		}
		while(threads.size() > 0){
			threads.front().join();
			threads.pop_front();
		}
	}

protected:
	virtual void workerfun() {
		unique_lock<mutex> lock(m);
		while(running){
			s.wait(lock, [this]{return !nodelayoperations.empty() || !delayoperations.empty() || !running;});

			if(!nodelayoperations.empty()){
				unique_ptr<RunItem> runItem(nodelayoperations.front());
				nodelayoperations.pop();
				lock.unlock();
				runItem->run();
				lock.lock();
			} else if(!delayoperations.empty()){

				shared_ptr<RunItem> theItem = delayoperations.top();
				const auto runtime = theItem->getRunTimepoint();

				if(chrono::steady_clock::now() >= runtime){
					delayoperations.pop();
					lock.unlock();
					theItem->run();
					lock.lock();

					if(theItem->mRepeat){
						theItem->mCreateTime = chrono::steady_clock::now();
						delayoperations.push(theItem);
					}

				} else {
					s.wait_until(lock, runtime);
				}
			}
		}
	}

private:
	queue<RunItem*> nodelayoperations;
	priority_queue<shared_ptr<RunItem>, deque<shared_ptr<RunItem>>, RunItemCompare> delayoperations;
	deque<thread> threads;
	bool running;
	int numThreads;
	mutex m;
	condition_variable s;
};

#endif /* !defined(CPPES_HAVE_CXX0X) */

#endif /* CPPE_EXECUTOR_H_ */
