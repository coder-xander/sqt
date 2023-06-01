#pragma once
#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include "task.hpp"
#include "thread_safe_queue.hpp"

template<class IN, class OUT>
class SequenceQueueTask
{
public:
	using PairDataIn = std::pair<size_t, IN>;
	using PairDataOut = std::pair<size_t, OUT>;
	size_t index{ 0 };
	size_t distributeIndex_{ 0 };
	SequenceQueueTask() {
		unSortResData_ = new ThreadSafeQueue<PairDataOut>();
	}
	~SequenceQueueTask()
	{
		isDistributeThreadEnded_ = true;
		for (auto e : tasks_)
		{
			e->isThreadEnded_ = true;
			e->workThread_.join();
		}
		ditributeThread_.join();

	}
	void in(IN data)
	{
		PairDataIn pairData = std::make_pair<size_t, IN>(std::move(index), std::move(data));
		index++;
		inQueue_.push(pairData);
	}
	// OUT out()
	// {
	// 	if
	// }
	void run()
	{
		//开启对应数量的工作线程
		for (int i = 0; i < taskNum_; ++i)
		{
			auto task = new Task<IN, OUT>(unSortResData_, processingFunction_);
			tasks_.push_back(task);
		}
		//开启一个数据分发线程
		ditributeThread_ = std::thread([this]
			{
				while (!isDistributeThreadEnded_)
				{
					PairDataIn pairData;
					inQueue_.wait_and_pop(pairData);
					//分配到任务队列
					tasks_[distributeIndex_++ % taskNum_]->getTaskData()->push(pairData);

				}
			});
		//开启一个数据排序线程
		sortThread_ = std::thread([this]
			{
				size_t expectedIndex = 0;
				std::map<size_t, OUT> tempStorage;
				while (!isSortThreadEnded_)
				{

					PairDataOut out;
					unSortResData_->wait_and_pop(out);

					// 如果当前出队的数据索引与期望的索引不同，则先暂存
					if (out.first != expectedIndex) {
						tempStorage[out.first] = out.second;
					}
					else {
						// 当前出队的数据索引与期望的索引相同
						sortResData_.push(out.second);
						++expectedIndex;

						// 检查暂存的数据中是否有符合期望索引的数据
						auto it = tempStorage.find(expectedIndex);
						while (it != tempStorage.end()) {
							sortResData_.push(it->second);
							tempStorage.erase(it);
							++expectedIndex;
							it = tempStorage.find(expectedIndex);
						}
					}
					//判断unsort里还有没有数据，如果没有就
					if (unSortResData_->empty())
					{
						lastFinishedTimePoint_ = std::chrono::steady_clock::now();
					}

				}
			});

	}
public:
	void setProcessingFunction(std::function<OUT(IN)> processingFunction)
	{
		processingFunction_ = processingFunction;
		run();
	}
private:
	std::chrono::time_point<std::chrono::steady_clock> lastFinishedTimePoint_;
	/**
 * \brief inqueue 数据输入In
 */
	ThreadSafeQueue<PairDataIn> inQueue_;
	/**
	 * \brief 分配、排序线程
	 */
	std::thread ditributeThread_;
	bool isDistributeThreadEnded_{ false };

	std::thread sortThread_;
	bool isSortThreadEnded_{ false };


	/**
	 * \brief
	 */
	int  taskNum_{ 10 };
	std::vector<Task<IN, OUT>*> tasks_;

	/**
	 * \brief 结果数据
	 */
	ThreadSafeQueue<OUT> sortResData_;
	ThreadSafeQueue<PairDataIn>* unSortResData_;
	/**
	 * \brief 处理函数
	 */
	std::function<OUT(IN)> processingFunction_;
public:
	double testRunOnceTimeMs(IN  args)
	{
		// 记录开始时间
		auto start = std::chrono::high_resolution_clock::now();

		// 调用传入的函数
		std::invoke(std::forward<std::function<OUT(IN)>>(processingFunction_), std::forward<IN>(args));

		// 记录结束时间
		auto end = std::chrono::high_resolution_clock::now();

		// 计算耗时
		std::chrono::duration<double, std::milli> elapsed = end - start;

		// 返回耗时，单位为毫秒
		return elapsed.count();


	}
	auto& getLastFinishedTimePoint()
	{
		return lastFinishedTimePoint_;
	}
	auto* getUnSortResData()
	{
		return unSortResData_;
	};
	auto& getSortResData()
	{
		return sortResData_;
	}
};