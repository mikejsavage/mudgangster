require( "scripts.gen_makefile" )

bin( "mudGangster", { "src/main", "src/script", "src/textbox", "src/input", "src/ui" } )
gcc_bin_ldflags( "mudGangster", "-lm -lX11 -llua" )
