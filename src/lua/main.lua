mud = {
	connected = false,
	os = package.config:sub( 1, 1 ) == "\\" and "windows" or "linux"
}

table.unpack = table.unpack or unpack

require( "utils" )

require( "event" )

local script = require( "script" )
local handlers = require( "handlers" )

local printMain, newlineMain, printChat, newlineChat,
	setHandlers, urgent, setStatus,
	sock_connect, sock_send, sock_close,
	get_time, set_font,
	exe_path = ...

local socket_api = {
	connect = sock_connect,
	send = sock_send,
	close = sock_close,
}

local socket_data_handler = require( "socket" ).init( socket_api )

mud.printMain = printMain
mud.newlineMain = newlineMain
mud.printChat = printChat
mud.newlineChat = newlineChat

mud.urgent = urgent
mud.now = get_time

mud.last_human_input_time = mud.now()

require( "mud" ).init( handlers.data )
require( "chat" ).init( handlers.chat )

mud.alias( "/font", {
	[ "^(.-)%s+(%d+)$" ] = function( name, size )
		size = tonumber( size )
		if not set_font( name, size ) then
			mud.print( "\n#s> Couldn't set font" )
		end
	end,
}, "<font name> <font size>" )

require( "status" ).init( setStatus )

setHandlers( handlers.input, handlers.macro, handlers.close, socket_data_handler, handlers.interval )

script.load( exe_path )
