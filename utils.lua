getmetatable( "" ).__mod = function( self, form )
	if type( form ) == "table" then
		return self:format( unpack( form ) )
	end

	return self:format( form )
end

function string.plural( count, plur, sing )
	return count == 1 and ( sing or "" ) or ( plur or "s" )
end

function string.startsWith( self, needle, ignoreCase )
	if ignoreCase then
		return self:lower():sub( 1, needle:len() ) == needle:lower()
	end

	return self:sub( 1, needle:len() ) == needle
end

function string.commas( num )
	num = tonumber( num )

	local out = ""

	while num >= 1000 do
		out = ( ",%03d%s" ):format( num % 1000, out )

		num = math.floor( num / 1000 )
	end

	return tostring( num ) .. out
end

function math.round( num )
	return math.floor( num + 0.5 )
end

function math.avg( a, b )
	return ( a + b ) / 2
end

function io.readable( path )
	local file, err = io.open( path, "r" )

	if not file then
		return false, err
	end

	io.close( file )

	return true
end

function enforce( var, name, ... )
	local acceptable = { ... }
	local ok = false

	for _, accept in ipairs( acceptable ) do
		if type( var ) == accept then
			ok = true
			
			break
		end
	end

	if not ok then
		error( "argument `%s' to %s should be of type %s (got %s)" % { name, debug.getinfo( 2, "n" ).name, table.concat( acceptable, " or " ), type( var ) }, 3 )
	end
end

local function genericPrint( message, main, chat )
	local bold = false
	local fg = 7
	local bg = 0

	local function setFG( colour )
		return function()
			fg = colour
		end
	end

	local Sequences = {
		d = setFG( 7 ),
		k = setFG( 0 ),
		r = setFG( 1 ),
		g = setFG( 2 ),
		y = setFG( 3 ),
		b = setFG( 4 ),
		m = setFG( 5 ),
		c = setFG( 6 ),
		w = setFG( 7 ),
		s = setFG( 8 ),
	}

	for line, newLine in message:gmatch( "([^\n]*)(\n?)" ) do
		for text, hashPos, light, colour, colourPos in ( line .. "#a" ):gmatch( "(.-)()#(l?)(%l)()" ) do
			if main then
				mud.printMain( text:gsub( "##", "#" ), fg, bg, bold )
			end

			if chat then
				mud.printChat( text:gsub( "##", "#" ), fg, bg, bold )
			end

			if line:sub( hashPos - 1, hashPos - 1 ) ~= "#" then
				if colourPos ~= line:len() + 3 then
					assert( Sequences[ colour ], "invalid colour sequence: #" .. light .. colour )

					bold = light == "l"
					Sequences[ colour ]()
				end
			else
				if main then
					mud.printMain( light .. colour, fg, bg, bold )
				end

				if chat then
					mud.printChat( light .. colour, fg, bg, bold )
				end
			end
		end

		if newLine ~= "" then
			if main then
				mud.newlineMain()
			end

			if chat then
				mud.newlineChat()
			end
		end
	end

	if main then
		mud.drawMain()
	end

	if chat then
		mud.drawChat()
	end

	mud.handleXEvents()
end

function mud.print( form, ... )
	genericPrint( form:format( ... ), true, false )
end

function mud.printc( form, ... )
	genericPrint( form:format( ... ), false, true )
end

function mud.printb( form, ... )
	genericPrint( form:format( ... ), true, true )
end

function mud.printr( str, fg, bg, bold )
	fg = fg or 7
	bg = bg or 0
	bold = bold or false

	for line, newLine in str:gmatch( "([^\n]*)(\n?)" ) do
		mud.printMain( line, fg, bg, bold )

		if newLine ~= "" then
			mud.newlineMain()
		end
	end

	mud.drawMain()
end

function mud.enable( ... )
	local things = { ... }

	if not things[ 1 ].enable then
		things = things[ 1 ]
	end

	for _, thing in ipairs( things ) do
		thing:enable()
	end
end

function mud.disable( ... )
	local things = { ... }

	if not things[ 1 ].enable then
		things = things[ 1 ]
	end

	for _, thing in ipairs( things ) do
		thing:disable()
	end
end

local ColourSequences = {
	d = 0,
	k = 30,
	r = 31,
	g = 32,
	y = 33,
	b = 34,
	m = 35,
	c = 36,
	w = 37,
}

function string.parseColours( message )
	message = assert( tostring( message ) )

	return ( message:gsub( "()#(l?)(%l)", function( patternPos, bold, sequence )
		if message:sub( patternPos - 1, patternPos - 1 ) == "#" then
			return
		end

		if bold == "l" then
			return "\27[1m\27[%dm" % { ColourSequences[ sequence ] }
		end

		return "\27[0m\27[%dm" % { ColourSequences[ sequence ] }
	end ):gsub( "##", "#" ) )
end
