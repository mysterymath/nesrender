cmake_minimum_required(VERSION 3.18)
set(LLVM_MOS_PLATFORM nes-mmc1)
find_package(llvm-mos-sdk REQUIRED)
project(render LANGUAGES ASM C CXX)

set(CMAKE_EXECUTABLE_SUFFIX .nes)

add_executable(render
	chr.s
	draw.cc
	line.cc
	main.cc
	map.cc
	log.cc
	log.s
	logt.c
	lcos.c
	overhead.cc
	perspective.cc
	screen.cc
	trig.cc
)

add_library(maps
	outer_inner_map.cc
	square_map.cc
)

target_link_libraries(render neslib nesdoug maps)
target_include_directories(render PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

set_property(SOURCE chr.s PROPERTY
	OBJECT_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/sprites.chr ${CMAKE_CURRENT_SOURCE_DIR}/pixels.chr)
