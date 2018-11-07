cmake_minimum_required(VERSION 3.8)
project(StudentELFReader)

set(CMAKE_C_STANDARD 11)

set(SOURCE_FILES elf_common.c elf_common.h)
add_library(StudentELFReader ${SOURCE_FILES})