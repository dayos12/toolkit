#ifndef CPU_LOADER_H
#define CPU_LOADER_H

#include <jni.h>
#include <atomic>
#include <thread>
#include <vector>

class CpuLoader {
private:
    std::atomic<bool> running;
    std::vector<std::thread> workerThreads;
    int numCores;

    void workerFunction();

public:
    CpuLoader();
    ~CpuLoader();
    void start(int cores);
    void stop();
};

#endif // CPU_LOADER_H
