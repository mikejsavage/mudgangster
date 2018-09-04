#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>

#include "config.h"

#define STRL( x ) ( x ), sizeof( x ) - 1

#define STATIC_ASSERT( p ) static_assert( p, #p )

template< typename T >
constexpr T min( T a, T b ) {
	return a < b ? a : b;
}

template< typename T >
constexpr T max( T a, T b ) {
	return a > b ? a : b;
}

template< typename To, typename From >
inline To checked_cast( const From & from ) {
	To result = To( from );
	assert( From( result ) == from );
	return result;
}

// TODO: probably don't need most of these
enum Colour {
	BLACK,
	RED,
	GREEN,
	YELLOW,
	BLUE,
	MAGENTA,
	CYAN,
	WHITE,
	SYSTEM,

	NUM_COLOURS,

	COLOUR_BG,
	COLOUR_STATUSBG,
	COLOUR_CURSOR,
};
