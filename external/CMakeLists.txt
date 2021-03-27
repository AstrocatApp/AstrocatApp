# Created by and for Qt Creator This file was created for editing the project sources only.
# You may attempt to use it for building too, by modifying this file here.

set(CMAKE_OSX_DEPLOYMENT_TARGET "10.14" CACHE STRING "Minimum OS X deployment version")

project(deps)

cmake_minimum_required(VERSION 3.5)
#add_subdirectory (cminpack)

add_subdirectory (lcms)
add_subdirectory (lz4)
add_subdirectory (RFC6234)
add_subdirectory (zlib)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cfitsio")
add_subdirectory (cfitsio)
add_subdirectory (pcl)

