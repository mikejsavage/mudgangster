local DataHandler
local mud_socket

local LastAddress
local LastPort

function mud.send( data )
	mud.last_command_time = mud.now()
	socket.send( mud_socket, data )
end

function mud.disconnect()
	mud.print( "\n#s> Disconnected!" )
	mud.connected = false
	socket.close( mud_socket )
	mud_socket = nil
end

function mud.connect( address, port )
	if mud.connected then
		mud.print( "\n#s> Already connected! (%s:%d)", LastAddress, LastPort )

		return
	end

	if not address then
		if not LastAddress then
			mud.print( "\n#s> I need an address..." )
			
			return
		end

		address = LastAddress
		port = LastPort
	end

	mud.print( "\n#s> Connecting to %s:%d...", address, port )

	local sock, err = socket.connect( address, port, function( sock, data )
		if data then
			DataHandler( data )
		else
			mud.disconnect()
		end
	end )

	if not sock then
		mud.print( "\n#s> Connection failed: %s", err )
		return
	end

	mud_socket = sock

	LastAddress = address
	LastPort = port

	mud.connected = true
	mud.last_command_time = mud.now()
end

mud.alias( "/con", {
	[ "^$" ] = function()
		mud.connect()
	end,

	[ "^(%S+)%s+(%d+)$" ] = function( address, port )
		mud.connect( address, port )
	end,
}, "<ip> <port>" )

mud.alias( "/dc", function()
	if mud.connected then
		mud.disconnect()
	else
		mud.print( "\n#s> You're not connected..." )
	end
end )

return {
	init = function( dataHandler )
		DataHandler = dataHandler
	end,
}
