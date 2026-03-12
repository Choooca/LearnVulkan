cmake_minimum_required(VERSION 4.1.2)

add_subdirectory(render)
add_subdirectory(core)
add_subdirectory(utils)

target_sources(Dragibus
	PRIVATE
		main.cpp
)