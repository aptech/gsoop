cmake_minimum_required(VERSION 2.8)
project (gauss)

IF (WIN32)
    SET(CMAKE_SHARED_LIBRARY_PREFIX "php_")
ELSE()
    SET(CMAKE_SHARED_LIBRARY_PREFIX "")
ENDIF()

SET(CMAKE_BUILD_TYPE Release CACHE STRING "Default to Release" FORCE)

IF(NOT DEFINED ENV{MTENGHOME15})
  message("MTENGHOME15 is not defined.  You must tell CMake where to find it. This will be your GAUSS Installation directory")
  # exit early 
  return()
ENDIF()

IF(NOT DEFINED ENV{PHP_INCLUDE})
  message("PHP_INCLUDE is not defined. Please set this variable as a semi-colon separated list of PHP include paths.")
  return()
ENDIF()

SET(MTENGHOME15 $ENV{MTENGHOME15})
SET(PHP_INCLUDE $ENV{PHP_INCLUDE})

IF (WIN32)
    add_definitions(-DZEND_WIN32)
    add_definitions(-DPHP_WIN32)
    add_definitions(-DZEND_WIN32_FORCE_INLINE)
    add_definitions(-D_USE_32BIT_TIME_T)
    add_definitions(-DZEND_DEBUG=0)
    add_definitions(-DZTS=1)
    add_definitions(-DNSAPI=1)
ENDIF()

# remove ":" from path
IF (UNIX)
    STRING(REGEX REPLACE "-I" ";" PHP_INCLUDE ${PHP_INCLUDE})
ENDIF()

message("Using PHP Include Path: ${PHP_INCLUDE}")

set(gauss_MAJOR_VERSION 0)
set(gauss_MINOR_VERSION 1)
set(gauss_PATCH_VERSION 0)
set(gauss_VERSION
  ${gauss_MAJOR_VERSION}.${gauss_MINOR_VERSION}.${gauss_PATCH_VERSION})

file(GLOB_RECURSE gauss_SOURCES "src/*.cpp")
file(GLOB_RECURSE gauss_HEADERS "src/*.h")

set (gauss_INCLUDE_DIRS "include;src;.")
#foreach (_headerFile ${gauss_HEADERS})
#    get_filename_component(_dir ${_headerFile} PATH)
#    list (APPEND gauss_INCLUDE_DIRS ${_dir})
#endforeach()

foreach (_phpHeader ${PHP_INCLUDE})
    list (APPEND gauss_INCLUDE_DIRS ${_phpHeader})
endforeach()

IF (WIN32)
    list(APPEND gauss_INCLUDE_DIRS "${MTENGHOME15}/pthreads")
ENDIF()

list(REMOVE_DUPLICATES gauss_INCLUDE_DIRS)

include_directories(${gauss_INCLUDE_DIRS})

find_library(MTENG_LIB mteng ${MTENGHOME15})

add_library (gauss SHARED ${gauss_SOURCES})

IF (WIN32)
    find_library(PHP_LIB php5ts ${gauss_INCLUDE_DIRS})
    target_link_libraries(gauss ${MTENG_LIB} ${PHP_LIB})
ELSE()
    target_link_libraries(gauss ${MTENG_LIB})
ENDIF()

