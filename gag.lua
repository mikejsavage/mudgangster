local Gags = { }
local AnsiGags = { }

local ChatGags = { }
local AnsiChatGags = { }

local doGags
local doAnsiGags

local doChatGags
local doAnsiChatGags

local function genericGags( gags )
	return
		function( pattern, disabled )
			local gag = {
				pattern = pattern,
				
				enabled = not disabled,

				enable = function( self )
					self.enabled = true
				end,
				disable = function( self )
					self.enabled = false
				end,
			}

			table.insert( gags, gag )

			return gag
		end,

		function( line )
			for i = 1, #gags do
				local gag = gags[ i ]

				if gag.enabled and line:find( gag.pattern ) then
					return true
				end
			end

			return false
		end
end

mud.gag,     doGags     = genericGags( Gags )
mud.gagAnsi, doAnsiGags = genericGags( AnsiGags )

mud.gagChat,     doChatGags     = genericGags( ChatGags )
mud.gagAnsiChat, doChatAnsiGags = genericGags( ChatAnsiGags )

return {
	doGags = doGags,
	doAnsiGags = doAnsiGags,

	doChatGags = doChatGags,
	doChatAnsiGags = doChatAnsiGags,
}
