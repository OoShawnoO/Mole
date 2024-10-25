/**
  ******************************************************************************
  * @file           : main.cpp
  * @author         : huzhida
  * @brief          : None
  * @date           : 2024/8/18
  ******************************************************************************
  */

#include <Mole.h>
#include <iostream>

int main() {

  MOLE_ENABLE(true);
  MOLE_CONSOLE(true);
  MOLE_SAVE(true,"tmp.log");

  std::string world = "world";
  MOLE_TRACE("hello {}",world);
  MOLE_DEBUG("hello {}",world);
  MOLE_INFO("hello {}",world);
  MOLE_WARN("hello {}",world);
  MOLE_ERROR("hello {}",world);
  MOLE_FATAL("hello {}",world);
}
