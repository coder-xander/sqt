#include "sequence_queue_task.hpp"
#include <iostream>
#include <string>
double fibonacci(double n) {
    if (n <= 1) {
        return n;
    }
    return fibonacci(n - 1) + fibonacci(n - 2);
}



int main()
{
#if 1
    //记录开始时间
    auto start = std::chrono::steady_clock::now();
    //初始化
    SequenceQueueTask<double, double> sq_;
    //配置（可选）
    //设置任务
    sq_.setProcessingFunction([](auto indata)
        {
            return fibonacci(indata);
        });
    double functionRunOnceTimeMs = sq_.testRunOnceTimeMs(30);

    //给数据
    for (double indata = 0; indata < 999; ++indata)
    {
        sq_.in(indata);
    }
    //sleep 100ms
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    while (sq_.getSortResData().size() != 999)
    {

    }
    //打印时间差
    auto end = std::chrono::steady_clock::now();
    std::cout << "time sq: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;


#else
    std::vector<double> aa;
    auto  start = std::chrono::steady_clock::now();
    for (int indata = 0; indata < 999; ++indata)
    {
        aa.push_back(tan(sin(cos(tan(sin(sin(indata)))))));
    }
    //打印运行时间
    //sleep 100ms
    // std::this_thread::sleep_for(std::chrono::milliseconds(100));
    auto end = std::chrono::steady_clock::now();
    std::cout << "time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;
#endif

    system("pause");
}

