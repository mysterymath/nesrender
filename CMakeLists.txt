cmake_minimum_required(VERSION 3.18)
set(LLVM_MOS_PLATFORM nes-nrom)
find_package(llvm-mos-sdk REQUIRED)
project(render LANGUAGES ASM CXX)

add_executable(render main.cc chr.s)
set_property(SOURCE chr.s PROPERTY OBJECT_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/chr.chr)

target_link_libraries(render neslib)
target_include_directories(render PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
