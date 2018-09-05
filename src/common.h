#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "config.h"

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

#define FATAL( form, ... ) \
	do { \
		printf( "[FATAL] " form, ##__VA_ARGS__ ); \
		abort(); \
	} while( 0 )

#define STATIC_ASSERT( p ) static_assert( p, #p )
#define ASSERT assert
#define NONCOPYABLE( T ) T( const T & ) = delete; void operator=( const T & ) = delete;

template< typename T, size_t N >
char ( &ArrayCountObj( const T ( & )[ N ] ) )[ N ];
#define ARRAY_COUNT( arr ) ( sizeof( ArrayCountObj( arr ) ) )

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

template< typename T >
inline T * malloc_array( size_t count ) {
	ASSERT( SIZE_MAX / count >= sizeof( T ) );
	return ( T * ) malloc( count * sizeof( T ) );
}

template< typename T >
inline T * realloc_array( T * old, size_t count ) {
	ASSERT( SIZE_MAX / count >= sizeof( T ) );
	return ( T * ) realloc( old, count * sizeof( T ) );
}
