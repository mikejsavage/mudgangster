#include "common.h"
#include "platform.h"
#include "ui.h"

#include "platform_time.h"

#if PLATFORM_WINDOWS
#include "libs/lua/lua.hpp"
#else
#include <lua.hpp>
#endif

#if LUA_VERSION_NUM < 502
#define luaL_len lua_objlen
#endif

static const uint8_t lua_bytecode[] = {
#include "../build/lua_bytecode.h"
};

static lua_State * lua;

static int inputHandlerIdx = LUA_NOREF;
static int macroHandlerIdx = LUA_NOREF;
static int closeHandlerIdx = LUA_NOREF;
static int socketHandlerIdx = LUA_NOREF;
static int intervalHandlerIdx = LUA_NOREF;

static void pcall( int args, const char * err ) {
	if( lua_pcall( lua, args, 0, 1 ) ) {
		printf( "%s: %s\n", err, lua_tostring( lua, -1 ) );
		exit( 1 );
	}

	assert( lua_gettop( lua ) == 1 );
}

void script_handleInput( const char * buffer, int len ) {
	assert( inputHandlerIdx != LUA_NOREF );

	lua_rawgeti( lua, LUA_REGISTRYINDEX, inputHandlerIdx );
	lua_pushlstring( lua, buffer, len );

	pcall( 1, "script_handleInput" );
}

void script_doMacro( const char * key, int len, bool shift, bool ctrl, bool alt ) {
	assert( macroHandlerIdx != LUA_NOREF );

	lua_rawgeti( lua, LUA_REGISTRYINDEX, macroHandlerIdx );

	lua_pushlstring( lua, key, len );

	lua_pushboolean( lua, shift );
	lua_pushboolean( lua, ctrl );
	lua_pushboolean( lua, alt );

	pcall( 4, "script_doMacro" );
}

void script_handleClose() {
	assert( closeHandlerIdx != LUA_NOREF );

	lua_rawgeti( lua, LUA_REGISTRYINDEX, closeHandlerIdx );
	pcall( 0, "script_handleClose" );
}

void script_socketData( void * sock, const char * data, size_t len ) {
	assert( socketHandlerIdx != LUA_NOREF );

	lua_rawgeti( lua, LUA_REGISTRYINDEX, socketHandlerIdx );

	lua_pushlightuserdata( lua, sock );
	if( data == NULL )
		lua_pushnil( lua );
	else
		lua_pushlstring( lua, data, len );

	pcall( 2, "script_socketData" );
}

void script_fire_intervals() {
	assert( intervalHandlerIdx != LUA_NOREF );

	lua_rawgeti( lua, LUA_REGISTRYINDEX, intervalHandlerIdx );
	pcall( 0, "script_fire_intervals" );
}

