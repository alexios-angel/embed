include(FetchContent)

function (sol2_fetch_content_jail)
	FetchContent_Declare(sol2
		GIT_REPOSITORY https://github.com/ThePhD/sol2.git
		GIT_TAG        main
	)
	FetchContent_MakeAvailable(sol2)
	FetchContent_Declare(lua
		GIT_REPOSITORY https://github.com/walterschell/Lua.git
		GIT_TAG        master
	)
	FetchContent_MakeAvailable(lua)
endfunction()
sol2_fetch_content_jail()

add_executable(example.std.lua.live source/live.c++)
target_common_options(example.std.lua.live ".c++")
target_common_example_options(example.std.lua.live)
target_link_libraries(example.std.lua.live PRIVATE sol2::sol2 lua_static)
add_test(NAME example.std.lua.live COMMAND example.std.lua.live)
