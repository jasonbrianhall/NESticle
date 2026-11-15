# NESticle with CMake: Windows with Microsoft C (vcpkg's SDL3), or Linux.
# (MS-DOS: msdos/Makefile. Linux and MinGW also have plain Makefiles in
# linux/ and windows/.)
#
# Windows (Visual Studio + vcpkg):
#   vcpkg install sdl3 --triplet x64-windows
#   cmake -S . -B build -A x64 -DCMAKE_TOOLCHAIN_FILE=<vcpkg>/scripts/buildsystems/vcpkg.cmake
#   cmake --build build --config Release
# Linux:
#   cmake -S . -B build && cmake --build build

cmake_minimum_required(VERSION 3.16)
project(NESticle CXX)

find_package(SDL3 CONFIG REQUIRED)

set(SRC ${CMAKE_SOURCE_DIR}/Source)
set(NES_SRCS build command config cpu disasm file font input inputw main message mmc mouse
             nes nesdlg nessound nesvideo prof r2img rom slist snapshot stddlg vol)
set(PORT_SRCS m6502 draw guirect gui guimenu nesapu guidata nespal guivolembed)

set(SOURCES ${CMAKE_SOURCE_DIR}/msdos/sdlmain.cpp)
foreach(f ${NES_SRCS})
  list(APPEND SOURCES ${SRC}/${f}.cpp)
endforeach()
foreach(f ${PORT_SRCS})
  list(APPEND SOURCES ${SRC}/port/${f}.cpp)
endforeach()
if(WIN32)
  list(APPEND SOURCES ${CMAKE_SOURCE_DIR}/windows/nesticle.rc)
endif()

add_executable(nesticle WIN32 ${SOURCES})
target_include_directories(nesticle PRIVATE ${SRC} ${CMAKE_BINARY_DIR})
target_compile_definitions(nesticle PRIVATE DOS SDL_PORT)
target_link_libraries(nesticle PRIVATE SDL3::SDL3)

# The original GUI.VOL, built into the program (Source/port/guivolembed.cpp),
# as a generated header of bytes
set(GUIVOL ${CMAKE_SOURCE_DIR}/msdos/GUI.VOL)
if(EXISTS ${GUIVOL})
  file(READ ${GUIVOL} guivol_hex HEX)
  string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," guivol_bytes "${guivol_hex}")
  file(WRITE ${CMAKE_BINARY_DIR}/guivol_data.h "${guivol_bytes}\n")
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${GUIVOL})
  set_source_files_properties(${SRC}/port/guivolembed.cpp PROPERTIES
    COMPILE_DEFINITIONS "NESTICLE_GUIVOL_INC=\"guivol_data.h\"")
endif()

if(MSVC)
  # /J: char is unsigned (the code expects Watcom's); /FI: the compatibility header
  target_compile_options(nesticle PRIVATE /J /FI${SRC}/port/port.h /W0 /permissive)
  target_compile_definitions(nesticle PRIVATE _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_WARNINGS)
else()
  target_compile_options(nesticle PRIVATE -fpermissive -funsigned-char -fno-strict-aliasing
    -fno-delete-null-pointer-checks -fno-exceptions -w -include ${SRC}/port/port.h)
endif()
