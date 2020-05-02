require( "ggbuild.gen_ninja" )

obj_cxxflags( ".*", "-I source -I libs" )

msvc_obj_cxxflags( ".*", "/W4 /wd4100 /wd4146 /wd4189 /wd4201 /wd4307 /wd4324 /wd4351 /wd4127 /wd4505 /wd4530 /wd4702 /wd4706 /D_CRT_SECURE_NO_WARNINGS" )
msvc_obj_cxxflags( ".*", "/wd4244 /wd4267" ) -- silence conversion warnings because there are tons of them
msvc_obj_cxxflags( ".*", "/fp:fast /GR- /EHs-c-" )

gcc_obj_cxxflags( ".*", "-std=c++11 -msse3 -ffast-math -fno-exceptions -fno-rtti -fno-strict-aliasing -fno-strict-overflow -fvisibility=hidden" )
gcc_obj_cxxflags( ".*", "-Wall -Wextra -Wcast-align -Wvla -Wformat-security" ) -- -Wconversion
gcc_obj_cxxflags( ".*", "-Wno-unused-parameter -Wno-missing-field-initializers -Wno-implicit-fallthrough -Wno-format-truncation" )
gcc_obj_cxxflags( ".*", "-Werror=vla -Werror=format-security -Werror=unused-value" )

local platform_srcs, platform_libs

if OS == "windows" then
	require( "libs.lua" )
	require( "libs.lpeg" )
	require( "libs.lfs" )

	platform_srcs = "src/win32.cc"
	platform_libs = { "lua", "lpeg", "lfs" }
else
	platform_srcs = "src/x11.cc"
	platform_libs = { }
end

bin( "mudgangster", {
	srcs = {
		platform_srcs,
		"src/ui.cc", "src/script.cc", "src/textbox.cc", "src/input.cc", "src/platform_network.cc",
	},

	libs = platform_libs,

	rc = "src/rc",

	msvc_extra_ldflags = "gdi32.lib Ws2_32.lib",
	gcc_extra_ldflags = "-lm -lX11 -llua",
} )

obj_dependencies( "src/script.cc", "build/lua_combined.h" )

printf( [[
rule combine-lua
    command = $lua ggbuild/pack_lua.lua src/lua main.lua
    description = lua_combined.h
]] )
print( "build build/lua_combined.h: combine-lua | ggbuild/pack_lua.lua" )
