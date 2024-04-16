cmake_minimum_required(VERSION 3.20)

project(Mole)

if(WIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif ()

add_library(Mole SHARED Mole.cpp Mole.h)

install(
        TARGETS Mole
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
)

install(FILES Mole.h DESTINATION include)

add_custom_target(uninstall
        COMMAND ${CMAKE_COMMAND} -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX} -P "${CMAKE_SOURCE_DIR}/cmake/uninstall.cmake"
)


#add_executable(Mole test/main.cpp Mole.h Mole.cpp)
