#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <unistd.h>

#define INVALID_SOCKET ( -1 )
#define SOCKET_ERROR ( -1 )

#define closesocket close

#if PLATFORM_OSX
static int NET_SEND_FLAGS = 0;
#else
static int NET_SEND_FLAGS = MSG_NOSIGNAL;
#endif

void net_init() { }
void net_term() { }

static void make_socket_nonblocking( int fd ) {
	int flags = fcntl( fd, F_GETFL, 0 );
	if( flags == -1 ) FATAL( "fcntl F_GETFL" );
	int ok = fcntl( fd, F_SETFL, flags | O_NONBLOCK );
	if( ok == -1 ) FATAL( "fcntl F_SETFL" );
}

static void platform_init_sock( int fd ) {
#if PLATFORM_OSX
	setsockoptone( fd, SOL_SOCKET, SO_NOSIGPIPE );
#endif
}

bool net_tryrecv( UDPSocket sock, void * buf, size_t len, NetAddress * addr, size_t * bytes_received ) {
	struct sockaddr_storage sa;

	socklen_t sa_size = sizeof( struct sockaddr_in );
	ssize_t received4 = recvfrom( sock.ipv4, buf, len, 0, ( struct sockaddr * ) &sa, &sa_size );
	if( received4 != -1 ) {
		*addr = sockaddr_to_netaddress( sa );
		*bytes_received = size_t( received4 );
		return true;
	}
	if( received4 == -1 && errno != EAGAIN ) {
		FATAL( "recvfrom" );
	}

	sa_size = sizeof( struct sockaddr_in6 );
	ssize_t received6 = recvfrom( sock.ipv6, buf, len, 0, ( struct sockaddr * ) &sa, &sa_size );
	if( received6 != -1 ) {
		*addr = sockaddr_to_netaddress( sa );
		*bytes_received = size_t( received6 );
		return true;
	}
	if( received6 == -1 && errno != EAGAIN ) {
		FATAL( "recvfrom" );
	}

	return false;
}
