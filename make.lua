require( "scripts.gen_makefile" )

-- it only really makes sense to vendor lua on windows/osx
-- but we can't actually build for them yet so do nothing for now
-- require( "libs.lua" )
-- require( "libs.lpeg" )

bin( "mudgangster", { "src/main", "src/script", "src/textbox", "src/input", "src/x11" } )
gcc_bin_ldflags( "mudgangster", "-lm -lX11 -llua" ) -- -Wl,-E" ) need to export symbols when vendoring
