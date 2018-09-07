local Intercepts = { }

local function doIntercept( command )
	for i = 1, #Intercepts do
		local intercept = Intercepts[ i ]

		if intercept.enabled then
			local ok, err = xpcall( string.gsub, debug.traceback, command, intercept.pattern, intercept.callback )

			if not ok then
				mud.print( "\n#s> intercept callback failed: %s", err )
			end
		end
	end
end

function mud.intercept( pattern, callback, disabled )
	enforce( pattern, "pattern", "string" )
	enforce( callback, "callback", "function" )
	enforce( disabled, "disabled", "boolean", "nil" )

	local intercept = {
		pattern = pattern,
		callback = callback,

		enabled = not disabled,

		enable = function( self )
			self.enabled = true
		end,
		disable = function( self )
			self.enabled = false
		end,
	}
	
	table.insert( Intercepts, intercept )

	return intercept
end

return {
	doIntercept = doIntercept,
}
