local socket_api
local data_callbacks = { }

local function connect( addr, port, cb )
	local sock, err = socket_api.connect( addr, port )
	if not sock then
		return nil, err
	end
	data_callbacks[ sock ] = cb
	return sock
end

local function close( sock )
	socket_api.close( sock )
	data_callbacks[ sock ] = nil
end

local function on_socket_data( sock, data )
	-- data could be like { type = "data/failed/close/etc", data = ... }
	-- need a failed to handle async connect
	data_callbacks[ sock ]( sock, data )
end

return {
	init = function( api )
		socket_api = api

		socket = {
			connect = connect,
			send = socket_api.send,
			close = close,
		}

		return on_socket_data
	end,
}
