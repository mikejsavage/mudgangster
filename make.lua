require( "scripts.gen_makefile" )

-- it only really makes sense to vendor lua on windows/osx
-- but we can't actually build for them yet so do nothing for now
-- require( "libs.lua" )
-- require( "libs.lpeg" )

bin( "mudGangster", { "src/main", "src/script", "src/textbox", "src/input", "src/ui" } )
gcc_bin_ldflags( "mudGangster", "-lm -lX11 -llua" ) -- -Wl,-E" ) need to export symbols when vendoring
