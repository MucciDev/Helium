cmake_minimum_required(VERSION 3.21)

# Geode v5 utilizes modern C++ (C++20/C++23)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Architecture target setup for Mac/iOS cross-compilation
if ("${CMAKE_SYSTEM_NAME}" STREQUAL "iOS" OR IOS)
    set(CMAKE_OSX_ARCHITECTURES "arm64")
else()
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
endif()

set(CMAKE_CXX_VISIBILITY_PRESET hidden)

project(Helium VERSION 1.0.0)

# Gather all your source files
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)

# Create the mod library
add_library(${PROJECT_NAME} SHARED ${SOURCES})

# THE CRITICAL PART: Properly hook into the Geode CLI's pre-compiled cache
if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

# Let Geode handle the final bindings, resources, and hooks
setup_geode_mod(${PROJECT_NAME})