cmake_minimum_required(VERSION 3.2.2)

project(trex)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(MSVC)
    set(DEMO_WARNING_FLAGS "") # the default is fine here
else()
    set(DEMO_WARNING_FLAGS "-Wall -Wextra -Wno-unused-variable")
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${DEMO_WARNING_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${DEMO_WARNING_FLAGS}")

add_executable(trex trex.hpp test.cpp)
