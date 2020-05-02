if not arg[ 1 ] or not arg[ 2 ] then
	print( arg[ 0 ] .. " <source directory> <path to main>" )
	os.exit( 1 )
end

local lfs = require( "INTERNAL_LFS" )

local merged = { }

local root = arg[ 1 ]
local main = arg[ 2 ]

local function addDir( rel )
	for file in lfs.dir( root .. "/" .. rel ) do
		if file ~= "." and file ~= ".." then
			local full = root .. "/" .. rel .. file
			local attr = lfs.attributes( full )

			if attr.mode == "directory" then
				addDir( rel .. file .. "/" )
			elseif file:match( "%.lua$" ) and ( rel ~= "" or file ~= main ) then
				local f = io.open( full, "r" )
				local contents = f:read( "*all" )
				f:close()

				table.insert( merged, ( [[
					package.preload[ "%s" ] = function( ... )
						%s
					end]] ):format(
						( rel .. file ):gsub( "%.lua$", "" ):gsub( "/", "." ),
						contents
					)
				)
			end
		end
	end
end

addDir( "" )

local f = io.open( root .. "/" .. main, "r" )
local contents = f:read( "*all" )
f:close()

table.insert( merged, contents )

local combined = table.concat( merged, "\n" )

local f = io.open( "build/lua_combined.h", "w" )
for i = 1, #combined do
	f:write( string.byte( combined, i ) .. "," )
end
f:close()
