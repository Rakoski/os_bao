cmake_minimum_required(VERSION 3.30)
project(os_bao2)

find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIR})

set(MYLIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../my-lib")
include_directories(${MYLIB_PATH}/include)

set(CMAKE_CXX_STANDARD 20)

add_executable(
        os_bao2
        main.cpp
        lib.cpp
        lib.h
        config.cpp
        config.h
        os/os-lib.h
        os/os.cpp
        os/os-lib.h
        os/process.cpp
        os/process.h
        os/os.h
        os/memory_manager.cpp
        os/memory_manager.h
        arch/arch.cpp
        arch/arch.h
        arch/computer.h
        arch/cpu.cpp
        arch/cpu.h
        arch/disk.h
        arch/disk.cpp
        arch/device.cpp
        arch/device.h
        arch/memory.cpp
        arch/memory.h
        arch/terminal.cpp
        arch/terminal.h
        arch/timer.cpp
        arch/timer.h
)
