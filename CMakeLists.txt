cmake_minimum_required(VERSION 3.14)

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS on)

project(FunCompiler C)

add_executable(
  func
  src/main.c
)
