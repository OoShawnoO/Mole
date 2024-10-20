message(path = ${CMAKE_INSTALL_PREFIX})

if(WIN32)
    if(EXISTS ${CMAKE_INSTALL_PREFIX}/lib/Mole)
        file(REMOVE_RECURSE ${CMAKE_INSTALL_PREFIX}/lib/Mole)
        message("delete shared library:${CMAKE_INSTALL_PREFIX}/lib/Mole")
    endif ()
elseif(UNIX)
    if(EXISTS ${CMAKE_INSTALL_PREFIX}/lib/Mole)
        file(REMOVE_RECURSE ${CMAKE_INSTALL_PREFIX}/lib/Mole)
        message("delete shared library:${CMAKE_INSTALL_PREFIX}/lib/Mole")
    endif ()
endif ()

if(EXISTS ${CMAKE_INSTALL_PREFIX}/lib/cmake/Mole)
    file(REMOVE_RECURSE ${CMAKE_INSTALL_PREFIX}/lib/cmake/Mole)
    message("delete cmake config dir:${CMAKE_INSTALL_PREFIX}/lib/cmake/Mole")
endif ()

if (EXISTS ${CMAKE_INSTALL_PREFIX}/include/Mole.h)
    file(REMOVE_RECURSE ${CMAKE_INSTALL_PREFIX}/include/Mole.h)
    message("delete header dir:${CMAKE_INSTALL_PREFIX}/include/Mole.h")
endif ()

if (EXISTS ${CMAKE_INSTALL_PREFIX}/include/fmt)
    file(REMOVE_RECURSE ${CMAKE_INSTALL_PREFIX}/include/fmt)
    message("delete header dir:${CMAKE_INSTALL_PREFIX}/include/fmt")
endif ()