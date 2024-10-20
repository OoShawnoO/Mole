cmake_minimum_required(VERSION 3.10)
# project
project(Mole VERSION 1.0.0)
# set c++ standard
set(CMAKE_CXX_STANDARD 11)

if(WIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

if(MSVC)
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
endif()
# set cmake flag
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DMOLE_EXPORTS -ffunction-sections -fdata-sections -fno-rtti -fno-exceptions")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Os -g0")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--as-needed")
# set SRC
set(SRC src/Mole.cpp src/format.cc src/os.cc)
# set include path
include_directories(include)
# add static library
add_library(Mole STATIC ${SRC})
add_library(Mole.share SHARED ${SRC})

add_subdirectory(test)

add_custom_target(
        uninstall
        COMMAND ${CMAKE_COMMAND} -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX} -P "${CMAKE_SOURCE_DIR}/cmake/uninstall.cmake"
)

target_include_directories(
        Mole
        INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include/Mole>
)

set_target_properties(
        Mole
        PROPERTIES
        PUBLIC_HEADER   include/Mole.h
)

install(
        TARGETS Mole
        EXPORT Mole
        ARCHIVE DESTINATION lib/Mole
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib/Mole
        PUBLIC_HEADER DESTINATION include/
)

install(DIRECTORY include/fmt DESTINATION include)

install(
        EXPORT Mole
        FILE Mole.cmake
        NAMESPACE Mole::
        DESTINATION lib/cmake/Mole
)

include(CMakePackageConfigHelpers)

write_basic_package_version_file(
        MoleConfigVersion.cmake
        VERSION 1.0.0
        COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
        cmake/PackageConfig.cmake.in
        MoleConfig.cmake
        INSTALL_DESTINATION lib/cmake/Mole
)

install(
        FILES
        ${CMAKE_CURRENT_BINARY_DIR}/MoleConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/MoleConfigVersion.cmake
        DESTINATION lib/cmake/Mole
)