require( "scripts.gen_makefile" )

-- bin( "wintest", { "win", "platform_network", "ggformat", "strlcpy", "strlcat", "patterns", "strtonum" } )
-- msvc_bin_ldflags( "wintest", "gdi32.lib Ws2_32.lib" )

-- it only really makes sense to vendor lua on windows/osx
-- but we can't actually build for them yet so do nothing for now
require( "libs.lua" )
require( "libs.lpeg" )
require( "libs.lfs" )

bin( "mudgangster", { "src/win32", "src/script", "src/textbox", "src/input", "src/platform_network" }, { "lua", "lpeg", "lfs" } )
msvc_bin_ldflags( "mudgangster", "gdi32.lib Ws2_32.lib" )
gcc_bin_ldflags( "mudgangster", "-lm -lX11 -llua" ) -- -Wl,-E" ) need to export symbols when vendoring
