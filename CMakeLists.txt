cmake_minimum_required(VERSION 3.16)
project(TimetableSuite LANGUAGES C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(conflict_check src/conflict_check.c)
add_executable(timetable_backend src/main.cpp)
add_executable(timetable_formatter src/timetable_formatter.cpp)

add_executable(timetable_gui
    src/gui/gui_main.cpp
    src/gui/MainWindow.cpp
    src/gui/MainWindow.h
    src/gui/Models.h
)
target_link_libraries(timetable_gui PRIVATE Qt6::Widgets)
