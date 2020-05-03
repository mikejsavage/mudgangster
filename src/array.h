#pragma once

#include "common.h"

template< typename T >
class DynamicArray {
	size_t n;
	size_t capacity;
	T * elems;

public:
	NONCOPYABLE( DynamicArray );

	DynamicArray( size_t initial_capacity = 0 ) {
		capacity = initial_capacity;
		elems = capacity == 0 ? NULL : alloc_many< T >( capacity );
		clear();
	}

	~DynamicArray() {
		free( elems );
	}

	size_t add( const T & x ) {
		size_t idx = extend( 1 );
		elems[ idx ] = x;
		return idx;
	}

	void insert( const T & x, size_t pos ) {
		ASSERT( pos <= n );

		extend( 1 );
		memmove( elems + pos + 1, elems + pos, ( n - pos - 1 ) * sizeof( T ) );
		elems[ pos ] = x;
	}

	void from_span( Span< const T > s ) {
		resize( s.n );
		memcpy( elems, s.ptr, s.num_bytes() );
	}

	bool remove( size_t pos ) {
		if( n == 0 || pos >= n )
			return false;

		resize( n - 1 );
		memmove( elems + pos, elems + pos + 1, ( n - pos ) * sizeof( T ) );

		return true;
	}

	void clear() {
		n = 0;
	}

	void resize( size_t new_size ) {
		if( new_size < n ) {
			n = new_size;
			return;
		}

		if( new_size <= capacity ) {
			n = new_size;
			return;
		}

		size_t new_capacity = max( size_t( 64 ), capacity );
		while( new_capacity < new_size )
			new_capacity *= 2;

		elems = realloc_many< T >( elems, new_capacity );
		capacity = new_capacity;
		n = new_size;
	}

	size_t extend( size_t by ) {
		size_t old_size = n;
		resize( n + by );
		return old_size;
	}

	T & operator[]( size_t i ) {
		ASSERT( i < n );
		return elems[ i ];
	}

	const T & operator[]( size_t i ) const {
		ASSERT( i < n );
		return elems[ i ];
	}

	T & top() {
		ASSERT( n > 0 );
		return elems[ n - 1 ];
	}

	const T & top() const {
		ASSERT( n > 0 );
		return elems[ n - 1 ];
	}

	T * ptr() { return elems; }
	const T * ptr() const { return elems; }
	size_t size() const { return n; }
	size_t num_bytes() const { return sizeof( T ) * n; }

	T * begin() { return elems; }
	T * end() { return elems + n; }
	const T * begin() const { return elems; }
	const T * end() const { return elems + n; }

	Span< T > span() { return Span< T >( elems, n ); }
	Span< const T > span() const { return Span< const T >( elems, n ); }
};
