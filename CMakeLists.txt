cmake_minimum_required(VERSION 3.18)
set(LLVM_MOS_PLATFORM nes-mmc1)
find_package(llvm-mos-sdk REQUIRED)
project(render LANGUAGES ASM C CXX)

set(CMAKE_EXECUTABLE_SUFFIX .nes)

add_executable(render
	chr.s
	draw.cc
	draw.s
	line.cc
	main.cc
	map.cc
	log.cc
	log.s
	logt.c
	lcos.c
	overhead.cc
	perspective.cc
	perspective.s
	screen.cc
	screen.s
	trig.cc
)

add_library(maps
	outer_inner_map.cc
	square_map.cc
	sectors.cc
)

target_link_libraries(render neslib nesdoug maps)
target_include_directories(render PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_options(render PRIVATE -T${CMAKE_CURRENT_SOURCE_DIR}/link.ld)

set_property(SOURCE chr.s PROPERTY
	OBJECT_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/sprites.chr ${CMAKE_CURRENT_SOURCE_DIR}/pixels.chr)

add_executable(render2
  chr.s
  framebuffer.cc
  framebuffer.S
  main2.cc
  nmi.s
)
target_include_directories(render2 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_options(render2 PRIVATE -T${CMAKE_CURRENT_SOURCE_DIR}/link.ld)
