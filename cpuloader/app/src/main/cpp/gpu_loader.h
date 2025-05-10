#ifndef GPU_LOADER_H
#define GPU_LOADER_H

#include <jni.h>
#include <atomic>
#include <thread>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <android/native_window.h>

class GpuLoader {
private:
    std::atomic<bool> running;
    std::thread workerThread;
    ANativeWindow* window;
    GLuint program;

    void workerFunction();
    GLuint loadShader(GLenum type, const char* shaderSrc);

public:
    GpuLoader();
    ~GpuLoader();
    void start(ANativeWindow* win);
    void stop();
};

#endif // GPU_LOADER_H