namespace {

extern "C" int mud_handleXEvents( lua_State * ) {
	ui_handleXEvents();
	return 0;
}

template< typename F >
static void generic_print( F * f, lua_State * L ) {
	const char * str = luaL_checkstring( L, 1 );
	size_t len = luaL_len( L, 1 );

	Colour fg = Colour( luaL_checkinteger( L, 2 ) );
	Colour bg = Colour( luaL_checkinteger( L, 3 ) );
	bool bold = lua_toboolean( L, 4 );

	f( str, len, fg, bg, bold );
}

extern "C" int mud_connect( lua_State * L ) {
	const char * host = luaL_checkstring( L, 1 );
	int port = luaL_checkinteger( L, 2 );

	const char * err;
	void * sock = platform_connect( &err, host, port );
	if( sock != NULL ) {
		lua_pushlightuserdata( lua, sock );
		return 1;
	}

	lua_pushnil( lua );
	lua_pushstring( lua, err );

	return 2;
}

extern "C" int mud_send( lua_State * L ) {
	luaL_argcheck( L, lua_isuserdata( L, 1 ) == 1, 1, "expected socket" );
	void * sock = lua_touserdata( L, 1 );

	const char * data = luaL_checkstring( L, 2 );
	size_t len = luaL_len( L, 2 );

	platform_send( sock, data, len );

	return 0;
}

extern "C" int mud_close( lua_State * L ) {
	luaL_argcheck( L, lua_isuserdata( L, 1 ) == 1, 1, "expected socket" );
	void * sock = lua_touserdata( L, 1 );

	platform_close( sock );

	return 0;
}

extern "C" int mud_printMain( lua_State * L ) {
	generic_print( ui_main_print, L );
	return 0;
}

extern "C" int mud_newlineMain( lua_State * L ) {
	ui_main_newline();
	return 0;
}

extern "C" int mud_drawMain( lua_State * L ) {
	ui_main_draw();
	return 0;
}

extern "C" int mud_printChat( lua_State * L ) {
	generic_print( ui_chat_print, L );
	return 0;
}

extern "C" int mud_newlineChat( lua_State * L ) {
	ui_chat_newline();
	return 0;
}

extern "C" int mud_drawChat( lua_State * L ) {
	ui_chat_draw();
	return 0;
}

extern "C" int mud_setStatus( lua_State * L ) {
	luaL_argcheck( L, lua_type( L, 1 ) == LUA_TTABLE, 1, "expected function" );

	size_t len = luaL_len( L, 1 );

	ui_clear_status();

	for( size_t i = 0; i < len; i++ ) {
		lua_pushnumber( L, i + 1 );
		lua_gettable( L, 1 );

		lua_pushliteral( L, "text" );
		lua_gettable( L, 2 );
		size_t seglen;
		const char * str = lua_tolstring( L, -1, &seglen );

		lua_pushliteral( L, "fg" );
		lua_gettable( L, 2 );
		const Colour fg = Colour( lua_tointeger( L, -1 ) );

		lua_pushliteral( L, "bold" );
		lua_gettable( L, 2 );
		const bool bold = lua_toboolean( L, -1 );

		for( size_t j = 0; j < seglen; j++ ) {
			ui_statusAdd( str[ j ], fg, bold );
		}

		lua_pop( L, 4 );
	}

	ui_draw_status();

	return 0;
}

extern "C" int mud_setHandlers( lua_State * L ) {
	luaL_argcheck( L, lua_type( L, 1 ) == LUA_TFUNCTION, 1, "expected function" );
	luaL_argcheck( L, lua_type( L, 2 ) == LUA_TFUNCTION, 2, "expected function" );
	luaL_argcheck( L, lua_type( L, 3 ) == LUA_TFUNCTION, 3, "expected function" );
	luaL_argcheck( L, lua_type( L, 4 ) == LUA_TFUNCTION, 4, "expected function" );
	luaL_argcheck( L, lua_type( L, 5 ) == LUA_TFUNCTION, 5, "expected function" );

	intervalHandlerIdx = luaL_ref( L, LUA_REGISTRYINDEX );
	socketHandlerIdx = luaL_ref( L, LUA_REGISTRYINDEX );
	closeHandlerIdx = luaL_ref( L, LUA_REGISTRYINDEX );
	macroHandlerIdx = luaL_ref( L, LUA_REGISTRYINDEX );
	inputHandlerIdx = luaL_ref( L, LUA_REGISTRYINDEX );

	return 0;
}

extern "C" int mud_urgent( lua_State * L ) {
	ui_urgent();
	return 0;
}

extern "C" int mud_now( lua_State * L ) {
	lua_pushnumber( L, get_time() );
	return 1;
}

} // anon namespace

#if PLATFORM_WINDOWS
extern "C" int luaopen_lpeg( lua_State * L );
extern "C" int luaopen_lfs( lua_State * L );
#endif

void script_init() {
	mud_handleXEvents( NULL ); // TODO: why is this here?

	lua = luaL_newstate();
	luaL_openlibs( lua );

#if PLATFORM_WINDOWS
	luaL_requiref( lua, "lpeg", luaopen_lpeg, 0 );
	lua_pop( lua, 1 );

	luaL_requiref( lua, "lfs", luaopen_lfs, 0 );
	lua_pop( lua, 1 );
#endif

	lua_getglobal( lua, "debug" );
	lua_getfield( lua, -1, "traceback" );
	lua_remove( lua, -2 );

	if( luaL_loadbufferx( lua, ( const char * ) lua_bytecode, sizeof( lua_bytecode ), "main", "t" ) != LUA_OK ) {
		printf( "Error reading main.lua: %s\n", lua_tostring( lua, -1 ) );
		exit( 1 );
	}

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

	lua_pushcfunction( lua, mud_connect );
	lua_pushcfunction( lua, mud_send );
	lua_pushcfunction( lua, mud_close );

	lua_pushcfunction( lua, mud_now );

	pcall( 14, "Error running main.lua" );
}

void script_term() {
	lua_close( lua );
}
