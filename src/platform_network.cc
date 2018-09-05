#include "common.h"

#include "platform.h"
#include "platform_network.h"

struct sockaddr_storage;

static NetAddress sockaddr_to_netaddress( const struct sockaddr_storage & ss );
static struct sockaddr_storage netaddress_to_sockaddr( const NetAddress & addr );
static void setsockoptone( OSSocket fd, int level, int opt );

#if PLATFORM_WINDOWS
#include "win32_network.cc"
#elif PLATFORM_UNIX
#include "unix_network.cc"
#else
#error new platform
#endif

bool operator==( const NetAddress & lhs, const NetAddress & rhs ) {
	if( lhs.type != rhs.type ) return false;
	if( lhs.port != rhs.port ) return false;
	if( lhs.type == NET_IPV4 ) return memcmp( &lhs.ipv4, &rhs.ipv4, sizeof( lhs.ipv4 ) ) == 0;
	return memcmp( &lhs.ipv6, &rhs.ipv6, sizeof( lhs.ipv6 ) ) == 0;
}

bool operator!=( const NetAddress & lhs, const NetAddress & rhs ) {
	return !( lhs == rhs );
}

static socklen_t sockaddr_size( const struct sockaddr_storage & ss ) {
	return ss.ss_family == AF_INET ? sizeof( struct sockaddr_in ) : sizeof( struct sockaddr_in6 );
}

static NetAddress sockaddr_to_netaddress( const struct sockaddr_storage & ss ) {
	NetAddress addr;
	if( ss.ss_family == AF_INET ) {
		const struct sockaddr_in & sa4 = ( const struct sockaddr_in & ) ss;

		addr.type = NET_IPV4;
		addr.port = ntohs( sa4.sin_port );
		memcpy( &addr.ipv4, &sa4.sin_addr.s_addr, sizeof( addr.ipv4 ) );
	}
	else {
		const struct sockaddr_in6 & sa6 = ( const struct sockaddr_in6 & ) ss;

		addr.type = NET_IPV6;
		addr.port = ntohs( sa6.sin6_port );
		memcpy( &addr.ipv6, &sa6.sin6_addr.s6_addr, sizeof( addr.ipv6 ) );
	}
	return addr;
}

static struct sockaddr_storage netaddress_to_sockaddr( const NetAddress & addr ) {
	struct sockaddr_storage ss;
	if( addr.type == NET_IPV4 ) {
		struct sockaddr_in & sa4 = ( struct sockaddr_in & ) ss;

		ss.ss_family = AF_INET;
		sa4.sin_port = htons( addr.port );
		memcpy( &sa4.sin_addr.s_addr, &addr.ipv4, sizeof( addr.ipv4 ) );
	}
	else {
		struct sockaddr_in6 & sa6 = ( struct sockaddr_in6 & ) ss;

		ss.ss_family = AF_INET6;
		sa6.sin6_port = htons( addr.port );
		memcpy( &sa6.sin6_addr.s6_addr, &addr.ipv6, sizeof( addr.ipv6 ) );
	}
	return ss;
}

static void setsockoptone( OSSocket fd, int level, int opt ) {
	int one = 1;
	int ok = setsockopt( fd, level, opt, ( char * ) &one, sizeof( one ) );
	if( ok == -1 ) {
		FATAL( "setsockopt" );
	}
}

UDPSocket net_new_udp( NonblockingBool nonblocking, u16 port ) {
	UDPSocket sock;

	sock.ipv4 = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
	if( sock.ipv4 == INVALID_SOCKET ) {
		FATAL( "socket" );
	}
	sock.ipv6 = socket( AF_INET6, SOCK_DGRAM, IPPROTO_UDP );
	if( sock.ipv6 == INVALID_SOCKET ) {
		FATAL( "socket" );
	}

	if( nonblocking == NET_NONBLOCKING ) {
		make_socket_nonblocking( sock.ipv4 );
		make_socket_nonblocking( sock.ipv6 );
	}

	if( port != 0 ) {
		setsockoptone( sock.ipv4, SOL_SOCKET, SO_REUSEADDR );
		setsockoptone( sock.ipv6, SOL_SOCKET, SO_REUSEADDR );
		setsockoptone( sock.ipv6, IPPROTO_IPV6, IPV6_V6ONLY );
	}

	platform_init_sock( sock.ipv4 );
	platform_init_sock( sock.ipv6 );

	{
		sockaddr_in my_addr4;
		my_addr4.sin_family = AF_INET;
		my_addr4.sin_port = htons( port );
		my_addr4.sin_addr.s_addr = htonl( INADDR_ANY );
		int ok = bind( sock.ipv4, ( struct sockaddr * ) &my_addr4, sizeof( my_addr4 ) );
		if( ok == SOCKET_ERROR ) {
			FATAL( "bind" );
		}
	}

	{
		sockaddr_in6 my_addr6;
		my_addr6.sin6_family = AF_INET6;
		my_addr6.sin6_port = htons( port );
		my_addr6.sin6_addr = in6addr_any;
		int ok = bind( sock.ipv6, ( struct sockaddr * ) &my_addr6, sizeof( my_addr6 ) );
		if( ok == SOCKET_ERROR ) {
			FATAL( "bind" );
		}
	}

	return sock;
}

