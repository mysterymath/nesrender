cmake_minimum_required(VERSION 3.18)
set(LLVM_MOS_PLATFORM nes-mmc1)
find_package(llvm-mos-sdk REQUIRED)
project(render LANGUAGES ASM C CXX)

set(CMAKE_EXECUTABLE_SUFFIX .nes)

add_executable(render main.cc chr.c)

target_link_libraries(render neslib nesdoug)
target_include_directories(render PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
