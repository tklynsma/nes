cmake_minimum_required(VERSION 2.8)

project(nes)

set(SOURCE_FILES src/main.c src/common.h src/cpu.h src/cpu.c src/memory.h src/memory.c)
add_executable(nes_emulator ${SOURCE_FILES})

set(TEST_SOURCE_FILES test/main.c test/testunit.h test/testunit.c test/cpu_tests.h test/cpu_tests.c)
add_executable(nes_tests ${TEST_SOURCE_FILES})
