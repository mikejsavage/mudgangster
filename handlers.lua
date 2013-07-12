local action = require( "action" )
local alias = require( "alias" )
local gag = require( "gag" )
local macro = require( "macro" )
local sub = require( "sub" )
local interval = require( "interval" )

local GA = "\255\249"

local bold = false
local fg = 7
local bg = 0

local lastWasChat = false

local receiving = false
local showInput = true

local dataBuffer = ""
local pendingInputs = { }

local function echoOn()
	showInput = true

	return ""
end

local function echoOff()
	showInput = true

	return ""
end

local function setFG( colour )
	return function()
		fg = colour
	end
end

local Escapes = {
	m = {
		[ "0" ] = function()
			bold = false
			fg = 7
			bg = 0
		end,

		[ "1" ] = function()
			bold = true
		end,

		[ "30" ] = setFG( 0 ),
		[ "31" ] = setFG( 1 ),
		[ "32" ] = setFG( 2 ),
		[ "33" ] = setFG( 3 ),
		[ "34" ] = setFG( 4 ),
		[ "35" ] = setFG( 5 ),
		[ "36" ] = setFG( 6 ),
		[ "37" ] = setFG( 7 ),
	},
}

local function printPendingInputs()
	if lastWasChat then
		mud.newlineMain()

		lastWasChat = false
	end

	for i = 1, #pendingInputs do
		mud.printr( pendingInputs[ i ] )
	end

	pendingInputs = { }
end

local function handleChat( message )
	if not message then
		lastWasChat = true

		return
	end

	local oldFG = fg
	local oldBG = bg
	local oldBold = bold

	fg = 1
	bg = 0
	bold = true

	local noAnsi = message:gsub( "\27%[[%d;]*%a", "" )

	action.doChatPreActions( noAnsi )
	action.doChatAnsiPreActions( message )

	mud.newlineMain()
	mud.newlineChat()

	for line, newLine in message:gmatch( "([^\n]*)(\n?)" ) do
		for text, opts, escape in ( line .. "\27[m" ):gmatch( "(.-)\27%[([%d;]*)(%a)" ) do
			if text ~= "" then
				mud.printMain( text, fg, bg, bold )
				mud.printChat( text, fg, bg, bold )
			end

			for opt in opts:gmatch( "([^;]+)" ) do
				if Escapes[ escape ][ opt ] then
					Escapes[ escape ][ opt ]()
				end
			end
		end

		if newLine == "\n" then
			mud.newlineMain()
			mud.newlineChat()
		end
	end

	mud.drawMain()
	mud.drawChat()

	action.doChatActions( noAnsi )
	action.doChatAnsiActions( message )

	fg = oldFG
	bg = oldBG
	bold = oldBold

	lastWasChat = true
end

local function handleData( data )
	receiving = true

	dataBuffer = dataBuffer .. data

	if data:match( GA ) then
		dataBuffer = dataBuffer:gsub( "\r", "" )

		dataBuffer = dataBuffer:gsub( "\255\252\1", echoOn )
		dataBuffer = dataBuffer:gsub( "\255\251\1", echoOff )

		dataBuffer = dataBuffer:gsub( "\255\253\24", "" )
		dataBuffer = dataBuffer:gsub( "\255\253\31", "" )
		dataBuffer = dataBuffer:gsub( "\255\253\34", "" )

		dataBuffer = dataBuffer .. "\n"

		for line in dataBuffer:gmatch( "([^\n]*)\n" ) do
			if lastWasChat then
				mud.newlineMain()

				lastWasChat = false
			end

			local clean, subs = line:gsub( GA, "" )
			local hasGA = subs ~= 0

			local noAnsi = clean:gsub( "\27%[%d*%a", "" )

			local gagged = gag.doGags( noAnsi ) or gag.doAnsiGags( clean )

			action.doPreActions( noAnsi )
			action.doAnsiPreActions( clean )

			if not gagged then
				local subbed = sub.doSubs( clean )

				for text, opts, escape in ( subbed .. "\27[m" ):gmatch( "(.-)\27%[([%d;]*)(%a)" ) do
					if text ~= "" then
						mud.printMain( text, fg, bg, bold )
					end

					for opt in opts:gmatch( "([^;]+)" ) do
						if Escapes[ escape ][ opt ] then
							Escapes[ escape ][ opt ]()
						end
					end
				end

				if not hasGA then
					mud.newlineMain()
				end
			end

			if hasGA then
				printPendingInputs()
			end

			action.doActions( noAnsi )
			action.doAnsiActions( clean )
		end

		mud.drawMain()

		dataBuffer = ""
		receiving = false
	end
end

local function handleCommand( input, hide )
	if not alias.doAlias( input ) then
		if not mud.connected then
			mud.print( "\n#s> You're not connected..." )

			return
		end

		if not hide and input ~= "" then
			local toShow = ( showInput and input or ( "*" ):rep( input:len() ) ) .. "\n"

			if receiving then
				table.insert( pendingInputs, toShow )
			else
				if lastWasChat then
					mud.newlineMain()

					lastWasChat = false
				end

				mud.printr( toShow )
			end
		end

		mud.send( input .. "\n" )
		mud.drawMain()
	end 
end

local function handleInput( input, display )
	for command in ( input .. ";" ):gmatch( "([^;]*);" ) do
		handleCommand( command, display )
	end
end

mud.input = handleInput

local function handleMacro( key, shift, ctrl, alt )
	if shift then
		key = "s" .. key
	end

	if ctrl then
		key = "c" .. key
	end

	if alt then
		key = "a" .. key
	end

	macro.doMacro( key )
end

local function handleClose()
	ev.Loop.default:unloop()

	mud.event( "shutdown" )
end

return {
	data = handleData,
	chat = handleChat,
	input = handleInput,
	macro = handleMacro,
	interval = interval.doIntervals,
	close = handleClose,
}
