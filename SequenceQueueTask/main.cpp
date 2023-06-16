#include "sequence_queue_task.hpp"
#include <iostream>
#include <string>
double fibonacci(double n) {

    if (n <= 1) {

        return n;
    }
    return fibonacci(n - 1) + fibonacci(n - 2);
}

using Matrix = std::vector<std::vector<double>>;


int main()
{
    SequenceQueueTask<double, double > sq_;

#if 1
    //记录开始时间
    auto start = std::chrono::steady_clock::now();
    //初始化
    //配置（可选）
    //设置任务
    sq_.run([](auto indata)
        {
            return fibonacci(13);
        });

    //给数据
    for (double indata = 0; indata < 999999; ++indata)
    {
        sq_.in(indata);
    }
    //sleep 100ms
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::vector<double> outDatas;
    // for (double outData = 0; outData < 999999; ++outData)
    // {
    //     outDatas.push_back(sq_.out());
    //
    // }
    outDatas = sq_.waitAndGetData();
    //打印时间差
    auto end = std::chrono::steady_clock::now();
    std::cout << "time sq: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;

#else
    auto time2 = sq_.testDiyFuncRunTimeConsuming(999999, &fibonacci, 13);
    std::cout << "time single: " << time2 << "ms" << std::endl;
#endif
    system("pause");
}

