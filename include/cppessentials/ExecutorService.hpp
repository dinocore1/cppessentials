

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


class ExecutorService {

	struct RunItem {
		friend ExecutorService;
	private:
		chrono::time_point<chrono::steady_clock> mCreateTime;
		std::function<void()> mRun;
		chrono::steady_clock::duration mDelay;
		bool mRepeat;
		bool mCanceled;

	public:
		RunItem(std::function<void()> run,
				chrono::steady_clock::duration delay = chrono::steady_clock::duration(0),
				bool repeat = false)
		: mCreateTime(chrono::steady_clock::now())
		, mRun(run)
		, mDelay(delay)
		, mRepeat(repeat)
		, mCanceled(false)
		{}

		chrono::time_point<chrono::steady_clock> runAt() {
			return mCreateTime + mDelay;
		}

		void cancel() {
			mCanceled = true;
		}

		void operator()() {
			mRun();
		}

	};

	struct RunItemCompare {
		bool operator()(shared_ptr<RunItem> a, shared_ptr<RunItem> b){
			bool retval = a->runAt() > b->runAt();
			{
				chrono::duration<double> arunat = chrono::duration_cast<chrono::duration<double>>(a->runAt().time_since_epoch());
				chrono::duration<double> brunat = chrono::duration_cast<chrono::duration<double>>(b->runAt().time_since_epoch());
				printf("comparing a: %f [0x%lx] and b: %f [0x%lx] return %d\n", arunat.count(), (unsigned long)a.get(), brunat.count(), (unsigned long)b.get(), retval);
			}
			return retval;
		}
	};

private:
	bool running;
	int numThreads;
	std::priority_queue<std::shared_ptr<RunItem>, std::deque<std::shared_ptr<RunItem>>, RunItemCompare> delayworkqueue;
	std::deque<std::shared_ptr<RunItem>> workqueue;
	std::vector<std::thread> workerThreads;
	std::mutex m;
	std::condition_variable cv;

public:
	ExecutorService(int numThreads = 1)
	: running(false)
	, numThreads(numThreads) {
	}

	~ExecutorService(){
		stop();
	}

	shared_ptr<RunItem> post(std::function<void()> task) {
		shared_ptr<RunItem> retval(new RunItem(task));
		std::lock_guard<std::mutex> lock(m);
		workqueue.push_back(retval);
		cv.notify_one();
		return retval;
	}

	shared_ptr<RunItem> post(std::function<void()> task, std::chrono::steady_clock::duration delay) {
		shared_ptr<RunItem> retval(new RunItem(task, delay, false));
		lock_guard<mutex> lock(m);
		if(delay == std::chrono::steady_clock::duration(0)){
			workqueue.push_back(retval);
		} else {
			delayworkqueue.push(retval);
		}
		cv.notify_one();
		return retval;
	}

	shared_ptr<RunItem> scheduleFixedInterval(std::function<void()> task, chrono::steady_clock::duration delay) {
		shared_ptr<RunItem> retval(new RunItem(task, delay, true));
		lock_guard<mutex> lock(m);
		delayworkqueue.push(retval);
		cv.notify_one();
		return retval;
	}

	void run() {
		lock_guard<mutex> lock(m);
		if(running){
			return;
		}	
		running = true;

		for(int i=0;i<numThreads;i++){
			workerThreads.push_back(thread([this](){workerfun();}));
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
			cv.notify_all();
		}
		for(unsigned int i=0;i<workerThreads.size();i++){
			workerThreads[i].join();
		}
		workerThreads.clear();
	}

protected:
	virtual void workerfun() {
		unique_lock<mutex> lock(m);
		while(running){
			cv.wait(lock, [this]{return !workqueue.empty() || !delayworkqueue.empty() || !running;});

			if(!workqueue.empty()){
				shared_ptr<RunItem> runItem(workqueue.front());
				workqueue.pop_front();
				lock.unlock();
				if(!runItem->mCanceled) {
					(*runItem.get())();
				}
				lock.lock();
			}
			if(!delayworkqueue.empty()){
				shared_ptr<RunItem> runItem = delayworkqueue.top();
				const chrono::time_point<chrono::steady_clock> runAt = runItem->runAt();
				if(chrono::steady_clock::now() >= runAt){
					delayworkqueue.pop();
					lock.unlock();
					if(!runItem->mCanceled){
						(*runItem.get())();
					}
					lock.lock();

					if(runItem->mRepeat && !runItem->mCanceled){
						runItem->mCreateTime = chrono::steady_clock::now();
						delayworkqueue.push(runItem);
					}

				} else if(workqueue.empty()) {
					cv.wait_until(lock, runAt);
				}
			}
		}
	}

};

#endif /* !defined(CPPES_HAVE_CXX0X) */

#endif /* CPPE_EXECUTOR_H_ */
