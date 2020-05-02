lib( "lfs", { "libs/lfs/lfs.cc" } )
obj_cxxflags( "libs/lfs/.*", "/c /TC /I libs/lua" )
obj_cxxflags( "libs/lfs/.*", "/wd4267" )
