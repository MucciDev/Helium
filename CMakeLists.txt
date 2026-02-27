cmake_minimum_required(VERSION 3.21)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Hides internal symbols, shrinking the final binary size
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

project(Helium VERSION 2.2.0)

# Gather all source files
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)
add_library(${PROJECT_NAME} SHARED ${SOURCES})

# Hook into the Geode CLI
if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable.")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)
setup_geode_mod(${PROJECT_NAME})

# ==========================================
# COMPILER OPTIMIZATIONS
# ==========================================
if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    message(STATUS "[Helium] Injecting Windows/MSVC Performance Flags...")
    
    # /O2: Maximize Execution Speed
    # /Ob2: Aggressive Inlining (Safe level, no LTO required)
    # /Oi: Hardware Intrinsics (Bypass standard C library for raw math)
    # /Ot: Favor Fast Code
    # /fp:fast: Optimize floating point operations for speed
    # /arch:SSE2: Safe, universally supported matrix math acceleration (Replaced AVX2)
    # /Zc:inline: Removes unreferenced functions from translation units
    target_compile_options(${PROJECT_NAME} PRIVATE 
        /O2 /Ob2 /Oi /Ot /fp:fast /arch:SSE2 /Zc:inline
    )
    
    # Keep identical function folding to reduce file size safely
    target_link_options(${PROJECT_NAME} PRIVATE /OPT:REF /OPT:ICF)
endif()
