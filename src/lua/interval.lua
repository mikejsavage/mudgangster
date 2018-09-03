local Intervals = { }

local function doIntervals( loop, watcher )
	local now = loop:update_now()

	for i = 1, #Intervals do
		local event = Intervals[ i ]

		if event.enabled then
			event:checkTick( now )
		end
	end
end

function mud.now()
	return ev.Loop.default:update_now()
end

function mud.interval( callback, interval, disabled )
	enforce( callback, "callback", "function" )
	enforce( interval, "interval", "number" )

	local event = {
		callback = callback,
		interval = interval,
		nextTick = mud.now(),

		enabled = not disabled,

		enable = function( self )
			self.enabled = true
		end,
		disable = function( self )
			self.enabled = false
		end,

		tick = function( self, now )
			callback( now )

			self.nextTick = now + self.interval
		end,

		checkTick = function( self, now )
			if self.nextTick == -1 then
				self.nextTick = now
			end

			if now >= self.nextTick then
				self:tick( now )
			end
		end,

		tend = function( self, tolerance, desired )
			tolerance = tolerance or 0
			desired = desired or mud.now()

			local toTick = ( self.nextTick - desired ) % self.interval
			local sinceTick = self.interval - toTick

			if toTick <= tolerance then
				self.nextTick = math.avg( self.nextTick, desired )
			elseif sinceTick <= tolerance then
				self.nextTick = math.avg( self.nextTick, desired + self.interval )
			else
				self.nextTick = desired

				self:checkTick( mud.now() )
			end
		end,
	}

	table.insert( Intervals, event )

	return event
end

return {
	doIntervals = doIntervals,
}