
file(GLOB_RECURSE files CONFIGURE_DEPENDS source/*.c++ source/*.c)
add_executable(example.std.lua ${files})
target_common_options(example.std.lua ".c++")
target_common_example_options(example.std.lua)
