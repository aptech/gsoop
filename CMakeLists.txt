cmake_minimum_required(VERSION 2.8)
project (ge)

set(ge_MAJOR_VERSION 0)
set(ge_MINOR_VERSION 3)
set(ge_PATCH_VERSION 1)
set(ge_VERSION
  ${ge_MAJOR_VERSION}.${ge_MINOR_VERSION}.${ge_PATCH_VERSION})

#set(CMAKE_BUILD_TYPE Release CACHE STRING "Default to Release")

if(NOT MTENGHOME)
    message(FATAL_ERROR "MTENGHOME is not defined.  You must tell CMake where to find it. This will be your GAUSS Engine Installation directory")
endif()

get_filename_component(MTENGHOME "${MTENGHOME}" ABSOLUTE)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

option(STATIC "Build static library instead of shared" OFF)
option(CPPONLY "Build C++ shared library only" OFF)

if(STATIC)
    set(GE_LIBRARY_TYPE STATIC)
else()
    set(GE_LIBRARY_TYPE SHARED)
endif()

find_library(MTENG_LIB mteng PATHS ${MTENGHOME} NO_DEFAULT_PATH)
set(GE_SRCS
    src/gauss.cpp src/gematrix.cpp src/gearray.cpp src/gestringarray.cpp 
    src/geworkspace.cpp src/workspacemanager.cpp src/gesymbol.cpp
)

if(CPPONLY)
    add_library(ge ${GE_LIBRARY_TYPE} ${GE_SRCS})
    target_include_directories(ge PUBLIC include src)
    target_link_libraries(ge PUBLIC ${MTENG_LIB})
    return()
endif()

set(CMAKE_SHARED_LIBRARY_PREFIX "")

unset(PHP_EXTENSION_DIR)

if(PHP_ROOT_DIR)
    get_filename_component(PHP_ROOT_DIR "${PHP_ROOT_DIR}" ABSOLUTE)
    message(STATUS "Checking for php-config in ${PHP_ROOT_DIR}/bin")
    set(PHP_CONFIG_HINT HINTS "${PHP_ROOT_DIR}" PATH_SUFFIXES bin NO_DEFAULT_PATH)
endif()

#if(NOT PHP_ROOT_DIR)
find_program(PHP_CONFIG_BINARY NAMES php-config5 php-config ${PHP_CONFIG_HINT})

if(NOT PHP_CONFIG_BINARY)
    message(FATAL_ERROR "Could not locate php-config binary. Ensure it can be found in PATH or provide PHP_ROOT_DIR argument instead")
endif()

