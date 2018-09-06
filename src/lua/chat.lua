local handleChat
local chat_name = "FromDaHood"

local CommandBytes = {
	name_change = "\1",
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

function Client:new( sock, address, port )
	local client = {
		socket = sock,

		address = address,
		port = port,

		state = "connecting",
		handler = coroutine.create( dataCoro ),
	}

	assert( coroutine.resume( client.handler, client ) )

	setmetatable( client, { __index = Client } )

	table.insert( Clients, client )

	socket.send( sock, "CHAT:%s\n127.0.0.14050 " % chat_name )

	return client
end

function Client:kill()
	if self.state == "killed" then
		return
	end

	mud.print( "\n#s> Disconnected from %s!", self.name )

	self.state = "killed"
	socket.close( self.socket )

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
end

function mud.chat_no_space( form, ... )
	local named = "\n" .. chat_name .. form:format( ... )
	local data = CommandBytes.all .. named:parseColours() .. "\n\255"

	for _, client in ipairs( Clients ) do
		if client.state == "connected" then
			socket.send( client.socket, data )
		end
	end

	mud.printb( "#lr%s", named )
end

function mud.chat( form, ... )
	mud.chatns( " " .. form, ... )
end

local function call( address, port )
	mud.print( "\n#s> Calling %s:%d...", address, port )

	local client
	local sock, err = socket.connect( address, port, function( sock, data )
		if data then
			assert( coroutine.resume( client.handler, data ) )
		else
			client:kill()
		end
	end )

	if not sock then
		mud.print( "\n#s> Connection failed: %s", err )
		return
	end

	client = Client:new( sock, address, port )
	socket.send( client.socket, CommandBytes.version .. "MudGangster" .. "\255" )
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

	socket.send( client.socket, data )
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
			mud.printb( "\n#lrYou chat to %s, \"%s\"", client.name, coloured )
		end
	end,
} )

mud.alias( "/emoteto", function( args )
end )

mud.alias( "/chatname", function( name )
	chat_name = name

	local message = CommandBytes.name_change .. chat_name .. "\255"
	for _, client in ipairs( Clients ) do
		if client.state == "connected" then
			socket.send( client.socket, message )
		end
	end

	mud.print( "\n#s> Chat name changed to %s", chat_name )
end )

return {
	init = function( chatHandler )
		handleChat = chatHandler
	end,
}
