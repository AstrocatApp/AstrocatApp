# Created by and for Qt Creator This file was created for editing the project sources only.
# You may attempt to use it for building too, by modifying this file here.

cmake_minimum_required(VERSION 3.5)
project(cfitsio)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 99)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(LIBRARY_OUTPUT_PATH ../libs)

IF(CMAKE_SYSTEM_NAME STREQUAL Windows)
add_compile_definitions(WIN32)
ELSEIF(CMAKE_SYSTEM_NAME STREQUAL Linux)
add_compile_definitions(unix)
ELSEIF(CMAKE_SYSTEM_NAME STREQUAL Darwin)
add_compile_definitions(unix)
ELSEIF(CMAKE_SYSTEM_NAME STREQUAL FreeBSD)
add_compile_definitions(unix)
ENDIF()

add_compile_definitions(_REENTRANT)
add_compile_definitions(STDC_HEADERS)
add_compile_definitions(HAVE_SYS_TYPES_H)
add_compile_definitions(HAVE_SYS_STAT_H)
add_compile_definitions(HAVE_STDLIB_H)
add_compile_definitions(HAVE_STRING_H)
add_compile_definitions(HAVE_MEMORY_H)
add_compile_definitions(HAVE_STRINGS_H)
add_compile_definitions(HAVE_INTTYPES_H)
add_compile_definitions(HAVE_STDINT_H)
add_compile_definitions(HAVE_UNISTD_H)
add_compile_definitions(HAVE_STDLIB_H)
add_compile_definitions(HAVE_STRING_H)
add_compile_definitions(HAVE_MATH_H)
add_compile_definitions(HAVE_LIMITS_H)
add_compile_definitions(_LARGEFILE_SOURCE)
add_compile_definitions(_FILE_OFFSET_BITS)
#add_compile_definitions(CFITSIO_HAVE_CURL)
add_compile_definitions(HAVE_FTRUNCATE)
add_compile_definitions(HAVE_LONGLONG)
add_compile_definitions(HAVE_SHMEM_SERVICES)
add_compile_definitions(HAVE_UNION_SEMUN)
add_compile_definitions(HAVE_FMEMOPEN)
add_compile_definitions(HAVE_NET_SERVICES)

include_directories(
    .
    zlib
)

set (SRCS
    zlib/adler32.c
    zlib/crc32.c
    zlib/crc32.h
    zlib/deflate.c
    zlib/deflate.h
    zlib/infback.c
    zlib/inffast.c
    zlib/inffast.h
    zlib/inffixed.h
    zlib/inflate.c
    zlib/inflate.h
    zlib/inftrees.c
    zlib/inftrees.h
    zlib/trees.c
    zlib/trees.h
    zlib/uncompr.c
    zlib/zcompress.c
    zlib/zconf.h
    zlib/zlib.h
    zlib/zuncompress.c
    zlib/zutil.c
    zlib/zutil.h
    buffers.c
    cfileio.c
    cfortran.h
    checksum.c
    cookbook.c
    drvrfile.c
    drvrgsiftp.c
    drvrgsiftp.h
    drvrmem.c
    drvrnet.c
    drvrsmem.c
    drvrsmem.h
    editcol.c
    edithdu.c
    eval_defs.h
    eval_f.c
    eval_l.c
    eval_tab.h
    eval_y.c
    f77_wrap.h
    f77_wrap1.c
    f77_wrap2.c
    f77_wrap3.c
    f77_wrap4.c
    fits_hcompress.c
    fits_hdecompress.c
    fitscopy.c
    fitscore.c
    fitsio.h
    fitsio2.h
    fpack.c
    fpack.h
    fpackutil.c
    funpack.c
    getcol.c
    getcolb.c
    getcold.c
    getcole.c
    getcoli.c
    getcolj.c
    getcolk.c
    getcoll.c
    getcols.c
    getcolsb.c
    getcolui.c
    getcoluj.c
    getcoluk.c
    getkey.c
    group.c
    group.h
    grparser.c
    grparser.h
    histo.c
    imcompress.c
    imcopy.c
    iraffits.c
    iter_a.c
    iter_b.c
    iter_c.c
    iter_image.c
    iter_var.c
    longnam.h
    modkey.c
    pliocomp.c
    putcol.c
    putcolb.c
    putcold.c
    putcole.c
    putcoli.c
    putcolj.c
    putcolk.c
    putcoll.c
    putcols.c
    putcolsb.c
    putcolu.c
    putcolui.c
    putcoluj.c
    putcoluk.c
    putkey.c
    quantize.c
    region.c
    region.h
    ricecomp.c
    scalnull.c
    simplerng.c
    simplerng.h
    smem.c
    speed.c
    swapproc.c
    testprog.c
#    vmsieee.c
    wcssub.c
    wcsutil.c
#    windumpexts.c
)

add_library(${PROJECT_NAME} ${SRCS})

target_link_libraries(${PROJECT_NAME} PRIVATE )
