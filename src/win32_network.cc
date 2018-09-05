#include <winsock2.h>
#include <ws2tcpip.h>

static int NET_SEND_FLAGS = 0;

void net_init() {
	WSADATA wsa_data;
	if( WSAStartup( MAKEWORD( 2, 2 ), &wsa_data ) == SOCKET_ERROR ) {
		FATAL( "WSAStartup" );
	}
}

void net_term() {
	if( WSACleanup() == SOCKET_ERROR ) {
		FATAL( "WSACleanup" );
	}
}

static void make_socket_nonblocking( SOCKET fd ) {
	u_long one = 1;
	int ok = ioctlsocket( fd, FIONBIO, &one );
	if( ok == SOCKET_ERROR ) {
		FATAL( "ioctlsocket" );
	}
}

static void platform_init_sock( SOCKET fd ) { }

bool net_tryrecv( UDPSocket sock, void * buf, size_t len, NetAddress * addr, size_t * bytes_received ) {
	struct sockaddr_storage sa;

	socklen_t sa_size = sizeof( struct sockaddr_in );
	ssize_t received4 = recvfrom( sock.ipv4, ( char * ) buf, len, 0, ( struct sockaddr * ) &sa, &sa_size );
	if( received4 != SOCKET_ERROR ) {
		*addr = sockaddr_to_netaddress( sa );
		*bytes_received = size_t( received4 );
		return true;
	}
	else {
		int error = WSAGetLastError();
		if( error != WSAEWOULDBLOCK && error != WSAECONNRESET ) {
			FATAL( "recvfrom" );
		}
	}

	sa_size = sizeof( struct sockaddr_in6 );
	ssize_t received6 = recvfrom( sock.ipv6, ( char * ) buf, len, 0, ( struct sockaddr * ) &sa, &sa_size );
	if( received6 != SOCKET_ERROR ) {
		*addr = sockaddr_to_netaddress( sa );
		*bytes_received = size_t( received6 );
		return true;
	}
	else {
		int error = WSAGetLastError();
		if( error != WSAEWOULDBLOCK && error != WSAECONNRESET ) {
			FATAL( "recvfrom" );
		}
	}

	return false;
}
