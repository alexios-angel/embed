add_executable(example.std.shader.invalid source/invalid.cpp)
target_common_options(example.std.shader.invalid ".c++")
target_common_example_options(example.std.shader.invalid)
set_target_properties(example.std.shader.invalid
	PROPERTIES
		EXCLUDE_FROM_ALL TRUE
		EXCLUDE_FROM_DEFAULT_BUILD TRUE
)
add_test(
	NAME example.std.shader.invalid
	COMMAND ${CMAKE_COMMAND}
		--build ${CMAKE_BINARY_DIR}
		--config $<CONFIG> --target example.std.shader.invalid
)
set_tests_properties(example.std.shader.invalid
	PROPERTIES WILL_FAIL TRUE
)


add_executable(example.std.shader.parse source/parse.cpp)
target_common_options(example.std.shader.parse ".c++")
target_common_example_options(example.std.shader.parse)
add_test(NAME example.std.shader.parse COMMAND example.std.shader.parse)
