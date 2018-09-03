mud = {
	connected = false,
}

-- local function luarocks_path( opt, orig )
-- 	local pipe = io.popen( "luarocks path " .. opt, "r" )
-- 	if not pipe then
-- 		return nil
-- 	end
--
-- 	local contents = pipe:read( "*all" )
-- 	pipe:close()
--
-- 	if not contents then
-- 		return orig
-- 	end
--
-- 	return contents:gsub( "%s*$", "" ) .. ";" .. orig
-- end
--
-- package.path = luarocks_path( "--lr-path", package.path )
-- package.cpath = luarocks_path( "--lr-cpath", package.cpath )

table.unpack = table.unpack or unpack

require( "utils" )

socket = require( "socket" )
ev = require( "ev" )

require( "event" )

local script = require( "script" )
local handlers = require( "handlers" )

require( "chat" ).init( handlers.chat )
require( "connect" ).init( handlers.data )

local xFD, handleXEvents,
	printMain, newlineMain, drawMain,
	printChat, newlineChat, drawChat,
	setHandlers, urgent, setStatus = ...

mud.printMain = printMain
mud.newlineMain = newlineMain
mud.drawMain = drawMain

mud.printChat = printChat
mud.newlineChat = newlineChat
mud.drawChat = drawChat

mud.urgent = urgent

require( "status" ).init( setStatus )

setHandlers( handlers.input, handlers.macro, handlers.close )

local loop = ev.Loop.default

mud.handleXEvents = handleXEvents

ev.IO.new( handleXEvents, xFD, ev.READ ):start( loop )
ev.Timer.new( handlers.interval, 0.5, 0.5 ):start( loop )

script.load()

loop:loop()

script.close()
