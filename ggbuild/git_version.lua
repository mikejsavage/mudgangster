local function exec( cmd )
	local pipe = assert( io.popen( cmd ) )
	local res = pipe:read( "*line" )
	pipe:close()
	return res or ""
end

local version = exec( "git tag --points-at HEAD" )
if version == "" then
	version = exec( "git rev-parse --short HEAD" )
end

local gitversion = "#define APP_VERSION \"" .. version .. "\"\n"

local r = io.open( "src/gitversion.h", "r" )
local current = r and r:read( "*all" )
if r then
	r:close()
end

if current ~= gitversion then
	local w = assert( io.open( "src/gitversion.h", "w" ) )
	w:write( gitversion )
	assert( w:close() )
end
