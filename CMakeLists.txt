cmake_minimum_required(VERSION 2.8)

project(nes)

set(SOURCE_FILES src/main.c src/cpu.c src/cpu_flags.c src/memory.c)
add_executable(nes_emulator ${SOURCE_FILES})

set(TEST_SOURCE_FILES test/src/main.c test/src/test_unit.c test/src/cpu_tests.c)
add_executable(nes_tests ${TEST_SOURCE_FILES})
