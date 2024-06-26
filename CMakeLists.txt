cmake_minimum_required(VERSION 3.12)

project(Meteorite VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(meteorite meteorite.cxx)

target_compile_definitions(meteorite PRIVATE METEORITE_VERSION="${CMAKE_PROJECT_VERSION}")