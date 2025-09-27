# AdaScript
# (C) 2025 Afif Ali Saadman, Chief Author of AdaScript
# License: LGPL
cmake_minimum_required(VERSION 3.15)
project(AdaScript LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Library for embedding (exports C API)
add_library(adascript_core SHARED
    src/main.cpp
)

target_compile_definitions(adascript_core PRIVATE ADASCRIPT_NO_MAIN ADASCRIPT_BUILD_DLL)

target_include_directories(adascript_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        src
)

# CLI executable
add_executable(adascript
    src/main.cpp
)

target_include_directories(adascript PRIVATE src ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Platform-specific networking
if (WIN32)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    target_link_libraries(adascript_core winhttp)
    target_link_libraries(adascript winhttp)
else()
    find_package(CURL QUIET)
    if (CURL_FOUND)
        target_link_libraries(adascript_core CURL::libcurl)
        target_link_libraries(adascript CURL::libcurl)
    else()
        message(WARNING "libcurl not found; HTTP will be disabled on this build")
        target_compile_definitions(adascript_core PRIVATE ADASCRIPT_NO_CURL)
        target_compile_definitions(adascript PRIVATE ADASCRIPT_NO_CURL)
    endif()
endif()

