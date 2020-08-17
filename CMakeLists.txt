cmake_minimum_required(VERSION 3.14)
project(Balls VERSION 2.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

add_subdirectory(XamlCpp)
add_subdirectory(Balls)
