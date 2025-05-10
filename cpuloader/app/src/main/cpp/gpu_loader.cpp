#include "gpu_loader.h"
#include <android/log.h>
#include <unistd.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <EGL/egl.h>
#define GL_GLEXT_PROTOTYPES
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#include <vector>
#include <string>

// 简单vec3实现
struct vec3 {
    float x, y, z;
    vec3(float v) : x(v), y(v), z(v) {}
    vec3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
    vec3 operator+(const vec3& v) const { return vec3(x+v.x, y+v.y, z+v.z); }
    vec3 operator*(float f) const { return vec3(x*f, y*f, z*f); }
    vec3 normalize() const {
        float len = sqrt(x*x + y*y + z*z);
        return vec3(x/len, y/len, z/len);
    }
    float dot(const vec3& v) const { return x*v.x + y*v.y + z*v.z; }
};

static vec3 ro(0.0f, 0.0f, -5.0f);
static vec3 rd(0.0f, 0.0f, 1.0f);

#define LOG_TAG "GPULoader"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

static GpuLoader* g_gpuLoader = nullptr;

GpuLoader::GpuLoader() : running(false), window(nullptr), program(0) {
    LOGI("GpuLoader constructor");
}

GpuLoader::~GpuLoader() {
    LOGI("GpuLoader destructor");
    stop();
}

GLuint GpuLoader::loadShader(GLenum type, const char* shaderSrc) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &shaderSrc, NULL);
    glCompileShader(shader);
    
    GLint compiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        GLint infoLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen > 1) {
            char* infoLog = new char[infoLen];
            glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
            LOGI("Error compiling shader: %s", infoLog);
            delete[] infoLog;
        }
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

void GpuLoader::workerFunction() {
    LOGI("GPU worker thread started");
    
    // 初始化EGL和GLES上下文
    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(display, 0, 0);
    
    EGLint attribs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_BLUE_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_RED_SIZE, 8,
        EGL_NONE
    };
    
    EGLConfig config;
    EGLint numConfigs;
    eglChooseConfig(display, attribs, &config, 1, &numConfigs);
    
    EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    EGLSurface surface = eglCreateWindowSurface(display, config, window, NULL);
    EGLContext context = eglCreateContext(display, config, NULL, contextAttribs);
    eglMakeCurrent(display, surface, surface, context);
    
    // 获取窗口尺寸
    int width = ANativeWindow_getWidth(window);
    int height = ANativeWindow_getHeight(window);
    glViewport(0, 0, width, height);
    
    // 创建全屏四边形VAO
    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    
    float vertices[] = {
        -1.0f,  1.0f, 0.0f,
        -1.0f, -1.0f, 0.0f,
         1.0f, -1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f,
         1.0f, -1.0f, 0.0f,
         1.0f,  1.0f, 0.0f
    };
    
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    
    // 加载体积云着色器
    const char* vertShader = R"(
        #version 300 es
        layout(location = 0) in vec3 aPos;
        void main() {
            gl_Position = vec4(aPos, 1.0);
        }
    )";
    
    const char* fragShader = R"(
        #version 300 es
        precision highp float;
        out vec4 FragColor;
        
        float noise(vec3 p) {
            // 简化噪声实现
            return fract(sin(dot(p, vec3(12.9898,78.233,45.5432))) * 43758.5453);
        }
        
        void main() {
            vec3 ro = vec3(0.0, 0.0, -5.0);
            vec3 rd = normalize(vec3(gl_FragCoord.xy/vec2(800.0,600.0)-0.5, 1.0));
            
            float t = 0.0;
            for(int i=0; i<100; i++) {
                vec3 p = ro + rd*t;
                float d = noise(p*0.2)*2.0 - 1.0;
                if(d < 0.01) {
                    FragColor = vec4(1.0);
                    return;
                }
                t += d;
                if(t > 20.0) break;
            }
            FragColor = vec4(0.0);
        }
    )";
    
    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, vertShader);
    GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, fragShader);
    program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    
    // 渲染循环
    while (running) {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        glUseProgram(program);
        glBindVertexArray(vao);
        
        // 增加GPU密集型计算
        for (int i = 0; i < 100; i++) {
            // 复杂的光线步进计算
            for (int j = 0; j < 1000; j++) {
                vec3 p = ro + rd * (float(j) / 1000.0f);
                // 模拟噪声计算
                float n = sin(p.x) * cos(p.y) * sin(p.z);
                // 模拟光照计算
                vec3 lightDir(0.5f, 1.0f, 0.7f);
                float light = p.normalize().dot(lightDir.normalize());
            }
            
            // 模拟多遍渲染
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
        
        eglSwapBuffers(display, surface);
        usleep(16000); // ~60 FPS
    }
    
    // 清理资源
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteProgram(program);
    eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(display, context);
    eglDestroySurface(display, surface);
    eglTerminate(display);
    
    ANativeWindow_release(window);
    window = nullptr;
    
    LOGI("GPU worker thread ended");
}

void GpuLoader::start(ANativeWindow* win) {
    if (!running) {
        LOGI("Starting GPU load");
        window = win;
        running = true;
        workerThread = std::thread(&GpuLoader::workerFunction, this);
    }
}

void GpuLoader::stop() {
    if (running) {
        LOGI("Stopping GPU load");
        running = false;
        if (workerThread.joinable()) {
            workerThread.join();
        }
        LOGI("GPU load stopped");
    }
}

// JNI接口
extern "C" {

JNIEXPORT void JNICALL
Java_com_example_cpuloader_MainActivity_startGpuLoad(JNIEnv *env, jobject thiz, jobject surface) {
    if (g_gpuLoader == nullptr) {
        g_gpuLoader = new GpuLoader();
    }
    ANativeWindow* window = ANativeWindow_fromSurface(env, surface);
    g_gpuLoader->start(window);
}

JNIEXPORT void JNICALL
Java_com_example_cpuloader_MainActivity_stopGpuLoad(JNIEnv *env, jobject thiz) {
    if (g_gpuLoader != nullptr) {
        g_gpuLoader->stop();
    }
}

}
