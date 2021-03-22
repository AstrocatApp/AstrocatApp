cmake_minimum_required(VERSION 3.5)
project(zlib)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 99)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(LIBRARY_OUTPUT_PATH ../libs)

include_directories(
    .
)

add_compile_definitions(_REENTRANT)

set (SRCS
    adler32.c
    compress.c
    crc32.c
    crc32.h
    deflate.c
    deflate.h
    gzclose.c
    gzguts.h
    gzlib.c
    gzread.c
    gzwrite.c
    infback.c
    inffast.c
    inffast.h
    inffixed.h
    inflate.c
    inflate.h
    inftrees.c
    inftrees.h
    trees.c
    trees.h
    uncompr.c
    zconf.h
    zlib.def
    zlib.h
    zutil.c
    zutil.h
)

set_source_files_properties(zlib.def PROPERTIES HEADER_FILE_ONLY TRUE)
add_library(${PROJECT_NAME} ${SRCS})

#target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE )
target_compile_options(${PROJECT_NAME} PUBLIC "-pthread")
target_compile_options(${PROJECT_NAME} PUBLIC "-march=x86-64")
target_compile_options(${PROJECT_NAME} PUBLIC "-fPIC")
target_compile_options(${PROJECT_NAME} PUBLIC "-mtune=skylake")
target_compile_options(${PROJECT_NAME} PUBLIC "-msse4.2")
target_compile_options(${PROJECT_NAME} PUBLIC "-minline-all-stringops")
target_compile_options(${PROJECT_NAME} PUBLIC "-O3")
target_compile_options(${PROJECT_NAME} PUBLIC "-ffunction-sections")
target_compile_options(${PROJECT_NAME} PUBLIC "-fdata-sections")
target_compile_options(${PROJECT_NAME} PUBLIC "-ffast-math")
target_compile_options(${PROJECT_NAME} PUBLIC "-Wall")
target_compile_options(${PROJECT_NAME} PUBLIC "-Wno-parentheses")
target_compile_options(${PROJECT_NAME} PUBLIC "-Wno-error=implicit-function-declaration")
target_compile_options(${PROJECT_NAME} PUBLIC "-MMD")
target_compile_options(${PROJECT_NAME} PUBLIC "-MP")


#include(CheckTypeSize)
#include(CheckFunctionExists)
#include(CheckIncludeFile)
#include(CheckCSourceCompiles)
#check_include_file(sys/types.h HAVE_SYS_TYPES_H)
#check_include_file(stdint.h HAVE_STDINT_H)
#check_include_file(stddef.h HAVE_STDDEF_H)
