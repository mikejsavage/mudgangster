#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <errno.h>
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

static void platform_init_sock( int fd ) {
#if PLATFORM_OSX
	setsockoptone( fd, SOL_SOCKET, SO_NOSIGPIPE );
#endif
}
