cmake_minimum_required(VERSION 2.8)
project (gauss)

set(gauss_MAJOR_VERSION 0)
set(gauss_MINOR_VERSION 2)
set(gauss_PATCH_VERSION 0)
set(gauss_VERSION
  ${gauss_MAJOR_VERSION}.${gauss_MINOR_VERSION}.${gauss_PATCH_VERSION})

if(WIN32)
    set(CMAKE_SHARED_LIBRARY_PREFIX "php_")
else()
    set(CMAKE_SHARED_LIBRARY_PREFIX "")
endif()

set(CMAKE_BUILD_TYPE Release CACHE STRING "Default to Release")

if(NOT MTENGHOME)
    message(FATAL_ERROR "MTENGHOME is not defined.  You must tell CMake where to find it. This will be your GAUSS Engine Installation directory")
    # exit early 
    return()
endif()

if(NOT DEFINED ENV{PHP_INCLUDE})
    message(FATAL_ERROR "PHP_INCLUDE is not defined. Please set this variable as a semi-colon separated list of PHP include paths.")
    return()
endif()
set(PHP_INCLUDE $ENV{PHP_INCLUDE})

if(WIN32)
    add_definitions(-DZEND_WIN32 -DPHP_WIN32 -DZEND_WIN32_FORCE_INLINE -D_USE_32BIT_TIME_T -DZEND_DEBUG=0 -DZTS=1 -DNSAPI=1)
endif()

# remove ":" from path
if(UNIX)
    string(REPLACE "-I" ";" PHP_INCLUDE ${PHP_INCLUDE})
endif()

message("Using PHP Include Path: ${PHP_INCLUDE}")

file(GLOB_RECURSE gauss_SOURCES "src/*.cpp")
file(GLOB_RECURSE gauss_HEADERS "src/*.h")

set(gauss_INCLUDE_DIRS "include;src;.")
foreach(_phpHeader ${PHP_INCLUDE})
    list(APPEND gauss_INCLUDE_DIRS ${_phpHeader})
endforeach()

if(WIN32)
    list(APPEND gauss_INCLUDE_DIRS "${MTENGHOME}/pthreads")
endif()

list(REMOVE_DUPLICATES gauss_INCLUDE_DIRS)

include_directories(${gauss_INCLUDE_DIRS})
find_library(MTENG_LIB mteng PATHS ${MTENGHOME})
add_library(gauss SHARED ${gauss_SOURCES})

if (WIN32)
    find_library(PHP_LIB php5ts PATHS ${gauss_INCLUDE_DIRS})
    target_link_libraries(gauss ${MTENG_LIB} ${PHP_LIB})
else()
    target_link_libraries(gauss ${MTENG_LIB})
endif()

