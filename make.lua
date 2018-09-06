require( "scripts.gen_makefile" )

local platform_objs = { "src/main", "src/x11" }
local libs = { }

if OS == "windows" then
	require( "libs.lua" )
	require( "libs.lpeg" )
	require( "libs.lfs" )

	platform_objs = "src/win32"
	libs = { "lua", "lpeg", "lfs" }
end

bin( "mudgangster", { platform_objs, "src/script", "src/textbox", "src/input", "src/platform_network" }, libs )
msvc_bin_ldflags( "mudgangster", "gdi32.lib Ws2_32.lib" )
gcc_bin_ldflags( "mudgangster", "-lm -lX11 -llua" )
