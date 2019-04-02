local action = require( "action" )
local alias = require( "alias" )
local intercept = require( "intercept" )
local gag = require( "gag" )
local macro = require( "macro" )
local sub = require( "sub" )
local interval = require( "interval" )

local lpeg = require( "lpeg" )

local GA = "\255\249"

local bold = false
local fg = 7
local bg = 0

local lastWasChat = false
local lastWasGA = false

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

local function setBG( colour )
	return function()
		bg = colour
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

		[ "40" ] = setBG( 0 ),
		[ "41" ] = setBG( 1 ),
		[ "42" ] = setBG( 2 ),
		[ "43" ] = setBG( 3 ),
		[ "44" ] = setBG( 4 ),
		[ "45" ] = setBG( 5 ),
		[ "46" ] = setBG( 6 ),
		[ "47" ] = setBG( 7 ),
	},
}

local function printPendingInputs()
	if lastWasChat then
		mud.newlineMain()

		lastWasChat = false
	end

	for i = 1, #pendingInputs do
		mud.printr( pendingInputs[ i ] )

		lastWasGA = false
	end

	pendingInputs = { }
end

local function handleChat( message )
	lastWasChat = true
	lastWasGA = false

	if not message then
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

	action.doChatActions( noAnsi )
	action.doChatAnsiActions( message )

	fg = oldFG
	bg = oldBG
	bold = oldBold
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
			if lastWasGA then
				if line ~= "" then
					mud.newlineMain()
				end

				lastWasGA = false
			end

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

			local subbed = sub.doSubs( clean )

			for text, opts, escape in ( subbed .. "\27[m" ):gmatch( "(.-)\27%[([%d;]*)(%a)" ) do
				if text ~= "" and not gagged then
					mud.printMain( text, fg, bg, bold )
				end

				for opt in opts:gmatch( "([^;]+)" ) do
					if Escapes[ escape ] and Escapes[ escape ][ opt ] then
						Escapes[ escape ][ opt ]()
					end
				end
			end

			if hasGA then
				lastWasGA = true
				printPendingInputs()
			else
				if not gagged then
					mud.newlineMain()
				end
			end

			action.doActions( noAnsi )
			action.doAnsiActions( clean )
		end

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

		intercept.doIntercept( input )

		if not hide and input ~= "" then
			local toShow = ( showInput and input or ( "*" ):rep( input:len() ) ) .. "\n"

			if receiving then
				table.insert( pendingInputs, toShow )
			else
				if lastWasChat then
					mud.newlineMain()

					lastWasChat = false
				end

				lastWasGA = false

				mud.printr( toShow )
			end
		end

		mud.send( input .. "\n" )
	end 
end

local function handleInput( input, display )
	mud.last_human_input_time = mud.now()

	for command in ( input .. ";" ):gmatch( "([^;]*);" ) do
		handleCommand( command, display )
	end
end

mud.input = handleInput

local function handleMacro( key, shift, ctrl, alt )
	mud.last_human_input_time = mud.now()

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
	mud.event( "shutdown" )
end

return {
	data = handleData,
	chat = handleChat,
	input = handleInput,
	macro = handleMacro,
	interval = interval.doIntervals,
	socket = handleSocketData,
	close = handleClose,
}
