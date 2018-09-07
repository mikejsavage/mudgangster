local Subs = { }

local function doSubs( line )
	for i = 1, #Subs do
		local sub = Subs[ i ]

		if sub.enabled then
			local ok, newLine, subs = pcall( string.gsub, line, sub.pattern, sub.replacement )

			if not ok then
				mud.print( "\n#s> sub failed: %s\n%s\n%s", newLine, sub.pattern, sub.replacement )
				return line
			end

			if subs ~= 0 then
				return newLine
			end
		end
	end

	return line
end

function mud.sub( pattern, replacement, disabled )
	enforce( pattern, "pattern", "string" )
	enforce( replacement, "replacement", "string", "function" )

	local sub = {
		pattern = pattern,
		replacement = replacement,

		enabled = not disabled,

		enable = function( self )
			self.enabled = true
		end,
		disable = function( self )
			self.enabled = false
		end,
	}

	table.insert( Subs, sub )

	return sub
end

return {
	doSubs = doSubs,
}
