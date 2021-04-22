# Created by and for Qt Creator This file was created for editing the project sources only.
# You may attempt to use it for building too, by modifying this file here.

set(CMAKE_OSX_DEPLOYMENT_TARGET "10.14" CACHE STRING "Minimum OS X deployment version")
project(deps)

cmake_minimum_required(VERSION 3.5)
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/libs)
#add_subdirectory (cminpack)

add_subdirectory (lcms)
add_subdirectory (lz4)
add_subdirectory (RFC6234)
add_subdirectory (zlib)
add_subdirectory (pcl)

if (WIN32)
include(ExternalProject)
ExternalProject_Add(
  cfitsio
  SOURCE_DIR ${PROJECT_SOURCE_DIR}/cfitsio
  BUILD_IN_SOURCE 1
  PREFIX ${PROJECT_SOURCE_DIR}/build/cfitsio
  INSTALL_DIR ${PROJECT_SOURCE_DIR}/out/library
  CMAKE_ARGS
      -DBUILD_SHARED_LIBS=OFF
      -DUseCurl=OFF
      -DBUILD_TOOLS=OFF
      -DBUILD_TESTS=OFF
      -DCMAKE_INSTALL_PREFIX=${PROJECT_SOURCE_DIR}/build/libs/${CMAKE_BUILD_TYPE}
      -D_REENTRANT=ON
      list(APPEND CMAKE_ARGS "-DUSE_PTHREADS=ON")
      list(APPEND CMAKE_ARGS "-DPTHREADS_INCLUDE_DIR=${PROJECT_SOURCE_DIR}/pthreads/")
      list(APPEND CMAKE_ARGS "-DPTHREADS_LIBRARY=${PROJECT_SOURCE_DIR}/build/libs")
)
endif (WIN32)
