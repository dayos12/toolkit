cmake_minimum_required(VERSION 3.4.1)

add_library(cpuloader SHARED
        cpu_loader.cpp
        cpu_loader.h
        gpu_loader.cpp
        gpu_loader.h)

# 包含NDK头文件路径
include_directories(${ANDROID_NDK}/sources/android/native_app_glue)
include_directories(${ANDROID_NDK}/sources/android/cpufeatures)

target_include_directories(cpuloader PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${ANDROID_NDK}/sources/android/native_app_glue
    ${ANDROID_NDK}/sources/android/cpufeatures
    ${ANDROID_NDK}/toolchains/llvm/prebuilt/windows-x86_64/sysroot/usr/include
    ${ANDROID_NDK}/sources/cxx-stl/llvm-libc++/include
    ${ANDROID_NDK}/sysroot/usr/include
    ${ANDROID_NDK}/sources/third_party/vulkan/src/include
    ${ANDROID_NDK}/sources/third_party/vulkan/src/common
)

find_library(log-lib log)
find_library(egl-lib EGL)
find_library(glesv2-lib GLESv2)
find_library(glesv3-lib GLESv3)
find_library(android-lib android)

target_link_libraries(cpuloader
        ${log-lib}
        ${egl-lib}
        ${glesv2-lib}
        ${glesv3-lib}
        ${android-lib})

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
