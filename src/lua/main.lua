mud = {
	connected = false,
}

table.unpack = table.unpack or unpack

require( "utils" )

require( "event" )

local script = require( "script" )
local handlers = require( "handlers" )

local handleXEvents,
	printMain, newlineMain, drawMain,
	printChat, newlineChat, drawChat,
	setHandlers, urgent, setStatus,
	sock_connect, sock_send, sock_close,
	get_time = ...

local socket_api = {
	connect = sock_connect,
	send = sock_send,
	close = sock_close,
}

local socket_data_handler = require( "socket" ).init( socket_api )

require( "mud" ).init( handlers.data )
require( "chat" ).init( handlers.chat )

mud.printMain = printMain
mud.newlineMain = newlineMain
mud.drawMain = drawMain

mud.printChat = printChat
mud.newlineChat = newlineChat
mud.drawChat = drawChat

mud.urgent = urgent
mud.now = get_time

require( "status" ).init( setStatus )

setHandlers( handlers.input, handlers.macro, handlers.close, socket_data_handler, handlers.interval )

mud.handleXEvents = handleXEvents

script.load()
