local loop = ev.Loop.default

local handleChat

local CommandBytes = {
	all = "\4",
	pm = "\5",
	message = "\7",
	version = "\19",
}

local Clients = { }

local function clientFromName( name )
	local idx = tonumber( name )

	if idx then
		return Clients[ idx ]
	end

	for _, client in ipairs( Clients ) do
		if client.name:startsWith( name ) then
			return client
		end
	end

	return nil
end

local function dataCoro( client )
	local data = coroutine.yield()
	local name = data:match( "^YES:(.+)\n$" )

	if not name then
		client.socket:shutdown()

		return
	end

	client.name = name
	client.state = "connected"

	mud.print( "\n#s> Connected to %s@%s:%s", client.name, client.address, client.port )

	local dataBuffer = ""

	while true do
		dataBuffer = dataBuffer .. coroutine.yield()

		dataBuffer = dataBuffer:gsub( "(.)(.-)\255", function( command, args )
			if command == CommandBytes.all or command == CommandBytes.pm or command == CommandBytes.message then
				local message = args:match( "^\n*(.-)\n*$" )

				handleChat( message:gsub( "\r", "" ) )
			end

			return ""
		end )
	end
end

local Client = { }

function Client:new( socket, address, port )
	socket:setoption( "keepalive", true )

	local client = {
		socket = socket,

		address = address,
		port = port,

		state = "connecting",
		handler = coroutine.create( dataCoro ),
	}

	assert( coroutine.resume( client.handler, client ) )

	setmetatable( client, { __index = Client } )

	table.insert( Clients, client )

	socket:send( "CHAT:Hirve\n127.0.0.14050 " )

	return client
end

function Client:kill()
	if self.state == "killed" then
		return
	end

	mud.print( "\n#s> Disconnected from %s!", self.name )

	self.state = "killed"
	self.socket:shutdown()

	for i, client in ipairs( Clients ) do
		if client == self then
			table.remove( Clients, i )
			break
		end
	end
end

local function dataHandler( client, loop, watcher )
	local _, err, data = client.socket:receive( "*a" )

	if err == "closed" then
		client:kill()
		watcher:stop( loop )

		return
	end

	assert( coroutine.resume( client.handler, data ) )
	mud.handleXEvents()
end

function mud.chatns( form, ... )
	local named = "\nHirve" .. form:format( ... )
	local data = CommandBytes.all .. named:parseColours() .. "\n\255"

	for _, client in ipairs( Clients ) do
		if client.state == "connected" then
			client.socket:send( data )
		end
	end

	mud.printb( "#lr%s", named )
	handleChat()
end

function mud.chat( form, ... )
	mud.chatns( " " .. form, ... )
end

local function call( address, port )
	mud.print( "\n#s> Calling %s:%d...", address, port )

	local sock = socket.tcp()
	sock:settimeout( 0 )

	sock:connect( address, port )

	ev.IO.new( function( loop, watcher )
		local _, err = sock:receive( "*a" )

		if err ~= "connection refused" then
			local client = Client:new( sock, address, port )

			ev.IO.new( function( loop, watcher )
				dataHandler( client, loop, watcher )
			end, sock:getfd(), ev.READ ):start( loop )

			ev.IO.new( function( loop, watcher )
				client.socket:send( CommandBytes.version .. "MudGangster" .. "\255" )
				watcher:stop( loop )
			end, sock:getfd(), ev.WRITE ):start( loop )
		else
			mud.print( "\n#s> Failed to call %s:%d", address, port )
		end

		watcher:stop( loop )
	end, sock:getfd(), ev.WRITE ):start( loop )
end

mud.alias( "/call", {
	[ "^(%S+)$" ] = function( address )
		call( address, 4050 )
	end,

	[ "^(%S+)[%s:]+(%d+)$" ] = call,
}, "<address> [port]" )

mud.alias( "/hang", {
	[ "^(%S+)$" ] = function( name )
		local client = clientFromName( name )

		if client then
			client:kill()
		end
	end,
} )

local function sendPM( client, message )
	local named = "\nHirve chats to you, '" .. message .. "'"
	local data = CommandBytes.pm .. named .. "\n\255"

	client.socket:send( data )
end

mud.alias( "/silentpm", {
	[ "^(%S+)%s+(.-)$" ] = function( name, message )
		local client = clientFromName( name )

		if client then
			local coloured = message:parseColours()

			sendPM( client, coloured )
		end
	end,
} )

mud.alias( "/pm", {
	[ "^(%S+)%s+(.-)$" ] = function( name, message )
		local client = clientFromName( name )

		if client then
			local coloured = message:parseColours()

			sendPM( client, coloured )

			local toPrint = ( "You chat to %s, '" % client.name ) .. coloured .. "'"

			handleChat( toPrint )
		end
	end,
} )

mud.alias( "/emoteto", function( args )
end )

return {
	init = function( chatHandler )
		handleChat = chatHandler
	end,
}
