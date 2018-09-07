local Macros = { }

local function doMacro( key )
	local macro = Macros[ key ]

	if macro then
		local ok, err = xpcall( macro, debug.traceback )
		if not ok then
			mud.print( "\n#s> macro callback failed: %s", err )
		end
	end
end

function mud.macro( key, callback, disabled )
	enforce( key, "key", "string" )
	enforce( callback, "callback", "function", "string" )

	if type( callback ) == "string" then
		local command = callback

		callback = function()
			mud.input( command )
		end
	end

	assert( not Macros[ key ], "macro `%s' already registered" % key )


	Macros[ key ] = callback
end

return {
	doMacro = doMacro,
}
