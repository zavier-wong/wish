cmake_minimum_required(VERSION 3.15)
project(wish)

set(CMAKE_CXX_STANDARD 14)

set(INC_DIR "/usr/include")
set(LINK_DIR "/usr/lib")
include_directories(${INC_DIR})
link_directories(${LINK_DIR})
add_executable(wish main.cpp Wish.cpp Wish.h Jobs.cpp Jobs.h)
