lib( "lfs", { "libs/lfs/lfs" } )
-- obj_replace_cxxflags( "libs/lpeg/%", "-c -O2 -x c" )
obj_cxxflags( "libs/lfs/%", "/c /TC /I libs/lua" )
obj_cxxflags( "libs/lfs/%", "/wd4267" )
