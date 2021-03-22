# Created by and for Qt Creator This file was created for editing the project sources only.
# You may attempt to use it for building too, by modifying this file here.

cmake_minimum_required(VERSION 3.5)
project(lcms)

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
    cmsalpha.c
    cmscam02.c
    cmscgats.c
    cmscnvrt.c
    cmserr.c
    cmsgamma.c
    cmsgmt.c
    cmshalf.c
    cmsintrp.c
    cmsio0.c
    cmsio1.c
    cmslut.c
    cmsmd5.c
    cmsmtrx.c
    cmsnamed.c
    cmsopt.c
    cmspack.c
    cmspcs.c
    cmsplugin.c
    cmsps2.c
    cmssamp.c
    cmssm.c
    cmstypes.c
    cmsvirt.c
    cmswtpnt.c
    cmsxform.c
    lcms2.def
    lcms2.h
    lcms2_internal.h
    lcms2_plugin.h
)

set_source_files_properties(lcms2.def PROPERTIES HEADER_FILE_ONLY TRUE)
add_library(${PROJECT_NAME} ${SRCS})

target_link_libraries(${PROJECT_NAME} PRIVATE)
