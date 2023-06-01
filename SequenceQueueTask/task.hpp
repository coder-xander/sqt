#pragma once
#include "sequence_queue_task.hpp"
#include "thread_safe_queue.hpp"
template<class IN, class OUT>
class Task
{
	using PairDataIn = std::pair<size_t, IN>;
	using PairDataOut = std::pair<size_t, OUT>;
public:
	Task(ThreadSafeQueue<PairDataIn>* unSortProcessedData, std::function<OUT(IN)> processingFunction)
	{
		processingFunction_ = processingFunction;
		unSortProcessedData_ = unSortProcessedData;
		taskData_ = new ThreadSafeQueue<PairDataIn>();
		workThread_ = std::thread([this]()
			{
				while (!isThreadEnded_)
				{

					PairDataIn pairData;
					taskData_->wait_and_pop(pairData);
					OUT out = processingFunction_(pairData.second);
					PairDataOut resPairdata = std::make_pair<size_t, OUT>(std::move(pairData.first), std::move(out));
					unSortProcessedData_->push(resPairdata);
				}
			});
	}
	~Task()
	{

	}
public:
	std::function<OUT(IN)> processingFunction_;
	//共享的结果数据
	ThreadSafeQueue<PairDataOut>* unSortProcessedData_;
	bool isThreadEnded_{ false };
	std::thread workThread_;
	//这个线程分到的任务数据
	ThreadSafeQueue<PairDataIn>* taskData_;
public:
	ThreadSafeQueue<PairDataIn>* getTaskData() {
		return taskData_;
	};

};


