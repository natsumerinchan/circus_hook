cmake_minimum_required(VERSION 3.15)
project(CIRCUS_HOOK LANGUAGES C CXX)

if(MSVC)
    add_compile_options(/utf-8)
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_library(CIRCUS_HOOK SHARED 
    src/dllmain.cpp
)

target_compile_definitions(CIRCUS_HOOK PRIVATE 
    _WINDLL
    _USRDLL
)

include_directories(src/detours)

# 显式指定库文件路径
target_link_directories(CIRCUS_HOOK PRIVATE src/detours)
# 显式指定库名称
target_link_libraries(CIRCUS_HOOK detours)

set_target_properties(CIRCUS_HOOK PROPERTIES
    OUTPUT_NAME "CIRCUS_HOOK"
    PREFIX ""
    SUFFIX ".dll"
)
