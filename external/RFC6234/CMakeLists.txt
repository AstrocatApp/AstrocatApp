# Created by and for Qt Creator This file was created for editing the project sources only.
# You may attempt to use it for building too, by modifying this file here.

cmake_minimum_required(VERSION 3.5)
project(RFC6234)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(LIBRARY_OUTPUT_PATH ../libs)

include_directories(
    .
)

set (SRCS
    hkdf.c
    hmac.c
    sha-private.h
    sha.h
    sha1.c
    sha224-256.c
    sha384-512.c
    usha.c
)

add_library(${PROJECT_NAME} ${SRCS})

target_link_libraries(${PROJECT_NAME} PRIVATE )
