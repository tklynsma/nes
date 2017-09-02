cmake_minimum_required(VERSION 2.8)

project(nes)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

set(CMAKE_C_FLAGS_DEBUG "-g")

set(SOURCE_FILES src/main.c src/cartridge.c src/cpu.c src/cpu_flags.c src/log.c src/memory.c src/mmc.c src/nes.c src/ppu.c src/vram.c)
add_executable(nes_emulator ${SOURCE_FILES})
target_link_libraries(nes_emulator ${SDL2_LIBRARIES})

set(TEST_SOURCE_FILES test/src/main.c test/src/test_unit.c test/src/cpu_tests.c test/src/memory_tests.c test/src/ppu_tests.c test/src/vram_tests.c src/cartridge.c src/cpu.c src/cpu_flags.c src/log.c src/memory.c src/mmc.c src/ppu.c src/vram.c)
add_executable(nes_tests ${TEST_SOURCE_FILES})
