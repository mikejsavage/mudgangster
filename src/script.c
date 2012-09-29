#include <stdlib.h>
#include <assert.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "common.h"
#include "ui.h"

static lua_State* L;

static int inputHandlerIdx = LUA_NOREF;
static int macroHandlerIdx = LUA_NOREF;
static int closeHandlerIdx = LUA_NOREF;

void script_handleInput( char* buffer, int len )
{
	assert( inputHandlerIdx != LUA_NOREF );

	lua_rawgeti( L, LUA_REGISTRYINDEX, inputHandlerIdx );
	lua_pushlstring( L, buffer, len );

	lua_call( L, 1, 0 );
}

void script_doMacro( char* key, int len, bool shift, bool ctrl, bool alt )
{
	lua_rawgeti( L, LUA_REGISTRYINDEX, macroHandlerIdx );

	lua_pushlstring( L, key, len );

	lua_pushboolean( L, shift );
	lua_pushboolean( L, ctrl );
	lua_pushboolean( L, alt );

	lua_call( L, 4, 0 );
}

void script_handleClose()
{
	assert( closeHandlerIdx != LUA_NOREF );

	lua_rawgeti( L, LUA_REGISTRYINDEX, closeHandlerIdx );

	lua_call( L, 0, 0 );
}

static int mud_handleXEvents( lua_State* L )
{
	PRETEND_TO_USE( L );

	ui_handleXEvents();

	return 0;
}

static int mud_printMain( lua_State* L )
{
	const char* str = luaL_checkstring( L, 1 );
	size_t len = lua_objlen( L, 1 );

	Colour fg = luaL_checkint( L, 2 );
	Colour bg = luaL_checkint( L, 3 );
	bool bold = lua_toboolean( L, 4 );

	textbox_add( UI.textMain, str, len, fg, bg, bold );

	return 0;
}

static int mud_newlineMain( lua_State* L )
{
	PRETEND_TO_USE( L );

	textbox_newline( UI.textMain );

	return 0;
}

static int mud_drawMain( lua_State* L )
{
	PRETEND_TO_USE( L );

	textbox_draw( UI.textMain );

	return 0;
}

static int mud_printChat( lua_State* L )
{
	const char* str = luaL_checkstring( L, 1 );
	size_t len = lua_objlen( L, 1 );

	Colour fg = luaL_checkint( L, 2 );
	Colour bg = luaL_checkint( L, 3 );
	bool bold = lua_toboolean( L, 4 );

	textbox_add( UI.textChat, str, len, fg, bg, bold );

	return 0;
}

static int mud_newlineChat( lua_State* L )
{
	PRETEND_TO_USE( L );

	textbox_newline( UI.textChat );

	return 0;
}

static int mud_drawChat( lua_State* L )
{
	PRETEND_TO_USE( L );

	textbox_draw( UI.textChat );

	return 0;
}

static int mud_setHandlers( lua_State* L )
{
	luaL_argcheck( L, lua_type( L, 1 ) == LUA_TFUNCTION, 1, "expected function" );
	luaL_argcheck( L, lua_type( L, 2 ) == LUA_TFUNCTION, 2, "expected function" );
	luaL_argcheck( L, lua_type( L, 3 ) == LUA_TFUNCTION, 3, "expected function" );

	closeHandlerIdx = luaL_ref( L, LUA_REGISTRYINDEX );
	macroHandlerIdx = luaL_ref( L, LUA_REGISTRYINDEX );
	inputHandlerIdx = luaL_ref( L, LUA_REGISTRYINDEX );

	return 0;
}

void script_init()
{
	mud_handleXEvents( NULL );

	L = lua_open();
	luaL_openlibs( L );

	lua_getglobal( L, "debug" );
	lua_getfield( L, -1, "traceback" );
	lua_remove( L, -2 );

	if( luaL_loadfile( L, "main.lua" ) )
	{
		printf( "Error reading main.lua: %s\n", lua_tostring( L, -1 ) );

		exit( 1 );
	}

	lua_pushinteger( L, ConnectionNumber( UI.display ) );
	lua_pushcfunction( L, mud_handleXEvents );

	lua_pushcfunction( L, mud_printMain );
	lua_pushcfunction( L, mud_newlineMain );
	lua_pushcfunction( L, mud_drawMain );

	lua_pushcfunction( L, mud_printChat );
	lua_pushcfunction( L, mud_newlineChat );
	lua_pushcfunction( L, mud_drawChat );

	lua_pushcfunction( L, mud_setHandlers );

	if( lua_pcall( L, 9, 0, -11 ) )
	{
		printf( "Error running main.lua: %s\n", lua_tostring( L, -1 ) );

		exit( 1 );
	}
}

void script_end()
{
	lua_close( L );
}
