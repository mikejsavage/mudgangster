mud = {
	connected = false,
}

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
	setHandlers, urgent = ...

mud.printMain = printMain
mud.newlineMain = newlineMain
mud.drawMain = drawMain

mud.printChat = printChat
mud.newlineChat = newlineChat
mud.drawChat = drawChat

mud.urgent = urgent

setHandlers( handlers.input, handlers.macro, handlers.close )

local loop = ev.Loop.default

mud.handleXEvents = handleXEvents

ev.IO.new( handleXEvents, xFD, ev.READ ):start( loop )
ev.Timer.new( handlers.interval, 0.5, 0.5 ):start( loop )

script.load()

loop:loop()

script.close()
