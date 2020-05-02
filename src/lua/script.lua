local lfs = require( "lfs" )
local serialize = require( "serialize" )

local ScriptsDirs
if mud.os == "windows" then
	ScriptsDirs = {
		os.getenv( "APPDATA" ) .. "\\Mud Gangster\\scripts",
		os.getenv( "USERPROFILE" ) .. "\\Documents\\Mud Gangster\\scripts",
	}
else
	ScriptsDirs = { os.getenv( "HOME" ) .. "/.mudgangster/scripts" }
end

-- TODO: missing exe dir
do
	local paths = { }
	for _, dir in ipairs( ScriptsDirs ) do
		table.insert( paths, dir .. "\\?.lua" )
	end

	package.path = package.path .. ";" .. table.concat( paths, ";" )
end

local PathSeparator = package.config:match( "^([^\n]+)" )

local function loadScript( name, path, padding )
	local function throw( err )
		if err:match( "^" .. path ) then
			err = err:sub( path:len() + 2 )
		end

		mud.print( "#lr%-" .. padding .. "s", name )
		mud.print( "#s\n>\n>     %s\n>", err )
	end

	mud.print( "#s\n>   " )

	local mainPath = path .. PathSeparator .. "main.lua"
	local readable, err = io.readable( mainPath )

	if not readable then
		throw( err )

		return
	end

	local env = setmetatable(
		{
			require = function( requirePath, ... )
				return require( "%s.%s" % { name, requirePath }, ... )
			end,

			saved = function( defaults )
				local settingsPath = path .. PathSeparator .. "settings.lua"

				mud.listen( "shutdown", function()
					local file = io.open( settingsPath, "w" )
					file:write( serialize.unwrapped( defaults ) )
					file:close()
				end )

				local settingsEnv = setmetatable( { }, {
					__newindex = defaults,
				} )

				local settings = loadfile( settingsPath, "t", settingsEnv )

				if not settings then
					return defaults
				end

				if _VERSION == "Lua 5.1" then
					setfenv( settings, settingsEnv )
				end

				pcall( settings )

				return defaults
			end,
		},
		{
			__index = _G,
			__newindex = _G,
		}
	)

	local script, err = loadfile( mainPath, "t", env )

	if not script then
		throw( err )

		return
	end

	if _VERSION == "Lua 5.1" then
		setfenv( script, env )
	end

	local loaded, err = pcall( script )

	if loaded then
		mud.print( "#lg%s", name )
	else
		throw( err )
	end
end

local function loadScriptsFrom( dir )
	mud.print( "\n#s> Loading scripts from %s... ", dir )

	local readable, err = io.readable( dir )

	if not readable then
		mud.print( "#lrfailed!\n#s>     %s", err )
		return
	end

	local attr = lfs.attributes( dir )

	if attr.mode == "directory" then
		local scripts = { }
		local maxLen = 0

		for script in lfs.dir( dir ) do
			if not script:match( "^%." ) then
				local path = dir .. PathSeparator .. script
				local scriptAttr = lfs.attributes( path )

				if scriptAttr.mode == "directory" then
					maxLen = math.max( script:len(), maxLen )

					table.insert( scripts, {
						name = script,
						path = path,
					} )
				end
			end
		end

		table.sort( scripts, function( a, b )
			return a.name < b.name
		end )

		for _, script in ipairs( scripts ) do
			loadScript( script.name, script.path, maxLen )
		end
	else
		mud.print( "#lrfailed!\n#s>     `%s' isn't a directory", dir )
	end
end

local function loadScripts( exe_path )
	for _, dir in ipairs( ScriptsDirs ) do
		loadScriptsFrom( dir )
	end

	loadScriptsFrom( exe_path .. PathSeparator .. "scripts" )

	mud.event( "scriptsLoaded" )
end

return {
	load = loadScripts,
}
