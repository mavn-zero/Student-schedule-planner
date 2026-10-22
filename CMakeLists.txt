cmake_minimum_required(VERSION 3.10)
project(TimetableScheduler)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 99)

include_directories(src)

add_executable(timetable_backend
    src/scheduler.cpp
    src/graph.c
)
