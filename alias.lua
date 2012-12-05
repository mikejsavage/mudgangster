local Aliases = { }

local function doAlias( line )
	local command, args = line:match( "^%s*(%S+)%s*(.*)$" )
	local alias = Aliases[ command ]

	if alias and alias.enabled then
		local badSyntax = true

		for i = 1, #alias.callbacks do
			local callback = alias.callbacks[ i ]
			local _, subs = args:gsub( callback.pattern, callback.callback )

			if subs ~= 0 then
				badSyntax = false

				break
			end
		end

		if badSyntax then
			mud.print( "\nsyntax: %s %s" % { command, alias.syntax } )
		end

		return true
	end

	return false
end

local function simpleAlias( callback, disabled )
	return {
		callbacks = {
			{
				pattern = "^(.*)$",
				callback = callback,
			},
		},

		enabled = not disabled,

		enable = function( self )
			self.enabled = true
		end,
		disable = function( self )
			self.enabled = false
		end,
	}
end

local function patternAlias( callbacks, syntax, disabled )
	local alias = {
		callbacks = { },
		syntax = syntax,

		enabled = not disabled,

		enable = function( self )
			self.enabled = true
		end,
		disable = function( self )
			self.enabled = false
		end,
	}

	for pattern, callback in pairs( callbacks ) do
		table.insert( alias.callbacks, {
			pattern = pattern,
			callback = callback,
		} )
	end

	return alias
end

function mud.alias( command, handler, ... )
	enforce( command, "command", "string" )
	enforce( handler, "handler", "function", "string", "table" )

	assert( not Aliases[ command ], "alias `%s' already registered" % command )

	if type( handler ) == "string" then
		local command = handler

		handler = function( args )
			mud.input( command % args )
		end
	end

	local alias = type( handler ) == "function"
		and simpleAlias( handler, ... ) 
		or patternAlias( handler, ... )
	
	Aliases[ command ] = alias

	return alias
end

return {
	doAlias = doAlias,
}
