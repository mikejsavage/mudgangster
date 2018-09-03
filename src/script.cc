#include <stdlib.h>
#include <assert.h>

#include <lua.hpp>

#include <X11/Xutil.h>

#include "common.h"
#include "ui.h"

static lua_State * lua;

static int inputHandlerIdx = LUA_NOREF;
static int macroHandlerIdx = LUA_NOREF;
static int closeHandlerIdx = LUA_NOREF;

void script_handleInput( const char * buffer, int len ) {
	assert( inputHandlerIdx != LUA_NOREF );

	lua_rawgeti( lua, LUA_REGISTRYINDEX, inputHandlerIdx );
	lua_pushlstring( lua, buffer, len );

	lua_call( lua, 1, 0 );
}

void script_doMacro( const char * key, int len, bool shift, bool ctrl, bool alt ) {
	lua_rawgeti( lua, LUA_REGISTRYINDEX, macroHandlerIdx );

	lua_pushlstring( lua, key, len );

	lua_pushboolean( lua, shift );
	lua_pushboolean( lua, ctrl );
	lua_pushboolean( lua, alt );

	lua_call( lua, 4, 0 );
}

void script_handleClose() {
	assert( closeHandlerIdx != LUA_NOREF );

	lua_rawgeti( lua, LUA_REGISTRYINDEX, closeHandlerIdx );

	lua_call( lua, 0, 0 );
}

// namespace {

extern "C" int mud_handleXEvents( lua_State * ) {
	ui_handleXEvents();
	return 0;
}

static void generic_print( TextBox * tb, lua_State * L ) {
	const char* str = luaL_checkstring( L, 1 );
	size_t len = lua_objlen( L, 1 );

	Colour fg = Colour( luaL_checkinteger( L, 2 ) );
	Colour bg = Colour( luaL_checkinteger( L, 3 ) );
	bool bold = lua_toboolean( L, 4 );

	textbox_add( tb, str, len, fg, bg, bold );
}

extern "C" int mud_printMain( lua_State * L ) {
	generic_print( &UI.textMain, L );
	return 0;
}

extern "C" int mud_newlineMain( lua_State * L ) {
	textbox_newline( &UI.textMain );
	return 0;
}

extern "C" int mud_drawMain( lua_State * L ) {
	textbox_draw( &UI.textMain );
	return 0;
}

extern "C" int mud_printChat( lua_State * L ) {
	generic_print( &UI.textChat, L );
	return 0;
}

extern "C" int mud_newlineChat( lua_State * L ) {
	textbox_newline( &UI.textChat );
	return 0;
}

extern "C" int mud_drawChat( lua_State * L ) {
	textbox_draw( &UI.textChat );
	return 0;
}

extern "C" int mud_setStatus( lua_State * L ) {
	luaL_argcheck( L, lua_type( L, 1 ) == LUA_TTABLE, 1, "expected function" );

	size_t len = lua_objlen( L, 1 );

	ui_statusClear();

	for( size_t i = 0; i < len; i++ )
	{
		lua_pushnumber( L, i + 1 );
		lua_gettable( L, 1 );

		lua_pushliteral( L, "text" );
		lua_gettable( L, 2 );
		size_t seglen;
		const char* str = lua_tolstring( L, -1, &seglen );

		lua_pushliteral( L, "fg" );
		lua_gettable( L, 2 );
		const Colour fg = Colour( lua_tointeger( L, -1 ) );

		lua_pushliteral( L, "fg" );
		lua_gettable( L, 2 );
		const bool bold = lua_toboolean( L, -1 );

		for( size_t j = 0; j < seglen; j++ ) {
			ui_statusAdd( str[ j ], fg, bold );
		}

		lua_pop( L, 4 );
	}

	ui_statusDraw();

	return 0;
}

extern "C" int mud_setHandlers( lua_State * L ) {
	luaL_argcheck( L, lua_type( L, 1 ) == LUA_TFUNCTION, 1, "expected function" );
	luaL_argcheck( L, lua_type( L, 2 ) == LUA_TFUNCTION, 2, "expected function" );
	luaL_argcheck( L, lua_type( L, 3 ) == LUA_TFUNCTION, 3, "expected function" );

	closeHandlerIdx = luaL_ref( L, LUA_REGISTRYINDEX );
	macroHandlerIdx = luaL_ref( L, LUA_REGISTRYINDEX );
	inputHandlerIdx = luaL_ref( L, LUA_REGISTRYINDEX );

	return 0;
}

extern "C" int mud_urgent( lua_State * L ) {
	if( !UI.hasFocus ) {
		XWMHints* hints = XGetWMHints( UI.display, UI.window );
		hints->flags |= XUrgencyHint;
		XSetWMHints( UI.display, UI.window, hints );
		XFree( hints );
	}

	return 0;
}

// } // anon namespace

void script_init() {
	mud_handleXEvents( NULL ); // TODO: why is this here?

	lua = lua_open();
	luaL_openlibs( lua );

	lua_getglobal( lua, "debug" );
	lua_getfield( lua, -1, "traceback" );
	lua_remove( lua, -2 );

	if( luaL_loadfile( lua, "main.lua" ) )
	{
		printf( "Error reading main.lua: %s\n", lua_tostring( lua, -1 ) );

		exit( 1 );
	}

	lua_pushinteger( lua, ConnectionNumber( UI.display ) );
	lua_pushcfunction( lua, mud_handleXEvents );

	lua_pushcfunction( lua, mud_printMain );
	lua_pushcfunction( lua, mud_newlineMain );
	lua_pushcfunction( lua, mud_drawMain );

	lua_pushcfunction( lua, mud_printChat );
	lua_pushcfunction( lua, mud_newlineChat );
	lua_pushcfunction( lua, mud_drawChat );

	lua_pushcfunction( lua, mud_setHandlers );

	lua_pushcfunction( lua, mud_urgent );

	lua_pushcfunction( lua, mud_setStatus );

	if( lua_pcall( lua, 11, 0, -13 ) )
	{
		printf( "Error running main.lua: %s\n", lua_tostring( lua, -1 ) );

		exit( 1 );
	}
}

void script_end() {
	lua_close( lua );
}
