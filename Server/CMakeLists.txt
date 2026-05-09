cmake_minimum_required(VERSION 3.15)
project(Server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(Server main.cpp)

target_include_directories(Server PRIVATE "/home/roflane/Libs/asio-1.36.0/include/")
