file(GLOB_RECURSE files
	CONFIGURE_DEPENDS *.c++
)

foreach (f ${files})
	cmake_path(GET f STEM LAST_ONLY fstem)
	cmake_path(GET f EXTENSION LAST_ONLY fext)
	set(target example.std.basic.${fstem})
	add_executable(${target} ${f})
	target_common_options(${target} ${fext})
	target_common_example_options(${target})
	add_test(NAME ${target} COMMAND ${target})
endforeach()
