#include <winsock2.h>
#include <ws2tcpip.h>

#define ssize_t int

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

static void platform_init_sock( SOCKET fd ) { }
