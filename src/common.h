#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "config.h"

#include "tracy/Tracy.hpp"

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

inline void * alloc_size( size_t size ) {
	void * ptr = malloc( size );
	if( ptr == NULL ) {
		FATAL( "malloc" );
		abort();
	}
	return ptr;
}

template< typename T >
T * alloc() {
	return ( T * ) alloc_size( sizeof( T ) );
}

template< typename T >
T * alloc_many( size_t n ) {
	if( SIZE_MAX / n < sizeof( T ) )
		FATAL( "allocation too large" );
	return ( T * ) alloc_size( n * sizeof( T ) );
}

template< typename T >
T * realloc_many( T * old, size_t n ) {
	if( SIZE_MAX / n < sizeof( T ) )
		FATAL( "allocation too large" );
	T * res = ( T * ) realloc( old, n * sizeof( T ) );
	if( res == NULL ) {
		FATAL( "realloc" );
		abort();
	}
	return res;
}

template< typename T >
struct Span {
	T * ptr;
	size_t n;

	constexpr Span() : ptr( NULL ), n( 0 ) { }
	constexpr Span( T * ptr_, size_t n_ ) : ptr( ptr_ ), n( n_ ) { }

	// allow implicit conversion to Span< const T >
	operator Span< const T >() { return Span< const T >( ptr, n ); }
	operator Span< const T >() const { return Span< const T >( ptr, n ); }

	size_t num_bytes() const { return sizeof( T ) * n; }

	T & operator[]( size_t i ) {
		assert( i < n );
		return ptr[ i ];
	}

	const T & operator[]( size_t i ) const {
		assert( i < n );
		return ptr[ i ];
	}

	Span< T > operator+( size_t i ) {
		assert( i <= n );
		return Span< T >( ptr + i, n - i );
	}

	Span< const T > operator+( size_t i ) const {
		assert( i <= n );
		return Span< const T >( ptr + i, n - i );
	}

	void operator++( int ) {
		assert( n > 0 );
		ptr++;
		n--;
	}

	T * begin() { return ptr; }
	T * end() { return ptr + n; }
	const T * begin() const { return ptr; }
	const T * end() const { return ptr + n; }

	Span< T > slice( size_t start, size_t one_past_end ) {
		assert( start <= one_past_end );
		assert( one_past_end <= n );
		return Span< T >( ptr + start, one_past_end - start );
	}

	Span< const T > slice( size_t start, size_t one_past_end ) const {
		assert( start <= one_past_end );
		assert( one_past_end <= n );
		return Span< const T >( ptr + start, one_past_end - start );
	}

	template< typename S >
	Span< S > cast() {
		assert( num_bytes() % sizeof( S ) == 0 );
		return Span< S >( ( S * ) ptr, num_bytes() / sizeof( S ) );
	}
};

template< typename T >
Span< T > alloc_span( size_t n ) {
	return Span< T >( alloc_many< T >( n ), n );
}
