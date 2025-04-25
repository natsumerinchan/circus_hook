cmake_minimum_required(VERSION 3.15)
project(KADENZF_CHS LANGUAGES C CXX)

if(MSVC)
    add_compile_options(/utf-8)
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_library(KADENZF_CHS SHARED 
    src/dllmain.cpp
)

target_compile_definitions(KADENZF_CHS PRIVATE 
    _WINDLL
    _USRDLL
)

include_directories(src/detours)

# 显式指定库文件路径
target_link_directories(KADENZF_CHS PRIVATE src/detours)
# 显式指定库名称
target_link_libraries(KADENZF_CHS detours)

set_target_properties(KADENZF_CHS PROPERTIES
    OUTPUT_NAME "KADENZF_CHS"
    PREFIX ""
    SUFFIX ".dll"
)
