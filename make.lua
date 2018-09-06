require( "scripts.gen_makefile" )

-- bin( "wintest", { "win", "platform_network", "ggformat", "strlcpy", "strlcat", "patterns", "strtonum" } )
-- msvc_bin_ldflags( "wintest", "gdi32.lib Ws2_32.lib" )

local main_obj = { "src/main", "src/x11" }
local libs = { }

if OS == "windows" then
	require( "libs.lua" )
	require( "libs.lpeg" )
	require( "libs.lfs" )

	main_obj = "src/win32"
	libs = { "lua", "lpeg", "lfs" }
end

bin( "mudgangster", { main_obj, "src/script", "src/textbox", "src/input", "src/platform_network" }, libs )
msvc_bin_ldflags( "mudgangster", "gdi32.lib Ws2_32.lib" )
gcc_bin_ldflags( "mudgangster", "-lm -lX11 -llua" ) -- -Wl,-E" ) need to export symbols when vendoring
