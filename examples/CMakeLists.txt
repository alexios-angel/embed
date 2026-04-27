function(target_common_example_options target)
	target_compile_options(${target}
		PRIVATE
		"--embed-dir=${CMAKE_SOURCE_DIR}/examples/assets"
	)
endfunction()

add_subdirectory(stdc)
add_subdirectory(std)
