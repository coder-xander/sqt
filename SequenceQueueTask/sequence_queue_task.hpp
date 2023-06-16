#pragma once
#include <functional>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <vector>
#include "task.hpp"
#include "thread_safe_queue.hpp"

// template <class T>
// using I = decltype(*std::begin(std::declval<T>()));
// concept  Container = requires
// {
//
// 	std::is_same_v< std::remove_cv_t<std::remove_reference_t<T>>, std::vector<I>>;
//
//
// };
//


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
		taskNum_= std::thread::hardware_concurrency();
		if (taskNum_ == 0) {
			std::cout << "Unable to determine the number of hardware threads." << std::endl;
			taskNum_ = 2; // default to 2 threads
		}
		std::cout << "Number of hardware threads: " << taskNum_ << std::endl;
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
	OUT out()
	{
		OUT  va;
		sortResData_.wait_and_pop(va);
		return va;
	}

public:
	void run(std::function<OUT(IN)> processingFunction)
	{
		processingFunction_ = processingFunction;
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
	int  taskNum_{ 1 };
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
	/**
	 * \brief
	 * \tparam Func 测试自定义的函数运行耗时
	 * \param times
	 * \param func
	 * \return
	 */
	template <typename Func>
	double testDiyFuncRunTimeConsuming(size_t times, Func func) {
		auto start = std::chrono::high_resolution_clock::now();
		for (size_t i = 0; i < times; ++i) {
			func();
		}
		auto end = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> diff = end - start;
		return diff.count();
	}

	template<typename Func, typename... Para>
	double testDiyFuncRunTimeConsuming(size_t times, Func func, Para... paras)
	{
		// 记录开始时间
		auto start = std::chrono::high_resolution_clock::now();

		for (int i = 0; i < times; ++i)
		{
			// 调用传入的函数并扩展参数包
			auto wrapper = [&]() { return std::invoke(func, paras...); };
			wrapper();
		}

		// 记录结束时间
		auto end = std::chrono::high_resolution_clock::now();

		// 计算耗时
		std::chrono::duration<double, std::milli> elapsed = end - start;

		// 返回耗时，单位为毫秒
		return elapsed.count();
	}

	double testRunTimeConsuming(size_t times, IN  args)
	{
		// 记录开始时间
		auto start = std::chrono::high_resolution_clock::now();

		for (int i = 0; i < times; ++i)
		{
			// 调用传入的函数
			std::invoke(std::forward<std::function<OUT(IN)>>(processingFunction_), std::forward<IN>(args));
		}

		// 记录结束时间
		auto end = std::chrono::high_resolution_clock::now();

		// 计算耗时
		std::chrono::duration<double, std::milli> elapsed = end - start;

		// 返回耗时，单位为毫秒
		return elapsed.count();


	}

	bool isfinished()
	{
		return index == sortResData_.size();
	}

	std::vector<OUT> waitAndGetData()
	{
		std::vector<OUT> res;
		while (res.size() != index)
		{
			OUT v;
			sortResData_.wait_and_pop(v);
			res.push_back(v);
		}


		return res;
	}

	// std::list<OUT> waitAndGetData()
	// {
	// 	return {};
	// }


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