#include "cpu_loader.h"
#include <cmath>
#include <android/log.h>
#include <unistd.h>
#include <atomic>

#define LOG_TAG "CPULoader"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// 使用全局静态变量确保loader实例在整个应用生命周期内存在
static CpuLoader* g_loader = nullptr;

CpuLoader::CpuLoader() : running(false), numCores(1) {
    LOGI("CpuLoader constructor");
}

CpuLoader::~CpuLoader() {
    LOGI("CpuLoader destructor");
    stop();
}

void CpuLoader::workerFunction() {
    LOGI("Worker thread started");
    const int checkInterval = 10000; // 每10000次迭代检查一次停止标志

    while (running) {
        // 执行一些计算密集型操作
        for (int i = 0; i < 1000000; i++) {
            if (i % checkInterval == 0 && !running) {
                LOGI("Worker thread stopping");
                return; // 直接返回，确保线程立即停止
            }
            double result = std::sin(i) * std::cos(i);
            (void)result; // 防止编译器优化
        }
        // 添加短暂休眠，让线程有机会检查running标志
        usleep(1000);
    }
    LOGI("Worker thread ended");
}

void CpuLoader::start(int cores) {
    if (!running) {
        LOGI("Starting CPU load with %d cores", cores);
        running = true;
        numCores = cores;
        workerThreads.clear();

        for (int i = 0; i < numCores; i++) {
            workerThreads.emplace_back(&CpuLoader::workerFunction, this);
        }
    }
}

void CpuLoader::stop() {
    if (running) {
        LOGI("Stopping CPU load");
        running = false;

        // 等待所有线程结束
        for (auto& thread : workerThreads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        workerThreads.clear();

        // 确保所有线程都已经停止
        usleep(100000); // 等待100ms确保所有线程都已经停止
        LOGI("CPU load stopped");
    }
}

// JNI接口
extern "C" {

JNIEXPORT void JNICALL
Java_com_example_cpuloader_MainActivity_startCpuLoad(JNIEnv *env, jobject thiz, jint cores) {
    if (g_loader == nullptr) {
        g_loader = new CpuLoader();
    }
    g_loader->start(cores);
}

JNIEXPORT void JNICALL
Java_com_example_cpuloader_MainActivity_stopCpuLoad(JNIEnv *env, jobject thiz) {
    if (g_loader != nullptr) {
        g_loader->stop();
    }
}

}
