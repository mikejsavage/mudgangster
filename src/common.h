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

template< typename T >
void swap( T & a, T & b ) {
        T t = a;
        a = b;
        b = t;
}

template< typename To, typename From >
inline To checked_cast( const From & from ) {
	To result = To( from );
	assert( From( result ) == from );
	return result;
}
