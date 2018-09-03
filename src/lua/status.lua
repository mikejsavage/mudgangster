local setStatus
local barItems = { }

local function drawBar()
	local status = { }

	for _, item in ipairs( barItems ) do
		local fg = 7
		local bold = false

		for text, opts, escape in ( item.text .. "\27[m" ):gmatch( "(.-)\27%[([%d;]*)(%a)" ) do
			if text ~= "" then
				table.insert( status, {
					text = text,
					fg = fg,
					bold = bold,
				} )
			end

			for opt in opts:gmatch( "([^;]+)" ) do
				if opt == "0" then
					fg = 7
					bold = false
				elseif opt == "1" then
					bold = true
				else
					opt = tonumber( opt )

					if opt and opt >= 30 and opt <= 37 then
						fg = opt - 30
					end
				end
			end
		end
	end

	setStatus( status )

	mud.handleXEvents()
end

function mud.bar( priority )
	local item = {
		priority = priority,
		text = "",

		set = function( self, form, ... )
			enforce( form, "form", "string" )

			self.text = string.parseColours( form:format( ... ) )
			drawBar()
		end,
	}

	table.insert( barItems, item )
	table.sort( barItems, function( a, b )
		return a.priority < b.priority
	end )

	return item
end

return {
	init = function( set )
		setStatus = set
	end,
}
