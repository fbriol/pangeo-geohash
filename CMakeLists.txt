cmake_minimum_required(VERSION 3.0)

include(CheckFunctionExists)
include(CheckCXXSourceRuns)

if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_BINARY_DIR}")
  message(FATAL_ERROR "The build directory must be different from the \
        root directory of this software.")
endif()

cmake_policy(SET CMP0048 NEW)
project(geohash LANGUAGES C CXX)

if (POLICY CMP0063)
  cmake_policy(SET CMP0063 NEW)
endif ()

if (POLICY CMP0074)
  cmake_policy(SET CMP0074 NEW)
endif ()

if (POLICY CMP0077)
  cmake_policy(SET CMP0077 NEW)
endif ()

# CMake module search path
set(
  CMAKE_MODULE_PATH
  "${CMAKE_CURRENT_SOURCE_DIR}/third_party/pybind11/tools;"
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
  "${CMAKE_MODULE_PATH}"
)

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

# By default, build type is set to release, with debugging information.
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RELWITHDEBINFO)
endif()
message("-- Build type: ${CMAKE_BUILD_TYPE}")

# The library must be built using C++17 compiler.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MACOSX_RPATH 1)

include(CheckCXXCompilerFlag)
if(NOT WIN32)
  check_cxx_compiler_flag("-std=c++17" HAS_CPP17_FLAG)
else()
  check_cxx_compiler_flag("/std:c++17" HAS_CPP17_FLAG)
endif()
if(NOT HAS_CPP17_FLAG)
  message(FATAL_ERROR "Unsupported compiler -- requires C++17 support!")
endif()

macro(check_cxx_compiler_and_linker_flags _RESULT _CXX_FLAGS _LINKER_FLAGS)
  set(CMAKE_REQUIRED_FLAGS ${_CXX_FLAGS})
  set(CMAKE_REQUIRED_LIBRARIES ${_LINKER_FLAGS})
  set(CMAKE_REQUIRED_QUIET FALSE)
  check_cxx_source_runs("int main(int argc, char **argv) { return 0; }" ${_RESULT})
  set(CMAKE_REQUIRED_FLAGS "")
  set(CMAKE_REQUIRED_LIBRARIES "")
  unset(_RESULT)
endmacro()

macro(check_floating_point_is_iec559)
  file(WRITE "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/is_iec559.cpp"
"#include <limits>
int main() {
  return std::numeric_limits<double>::is_iec559 ? 1 : 0;
}")
  try_run(IS_IEC559
          _UNUSED
          "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}"
          "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/is_iec559.cpp")
  unset(_UNUSED)
  if (NOT IS_IEC559)
    message(FATAL_ERROR
            "'double' floating point type doesn't conform to the "
            "IEC 559 requirements.")
  endif()
endmacro()

check_floating_point_is_iec559()

# Always use libc++ on Clang
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  check_cxx_compiler_and_linker_flags(
    HAS_LIBCPP "-stdlib=libc++" "-stdlib=libc++")
  if (HAS_LIBCPP)
    string(APPEND CMAKE_CXX_FLAGS " -stdlib=libc++")
    string(APPEND CMAKE_EXE_LINKER_FLAGS " -stdlib=libc++")
    string(APPEND CMAKE_SHARED_LINKER_FLAGS " -stdlib=libc++")
    check_cxx_compiler_and_linker_flags(
      HAS_LIBCPPABI "-stdlib=libc++" "-stdlib=libc++ -lc++abi")
    if(HAS_LIBCPPABI)
      string(APPEND CMAKE_EXE_LINKER_FLAGS " -lc++abi")
      string(APPEND CMAKE_SHARED_LINKER_FLAGS " -lc++abi")
    endif()
  endif()
  check_cxx_compiler_and_linker_flags(
    HAS_SIZED_DEALLOCATION "-fsized-deallocation" "")
  if(HAS_SIZED_DEALLOCATION)
    string(APPEND CMAKE_CXX_FLAGS " -fsized-deallocation")
  endif()
endif()

if(NOT WIN32)
  if(NOT CMAKE_CXX_FLAGS MATCHES "-Wall$")
    string(APPEND CMAKE_CXX_FLAGS " -Wall")
  endif()
  if(NOT CMAKE_CXX_COMPILER MATCHES "icpc$" AND NOT CMAKE_CXX_FLAGS MATCHES "-Wpedantic$")
    string(APPEND CMAKE_CXX_FLAGS " -Wpedantic")
  endif()
endif()

CHECK_FUNCTION_EXISTS(pow POW_FUNCTION_EXISTS)
if(NOT POW_FUNCTION_EXISTS)
  unset(POW_FUNCTION_EXISTS CACHE)
  list(APPEND CMAKE_REQUIRED_LIBRARIES m)
  CHECK_FUNCTION_EXISTS(pow POW_FUNCTION_EXISTS)
  if(POW_FUNCTION_EXISTS)
    set(MATH_LIBRARY m CACHE STRING "" FORCE)
  else()
    message(FATAL_ERROR "Failed making the pow() function available")
  endif()
endif()

# Python
find_package(PythonInterp REQUIRED)
execute_process(
    COMMAND
    ${PYTHON_EXECUTABLE} -c [=[import os
import sysconfig
import sys
sys.stdout.write(os.path.dirname(sysconfig.get_config_h_filename()))
]=] OUTPUT_VARIABLE PYTHON_INCLUDE_DIR)
find_package(PythonLibs REQUIRED)

# Boost
find_package(Boost 1.63 REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

# Eigen3
find_package(Eigen3 3.3.1 REQUIRED)
include_directories(${EIGEN3_INCLUDE_DIR})

# Snappy
find_package(Snappy REQUIRED)
include_directories(${SNAPPY_INCLUDE_DIR})

# unqlite
add_library(unqlite STATIC "third_party/unqlite/unqlite.c")
target_compile_definitions(unqlite PUBLIC UNQLITE_ENABLE_THREADS)
if(NOT WIN32)
  set_target_properties(unqlite PROPERTIES COMPILE_FLAGS "-fPIC")
endif()

# core module
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third_party/pybind11)
file(GLOB_RECURSE SOURCES "src/*.cpp")
include_directories("src/geohash/core/include")
pybind11_add_module(core ${SOURCES})
target_link_libraries(core PUBLIC unqlite ${SNAPPY_LIBRARIES})
