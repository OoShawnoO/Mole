cmake_minimum_required(VERSION 3.20)

project(Mole)

#add_library(Mole SHARED Mole.cpp Mole.h)
add_executable(Mole test/main.cpp Mole.h Mole.cpp)
