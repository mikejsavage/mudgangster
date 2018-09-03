local Events = { }

function mud.listen( name, callback, disabled )
	enforce( name, "name", "string" )
	enforce( callback, "callback", "function" )

	local event = {
		callback = callback,

		enabled = not disabled,

		enable = function( self )
			self.enabled = true
		end,
		disable = function( self )
			self.enabled = false
		end,
	}

	if not Events[ name ] then
		Events[ name ] = { }
	end

	table.insert( Events[ name ], event )

	return event
end

function mud.event( name, ... )
	enforce( name, "name", "string" )

	if Events[ name ] then
		for _, event in ipairs( Events[ name ] ) do
			if event.enabled then
				event.callback( ... )
			end
		end
	end
end
