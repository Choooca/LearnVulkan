configure_file(build_macro.h.in build_macro.h)

target_sources(Dragibus
	PRIVATE
		files.cpp
)
