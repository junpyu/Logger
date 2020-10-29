cmake_minimum_required(VERSION 2.6)

project(Logger C CXX)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()

cmake_minimum_required(VERSION 3.2)
PROJECT(Logger)

add_executable(main main.cc Logger.cc)
target_link_libraries(main)