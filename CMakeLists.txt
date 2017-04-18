cmake_minimum_required(VERSION 3.4)

set(CMAKE_CXX_STANDARD 11)

include_directories (
	CImg
)

add_definitions (-Dcimg_display=0)

add_executable (
	typewriter
	./main.cpp
)
