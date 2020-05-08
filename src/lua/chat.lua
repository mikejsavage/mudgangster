local lpeg = require( "lpeg" )

local handleChat
local chatName = "FromDaHood"

local CommandBytes = {
	nameChange = "\1",
	all = "\4",
	pm = "\5",
	message = "\7",
	version = "\19",
}

local Chats = { }

local parser
do
	local command = lpeg.P( 1 )
	local terminator = lpeg.P( "\255" )
	local contents = ( 1 - terminator ) ^ 0
	parser = lpeg.C( command ) * lpeg.C( contents ) * terminator
end

local function chatFromName( name )
	local idx = tonumber( name )

	if idx then
		return Chats[ idx ]
	end

	for _, chat in ipairs( Chats ) do
		if chat.name:startsWith( name ) then
			return chat
		end
	end

	return nil
end

local function killChat( chat )
	if chat.state == "killed" then
		return
	end

	mud.print( "\n#s> Disconnected from %s!", chat.name )

	chat.state = "killed"
	socket.close( chat.socket )

	for i, other in ipairs( Chats ) do
		if other == chat then
			table.remove( Chats, i )
			break
		end
	end
end

local function dataCoro( chat )
	local handshake = coroutine.yield()
	local name, handshakeLen = handshake:match( "^YES:(.+)\n()" )

	if not name then
		killChat( chat )
		return
	end

	chat.name = name
	chat.state = "connected"

	mud.print( "\n#s> Connected to %s@%s:%s", chat.name, chat.address, chat.port )

	local data = handshake:sub( handshakeLen )

	while true do
		while true do
			local command, args = parser:match( data )
			if not command then
				break
			end

			data = data:sub( args:len() + 3 )

			if command == CommandBytes.all or command == CommandBytes.pm or command == CommandBytes.message then
				local message = args:match( "^\n*(.-)\n*$" )

				handleChat( message:gsub( "\r", "" ) )
			end
		end

		data = data .. coroutine.yield()
	end
end

local function newChat( sock, address, port )
	local chat = {
		name = address .. ":" .. port,
		socket = sock,

		address = address,
		port = port,

		state = "connecting",
		handler = coroutine.create( dataCoro ),
	}

	assert( coroutine.resume( chat.handler, chat ) )

	table.insert( Chats, chat )

	socket.send( sock, "CHAT:%s\n127.0.0.14050 " % chatName )

	return chat
end

local function dataHandler( chat, loop, watcher )
	local _, err, data = chat.socket:receive( "*a" )

	if err == "closed" then
		killChat( chat )
		watcher:stop( loop )

		return
	end

	assert( coroutine.resume( chat.handler, data ) )
end

function mud.chat_no_space( form, ... )
	local named = chatName .. form:format( ... )
	local data = CommandBytes.all .. named:parseColours() .. "\n\255"

	for _, chat in ipairs( Chats ) do
		if chat.state == "connected" then
			socket.send( chat.socket, data )
		end
	end

	mud.printb( "\n#lr%s", named )
	handleChat()
end

function mud.chat( form, ... )
	mud.chat_no_space( " " .. form, ... )
end

local function call( address, port )
	mud.print( "\n#s> Calling %s:%d...", address, port )

	local chat
	local sock, err = socket.connect( address, port, function( sock, data )
		if data then
			assert( coroutine.resume( chat.handler, data ) )
		else
			killChat( chat )
		end
	end )

	if not sock then
		mud.print( "\n#s> Connection failed: %s", err )
		return
	end

	chat = newChat( sock, address, port )
	socket.send( chat.socket, CommandBytes.version .. "MudGangster" .. "\255" )
end

mud.alias( "/call", {
	[ "^(%S+)$" ] = function( address )
		call( address, 4050 )
	end,

	[ "^(%S+)[%s:]+(%d+)$" ] = call,
}, "<address> [port]" )

mud.alias( "/hang", {
	[ "^(%S+)$" ] = function( name )
		local chat = chatFromName( name )
		if chat then
			killChat( chat )
		end
	end,
} )

local function sendPM( chat, message )
	local named = chatName .. " chats to you, '" .. message .. "'"
	local data = CommandBytes.pm .. named .. "\n\255"

	socket.send( chat.socket, data )
end

mud.alias( "/silentpm", {
	[ "^(%S+)%s+(.-)$" ] = function( name, message )
		local chat = chatFromName( name )

		if chat then
			local coloured = message:parseColours()
			sendPM( chat, coloured )
		end
	end,
} )

mud.alias( "/pm", {
	[ "^(%S+)%s+(.-)$" ] = function( name, message )
		local chat = chatFromName( name )

		if chat then
			local coloured = message:parseColours()
			sendPM( chat, coloured )
			mud.printb( "\n#lrYou chat to %s, \"%s\"", chat.name, coloured )
			handleChat()
		end
	end,
} )

mud.alias( "/emoteto", function( args )
end )

mud.alias( "/chatname", function( name )
	chatName = name

	local message = CommandBytes.nameChange .. chatName .. "\255"
	for _, chat in ipairs( Chats ) do
		if chat.state == "connected" then
			socket.send( chat.socket, message )
		end
	end

	mud.print( "\n#s> Chat name changed to %s", chatName )
	handleChat()
end )

return {
	init = function( chatHandler )
		handleChat = chatHandler
	end,
}
