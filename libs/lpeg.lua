lib( "lpeg", {
	"libs/lpeg/lpcap",
	"libs/lpeg/lpcode",
	"libs/lpeg/lpprint",
	"libs/lpeg/lptree",
	"libs/lpeg/lpvm",
} )
-- obj_replace_cxxflags( "libs/lpeg/%", "-c -O2 -x c" )
obj_cxxflags( "libs/lpeg/%", "/c /TC /I libs/lua" )
