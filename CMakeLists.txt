cmake_minimum_required(VERSION 3.20)

project(MoyuSimulatorFinal VERSION 1.0.0 LANGUAGES CXX RC)

if(NOT WIN32)
    message(FATAL_ERROR "This project builds a Windows desktop application and requires a Windows toolchain.")
endif()

add_executable(MoyuSimulatorFinal WIN32
    src/main.cpp
    src/resources.rc
)

target_compile_features(MoyuSimulatorFinal PRIVATE cxx_std_17)
target_compile_definitions(MoyuSimulatorFinal PRIVATE
    UNICODE
    _UNICODE
    _WIN32_WINNT=0x0A00
    WINVER=0x0A00
)
target_link_libraries(MoyuSimulatorFinal PRIVATE user32 gdi32)

if(MINGW)
    target_link_options(MoyuSimulatorFinal PRIVATE
        -municode
        -static
        -static-libgcc
        -static-libstdc++
        $<$<CONFIG:Release>:-s>
    )
endif()

set_target_properties(MoyuSimulatorFinal PROPERTIES
    OUTPUT_NAME "摸鱼模拟器最终版"
)
