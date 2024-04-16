cmake_minimum_required(VERSION 3.20)

project(Mole)

if(WIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

add_library(Mole SHARED Mole.cpp)
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
        PUBLIC_HEADER Mole.h
)

install(
        TARGETS Mole
        EXPORT Mole
        ARCHIVE DESTINATION lib/Mole
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib/Mole
        PUBLIC_HEADER DESTINATION include/Mole
)

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
