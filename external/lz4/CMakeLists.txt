cmake_minimum_required(VERSION 3.5)
project(lz4)

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
    lz4.c
    lz4.h
    lz4hc.c
    lz4hc.h
)


add_library(${PROJECT_NAME} ${SRCS})

target_link_libraries(${PROJECT_NAME} PRIVATE )