void net_send( UDPSocket sock, const void * data, size_t len, const NetAddress & addr ) {
	struct sockaddr_storage ss = netaddress_to_sockaddr( addr );
	socklen_t ss_size = sockaddr_size( ss );
	OSSocket fd = addr.type == NET_IPV4 ? sock.ipv4 : sock.ipv6;
	ssize_t ok = sendto( fd, ( const char * ) data, checked_cast< int >( len ), NET_SEND_FLAGS, ( struct sockaddr * ) &ss, ss_size );
	if( ok == SOCKET_ERROR ) {
		FATAL( "sendto" );
	}
}

void net_destroy( UDPSocket * sock ) {
	int ok4 = closesocket( sock->ipv4 );
	if( ok4 == -1 ) {
		FATAL( "closesocket" );
	}
	int ok6 = closesocket( sock->ipv6 );
	if( ok6 == -1 ) {
		FATAL( "closesocket" );
	}
	sock->ipv4 = INVALID_SOCKET;
	sock->ipv6 = INVALID_SOCKET;
}

bool net_new_tcp( TCPSocket * sock, const NetAddress & addr, NonblockingBool nonblocking ) {
	struct sockaddr_storage ss = netaddress_to_sockaddr( addr );
	socklen_t ss_size = sockaddr_size( ss );

	sock->fd = socket( ss.ss_family, SOCK_STREAM, IPPROTO_TCP );
	if( sock->fd == INVALID_SOCKET ) {
		FATAL( "socket" );
	}

	int ok = connect( sock->fd, ( const sockaddr * ) &ss, ss_size );
	if( ok == -1 ) {
		int ok_close = closesocket( sock->fd );
		if( ok_close == -1 ) {
			FATAL( "closesocket" );
		}
		// TODO: check for actual coding errors too
		return false;
	}

	if( nonblocking == NET_NONBLOCKING ) {
		make_socket_nonblocking( sock->fd );
	}

	setsockoptone( sock->fd, SOL_SOCKET, SO_KEEPALIVE );

	platform_init_sock( sock->fd );

	return true;
}

bool net_send( TCPSocket sock, const void * data, size_t len ) {
	ssize_t sent = send( sock.fd, ( const char * ) data, len, NET_SEND_FLAGS );
	if( sent < 0 ) return false;
	return checked_cast< size_t >( sent ) == len;
}

TCPRecvResult net_recv( TCPSocket sock, void * buf, size_t buf_size, size_t * bytes_read, u32 timeout_ms ) {
	while( true ) {
		if( timeout_ms > 0 ) {
			fd_set fds;
			FD_ZERO( &fds );
			FD_SET( sock.fd, &fds );

			struct timeval tv;
			tv.tv_sec = timeout_ms / 1000;
			tv.tv_usec = ( timeout_ms % 1000 ) * 1000;

			int ok = select( sock.fd + 1, &fds, NULL, NULL, &tv );
			// TODO: update timeout
			if( ok == 0 ) {
				return TCP_TIMEOUT;
			}
			if( ok == -1 ) {
				if( errno != EINTR ) continue;
				FATAL( "select" );
			}
		}

		ssize_t r = recv( sock.fd, ( char * ) buf, buf_size, 0 );
		// TODO: this is not right on windows
		if( r == -1 ) {
			if( errno == EINTR ) continue;
			if( errno == ECONNRESET ) return TCP_ERROR;
			FATAL( "recv" );
		}

		*bytes_read = checked_cast< size_t >( r );
		return r == 0 ? TCP_CLOSED : TCP_OK;
	}
}

void net_destroy( TCPSocket * sock ) {
	int ok = closesocket( sock->fd );
	if( ok == -1 ) {
		FATAL( "closesocket" );
	}
	sock->fd = INVALID_SOCKET;
}

size_t dns( const char * host, NetAddress * out, size_t n ) {
	struct addrinfo hints;
	memset( &hints, 0, sizeof( struct addrinfo ) );
	hints.ai_family = AF_UNSPEC;
	// TODO: figure out why ivp6 doesn't work on windows
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	struct addrinfo * addresses;
	int ok = getaddrinfo( host, "http", &hints, &addresses );
	if( ok != 0 )
		return 0;

	struct addrinfo * cursor = addresses;
	size_t i = 0;
	while( cursor != NULL && i < n ) {
		out[ i ] = sockaddr_to_netaddress( *( struct sockaddr_storage * ) cursor->ai_addr );
		cursor = cursor->ai_next;
		i++;
	}
	freeaddrinfo( addresses );

	return i;
}

bool dns_first( const char * host, NetAddress * address ) {
	return dns( host, address, 1 ) == 1;
}
