cmake_minimum_required(VERSION 3.18)
set(LLVM_MOS_PLATFORM nes-mmc1)
find_package(llvm-mos-sdk REQUIRED)
project(render LANGUAGES ASM CXX)

set(CMAKE_EXECUTABLE_SUFFIX .nes)

add_executable(render main.cc chr.s)
set_property(SOURCE chr.s PROPERTY OBJECT_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/chr.chr)

target_link_libraries(render neslib nesdoug)
target_include_directories(render PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})