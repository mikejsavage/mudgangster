local DataHandler

local LastAddress
local LastPort

local loop = ev.Loop.default

function mud.disconnect()
	if not mud.connected then
		mud.print( "\n#s> You're not connected..." )

		return
	end

	mud.connected = false

	mud.kill()
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

	local sock = socket.tcp()
	sock:settimeout( 0 )
	sock:connect( address, port )

	mud.print( "\n#s> Connecting to %s:%d...", address, port )

	mud.connected = true
	mud.lastInput = mud.now()

	mud.send = function( data )
		mud.lastInput = mud.now()

		sock:send( data )
	end

	mud.kill = function()
		sock:shutdown()
	end

	ev.IO.new( function( loop, watcher )
		local _, err = sock:receive( "*a" )

		if err ~= "connection refused" then
			LastAddress = address
			LastPort = port

			ev.IO.new( function( loop, watcher )
				local _, err, data = sock:receive( "*a" )

				if not data then
					data = _
				end
				
				if data then
					DataHandler( data )
					mud.handleXEvents()
				end
				
				if err == "closed" then
					mud.print( "\n#s> Disconnected!" )

					watcher:stop( loop )
					sock:shutdown()

					mud.connected = false

					return
				end
			end, sock:getfd(), ev.READ ):start( loop )
		else
			mud.print( "\n#s> Refused!" )

			mud.connected = false
		end

		watcher:stop( loop )
	end, sock:getfd(), ev.WRITE ):start( loop )
end

mud.alias( "/con", {
	[ "^$" ] = function()
		mud.connect()
	end,

	[ "^(%S+)%s+(%d+)$" ] = function( address, port )
		mud.connect( address, port )
	end,
}, "<ip> <port>" )

mud.alias( "/dc", mud.disconnect )

return {
	init = function( dataHandler )
		DataHandler = dataHandler
	end,
}
