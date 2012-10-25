local Actions = { }
local PreActions = { }
local AnsiActions = { }
local AnsiPreActions = { }

local ChatActions = { }
local ChatPreActions = { }
local ChatAnsiActions = { }
local ChatAnsiPreActions = { }

local doActions
local doPreActions
local doAnsiActions
local doAnsiPreActions

local doChatActions
local doChatPreActions
local doChatAnsiActions
local doChatAnsiPreActions

local function genericActions( actions )
	return
		function( pattern, callback, disabled )
			enforce( pattern, "pattern", "string" )
			enforce( callback, "callback", "function", "string" )

			if type( callback ) == "string" then
				local command = callback

				callback = function()
					mud.input( command, false )
				end
			end

			local action = {
				pattern = pattern,
				callback = callback,

				enabled = not disabled,

				enable = function( self )
					self.enabled = true
				end,
				disable = function( self )
					self.enabled = false
				end,
			}

			table.insert( actions, action )

			return action
		end,

		function( line )
			for i = 1, #actions do
				local action = actions[ i ]

				if action.enabled then
					local ok, err = pcall( string.gsub, line, action.pattern, action.callback )

					if not ok then
						mud.print( debug.traceback( "\n#s> action callback failed: %s" % err ) )
					end
				end
			end
		end
end

mud.action,        doActions        = genericActions( Actions )
mud.preAction,     doPreActions     = genericActions( PreActions )
mud.ansiAction,    doAnsiActions    = genericActions( AnsiActions )
mud.ansiPreAction, doAnsiPreActions = genericActions( AnsiPreActions )

mud.chatAction,        doChatActions        = genericActions( ChatActions )
mud.preChatAction,     doChatPreActions     = genericActions( ChatPreActions )
mud.ansiChatAction,    doChatAnsiActions    = genericActions( ChatAnsiActions )
mud.ansiPreChatAction, doChatAnsiPreActions = genericActions( ChatAnsiPreActions )

return {
	doActions = doActions,
	doPreActions = doPreActions,
	doAnsiActions = doAnsiActions,
	doAnsiPreActions = doAnsiPreActions,

	doChatActions = doChatActions,
	doChatPreActions = doChatPreActions,
	doChatAnsiActions = doChatAnsiActions,
	doChatAnsiPreActions = doChatAnsiPreActions,
}
