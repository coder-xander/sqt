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
		//������Ӧ�����Ĺ����߳�
		for (int i = 0; i < taskNum_; ++i)
		{
			auto task = new Task<IN, OUT>(unSortResData_, processingFunction_);
			tasks_.push_back(task);
		}
		//����һ�����ݷַ��߳�
		ditributeThread_ = std::thread([this]
			{
				while (!isDistributeThreadEnded_)
				{
					PairDataIn pairData;
					inQueue_.wait_and_pop(pairData);
					//���䵽�������
					tasks_[distributeIndex_++ % taskNum_]->getTaskData()->push(pairData);

				}
			});
		//����һ�����������߳�
		sortThread_ = std::thread([this]
			{
				size_t expectedIndex = 0;
				std::map<size_t, OUT> tempStorage;
				while (!isSortThreadEnded_)
				{

					PairDataOut out;
					unSortResData_->wait_and_pop(out);

					// �����ǰ���ӵ�����������������������ͬ�������ݴ�
					if (out.first != expectedIndex) {
						tempStorage[out.first] = out.second;
					}
					else {
						// ��ǰ���ӵ�����������������������ͬ
						sortResData_.push(out.second);
						++expectedIndex;

						// ����ݴ���������Ƿ��з�����������������
						auto it = tempStorage.find(expectedIndex);
						while (it != tempStorage.end()) {
							sortResData_.push(it->second);
							tempStorage.erase(it);
							++expectedIndex;
							it = tempStorage.find(expectedIndex);
						}
					}
					//�ж�unsort�ﻹ��û�����ݣ����û�о�
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
 * \brief inqueue ��������In
 */
	ThreadSafeQueue<PairDataIn> inQueue_;
	/**
	 * \brief ���䡢�����߳�
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
	 * \brief �������
	 */
	ThreadSafeQueue<OUT> sortResData_;
	ThreadSafeQueue<PairDataIn>* unSortResData_;
	/**
	 * \brief ������
	 */
	std::function<OUT(IN)> processingFunction_;
public:
	double testRunOnceTimeMs(IN  args)
	{
		// ��¼��ʼʱ��
		auto start = std::chrono::high_resolution_clock::now();

		// ���ô���ĺ���
		std::invoke(std::forward<std::function<OUT(IN)>>(processingFunction_), std::forward<IN>(args));

		// ��¼����ʱ��
		auto end = std::chrono::high_resolution_clock::now();

		// �����ʱ
		std::chrono::duration<double, std::milli> elapsed = end - start;

		// ���غ�ʱ����λΪ����
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