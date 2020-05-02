lib( "lpeg", { "libs/lpeg/*.cc" } )
obj_cxxflags( "libs/lpeg/.*", "/c /TC /I libs/lua" )
obj_cxxflags( "libs/lpeg/.*", "/wd4244 /wd4267" )
