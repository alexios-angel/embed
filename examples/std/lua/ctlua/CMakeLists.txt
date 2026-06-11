add_executable(example.std.lua.ctlua source/lua.c++)
target_common_options(example.std.lua.ctlua ".c++")
target_common_example_options(example.std.lua.ctlua)
add_test(NAME example.std.lua.ctlua COMMAND example.std.lua.ctlua)
