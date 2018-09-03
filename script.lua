local lfs = require( "lfs" )
local serialize = require( "serialize" )

local ScriptsDir = os.getenv( "HOME" ) .. "/.mudgangster/scripts"

package.path = package.path .. ";" .. ScriptsDir .. "/?.lua"

local function loadScript( name, path, padding )
	local function throw( err )
		if err:match( "^" .. path ) then
			err = err:sub( path:len() + 2 )
		end

		mud.print( "#lr%-" .. padding .. "s", name )
		mud.print( "#s\n>\n>     %s\n>", err )
	end

	mud.print( "#s\n>   " )

	local mainPath = path .. "/main.lua"
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
				local settingsPath = path .. "/settings.lua"

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

local function loadScripts()
	mud.print( "#s> Loading scripts..." )

	local readable, err = io.readable( ScriptsDir )

	if readable then
		local attr = lfs.attributes( ScriptsDir )

		if attr.mode == "directory" then
			local scripts = { }
			local maxLen = 0

			for script in lfs.dir( ScriptsDir ) do
				if not script:match( "^%." ) then
					local path = ScriptsDir .. "/" .. script
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
			mud.print( "#sfailed!\n> `%s' isn't a directory", ScriptsDir )
		end
	else
		mud.print( "#sfailed!\n> %s", err )
	end

	mud.event( "scriptsLoaded" )
end

local function closeScripts()
end

return {
	load = loadScripts,
	close = closeScripts,
}
