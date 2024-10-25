cmake_minimum_required(VERSION 3.10)

# project
project(Mole
        VERSION 1.0.0
        LANGUAGES CXX)

# cpp standard
set(CMAKE_CXX_STANDARD 11)

# multi-platform
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    # Windows settings
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)  # 避免Windows平台上的安全警告
    add_compile_options(/utf-8)  # 设置源文件编码为UTF-8
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # Linux settings
    add_compile_options(-Wall -Wextra -Wpedantic)  # 启用常见的警告
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    # macOS settings
    add_compile_options(-Wall -Wextra -Wpedantic)  # 启用常见的警告
endif ()

# output path
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
# set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
# set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)

# subdirectory
# add_subdirectory()

# test
# enable_testing()
# add_subdirectory(tests)

# third party
# find_package()

# executable
add_executable(Mole.test main.cpp)

# include path
# link libraries
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_link_libraries(Mole.test Mole)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(Mole.test Mole pthread)
endif()