execute_process(
    COMMAND ${PHP_CONFIG_BINARY} --includes
    OUTPUT_VARIABLE PHP_INCLUDE_DIRS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

set(CMAKE_CXX_FLAGS "${PHP_INCLUDE_DIRS} ${CMAKE_CXX_FLAGS}")

execute_process(
    COMMAND ${PHP_CONFIG_BINARY} --extension-dir
    OUTPUT_VARIABLE PHP_EXTENSION_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
    COMMAND ${PHP_CONFIG_BINARY} --version
    OUTPUT_VARIABLE PHP_VERSION_RAW
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

string(SUBSTRING "${PHP_VERSION_RAW}" 0 1 PHP_VERSION)

if(PHP_VERSION EQUAL 5)
    set(SWIG_PHP5_FOUND ON)
else()
    set(SWIG_PHP5_FOUND OFF)
endif()

#else()
#    set(PHP_INCLUDE_DIRS "${PHP_ROOT_DIR}/include/php")
#
#    foreach(DIR main TSRM Zend ext ext/date/lib)
#        list(APPEND PHP_INCLUDE_DIRS "${PHP_ROOT_DIR}/include/php/${DIR}")
#    endforeach()
#
#    list(APPEND gauss_INCLUDE_DIRS ${PHP_INCLUDE_DIRS})
#    list(REMOVE_DUPLICATES gauss_INCLUDE_DIRS)
#endif()

message(STATUS "Found PHP ${PHP_VERSION}")
message(STATUS "Using PHP Include Path: ${PHP_INCLUDE_DIRS}")

option(PHP5 "Use PHP5 mode for SWIG instead of PHP7" ${SWIG_PHP5_FOUND})
option(SWIG "Execute SWIG to build wrapper" OFF)
option(SWIGOFFICIAL "Create official prebuilt SWIG wrappers" OFF)

add_definitions(-DGAUSS_LIBRARY -DSWIGPHP)

#swig -c++ -php7 -outdir php -o src/gauss_wrap.cpp ge.i
if (PHP5)
    set(SWIG_PHP_FLAG php5)
    add_definitions(-DSWIGPHP5)
else()
    set(SWIG_PHP_FLAG php7)
    add_definitions(-DSWIGPHP7)
endif()


set(SWIG_CPP_WRAP_FILENAME "gauss_wrap.cpp")
set(SWIG_H_WRAP_FILENAME "gauss_wrap.h")
set(SWIG_PHP_H_WRAP_FILENAME "php_ge.h")
set(SWIG_GE_PHP_FILENAME "ge.php")
set(SWIG_WRAP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/php/${SWIG_PHP_FLAG}")
set(SWIG_CPP_WRAP_OUT "${SWIG_WRAP_DIR}/${SWIG_CPP_WRAP_FILENAME}")
set(SWIG_H_WRAP_OUT "${SWIG_WRAP_DIR}/${SWIG_H_WRAP_FILENAME}")
set(SWIG_PHP_H_WRAP_OUT "${SWIG_WRAP_DIR}/${SWIG_PHP_H_WRAP_FILENAME}")
set(SWIG_GE_PHP_OUT "${SWIG_WRAP_DIR}/${SWIG_GE_PHP_FILENAME}")

if(SWIG)
    if(NOT SWIG_BINARY)
        find_program(SWIG_BINARY swig)
    
        if(NOT SWIG_BINARY)
            message(FATAL_ERROR "Can't find SWIG binary. Please provide SWIG_BINARY argument or ensure it is findable in PATH")
        endif()
    endif()
   
    set(SWIG_CPP_WRAP_OUT "${CMAKE_CURRENT_BINARY_DIR}/${SWIG_CPP_WRAP_FILENAME}")
    set(SWIG_H_WRAP_OUT "${CMAKE_CURRENT_BINARY_DIR}/${SWIG_H_WRAP_FILENAME}")
    set(SWIG_PHP_H_WRAP_OUT "${CMAKE_CURRENT_BINARY_DIR}/${SWIG_PHP_H_WRAP_FILENAME}")

    add_custom_command(
        OUTPUT "${SWIG_CPP_WRAP_OUT}" "${SWIG_H_WRAP_OUT}" "${SWIG_PHP_H_WRAP_OUT}" "${SWIG_GE_PHP_OUT}"
        COMMAND "${SWIG_BINARY}" -c++ -${SWIG_PHP_FLAG} -outdir "${CMAKE_CURRENT_BINARY_DIR}" -o "${SWIG_CPP_WRAP_OUT}" -oh "${SWIG_H_WRAP_OUT}" ge.i
        MAIN_DEPENDENCY "${CMAKE_CURRENT_SOURCE_DIR}/ge.i"
        DEPENDS 
        "${CMAKE_CURRENT_SOURCE_DIR}/src/gauss.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/gesymbol.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/gearray.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/gematrix.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/gestringarray.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/geworkspace.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/workspacemanager.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/gefuncwrapper.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/gesymtype.h"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "Executing SWIG generator binary"
    )
    
    add_custom_target(swig 
        DEPENDS "${SWIG_CPP_WRAP_OUT}"
        COMMENT "Checking SWIG interface file for changes"
    )

    if(SWIGOFFICIAL)
        add_custom_command(TARGET swig POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${SWIG_CPP_WRAP_FILENAME}" "${SWIG_WRAP_DIR}"
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${SWIG_H_WRAP_FILENAME}" "${SWIG_WRAP_DIR}"
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${SWIG_PHP_H_WRAP_FILENAME}" "${SWIG_WRAP_DIR}"
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${SWIG_GE_PHP_FILENAME}" "${SWIG_WRAP_DIR}"
            DEPENDS "${SWIG_CPP_WRAP_OUT}" "${SWIG_H_WRAP_OUT}" "${SWIG_PHP_H_WRAP_OUT}" "${SWIG_GE_PHP_OUT}"
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
            COMMENT "Copying generated SWIG files to ${SWIG_WRAP_DIR}")
    endif()
else()
    # use pre-built
    configure_file("${SWIG_CPP_WRAP_OUT}" "${CMAKE_CURRENT_BINARY_DIR}" COPYONLY)
    configure_file("${SWIG_H_WRAP_OUT}" "${CMAKE_CURRENT_BINARY_DIR}" COPYONLY)
    configure_file("${SWIG_PHP_H_WRAP_OUT}" "${CMAKE_CURRENT_BINARY_DIR}" COPYONLY)
    configure_file("${SWIG_GE_PHP_OUT}" "${CMAKE_CURRENT_BINARY_DIR}" COPYONLY)
endif()

add_library(ge ${GE_LIBRARY_TYPE} "${SWIG_CPP_WRAP_OUT}" ${GE_SRCS})

target_include_directories(ge PUBLIC include src "${CMAKE_CURRENT_BINARY_DIR}")

if(SWIG)
    add_dependencies(ge swig)
else()
    target_include_directories(ge PUBLIC "${SWIG_WRAP_DIR}")
endif()

target_link_libraries(ge PUBLIC ${MTENG_LIB})

if(PHP_EXTENSION_DIR)
    message(STATUS "PHP Extension directory found: ${PHP_EXTENSION_DIR}")
    message(STATUS "Run 'make installext' to install directly. Please edit php.ini manually")

    add_custom_target(installext
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:ge>" "${PHP_EXTENSION_DIR}"
        DEPENDS ge
        COMMENT "Copying GE extension to PHP directory"
    )

    find_program(PHP_BINARY NAMES php php5 php7 ${PHP_CONFIG_HINT})
    find_program(PHP_UNIT_PHAR NAMES phpunit phpunit.phar ${PHP_CONFIG_HINT})

    if(PHP_BINARY AND PHP_UNIT_PHAR)
        message(STATUS "Found php and phpunit. Run 'make unittest' to execute unit test")
        add_custom_target(unittest 
            ${CMAKE_COMMAND} -E env MTENGHOME=${MTENGHOME} "${PHP_BINARY}" "${PHP_UNIT_PHAR}" "${CMAKE_CURRENT_SOURCE_DIR}/php/unit_test.php"
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
            COMMENT "Running unit test")
        add_dependencies(unittest installext)
    endif()
endif()

