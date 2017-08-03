cmake_minimum_required(VERSION 2.8)

project(nes)

set(SOURCE_FILES src/main.c)
add_executable(nes_emulator ${SOURCE_FILES})
