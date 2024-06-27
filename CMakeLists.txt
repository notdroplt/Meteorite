cmake_minimum_required(VERSION 3.12)

project(Meteorite VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(Supernova)

add_executable(meteorite meteorite.cxx meteorite_core.cxx)
target_link_libraries(meteorite PRIVATE supernova-iface)
target_compile_definitions(meteorite PRIVATE METEORITE_VERSION="${CMAKE_PROJECT_VERSION}")

install(TARGETS meteorite RUNTIME)

set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_NAME "meteorite")
set(CPACK_PACKAGE_VENDOR "notdroplt")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Meteorite - Supernova assembler")
include(CPack)