# Created by and for Qt Creator This file was created for editing the project sources only.
# You may attempt to use it for building too, by modifying this file here.

set(CMAKE_OSX_DEPLOYMENT_TARGET "10.14" CACHE STRING "Minimum OS X deployment version")

project(deps)

cmake_minimum_required(VERSION 3.5)
set(CMAKE_BUILD_TYPE Release)
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/libs)
#add_subdirectory (cminpack)

add_subdirectory (lcms)
add_subdirectory (lz4)
add_subdirectory (RFC6234)
add_subdirectory (zlib)
#list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cfitsio")
#add_subdirectory (cfitsio)
add_subdirectory (pcl)

include(ExternalProject)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/out/library)
ExternalProject_Add(
  cfitsio
#  GIT_REPOSITORY "https://github.com/healpy/cfitsio.git"
#  GIT_TAG "master"
#  GIT_SHALLOW TRUE

  SOURCE_DIR ${PROJECT_SOURCE_DIR}/cfitsio
  BUILD_IN_SOURCE 1
  BUILD_COMMAND make
  PREFIX ${PROJECT_SOURCE_DIR}/cfitsio
#  CONFIGURE_COMMAND <SOURCE_DIR>/configure --enable-reentrant --disable-curl --enable-ssse3
  CONFIGURE_COMMAND <SOURCE_DIR>/configure --enable-reentrant --disable-curl
  INSTALL_DIR ${PROJECT_SOURCE_DIR}/out/library
#  INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_directory lib ${PROJECT_BINARY_DIR}/libs
  INSTALL_COMMAND ${CMAKE_COMMAND} -E copy libcfitsio.a ${PROJECT_BINARY_DIR}/libs/
#  CMAKE_ARGS
#      -DBUILD_SHARED_LIBS=OFF
#      -DUSE_PTHREADS=ON
#      -DUseCurl=OFF
#      -DBUILD_TOOLS=OFF
#      -DBUILD_TESTS=OFF
#      -DCMAKE_INSTALL_PREFIX=${PROJECT_SOURCE_DIR}/external/libs/${CMAKE_BUILD_TYPE}
#      -DCMAKE_BUILD_TYPE=Release
#      -D_REENTRANT=ON
)

#ExternalProject_Add(
#  pcl
#  SOURCE_DIR ${PROJECT_SOURCE_DIR}/pcl
#  BUILD_IN_SOURCE 1
#  INSTALL_DIR ${PROJECT_SOURCE_DIR}/out/library
#  CMAKE_ARGS
#      -DBUILD_SHARED_LIBS=ON
#      -DUSE_PTHREADS=ON
#      -DBUILD_TOOLS=OFF
#      -DBUILD_TESTS=OFF
#      -DCMAKE_INSTALL_PREFIX=${PROJECT_SOURCE_DIR}/external/libs/${CMAKE_BUILD_TYPE}
#      -DCMAKE_BUILD_TYPE=Release
#      -D_REENTRANT=ON

#)
