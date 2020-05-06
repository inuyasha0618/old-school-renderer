cmake_minimum_required (VERSION 2.8)
set(CMAKE_C_FLAGS "-g -Wall  -I C:\\mingw\\include -L C:\\mingw\\lib")#  
set(CMAKE_C_COMPILER "gcc")
set(CMAKE_CXX_COMPILER "g++")
set(CMAKE_CXX_FLAGS "-g -Wall  -I C:\\mingw\\include -L C:\\mingw\\lib")  

project (oldSchoolRenderer)

add_executable(oldSchoolRenderer main.cpp tgaimage.cpp model.cpp)