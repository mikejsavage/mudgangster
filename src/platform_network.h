#pragma once

#include "platform.h"
#include "common.h"

#if PLATFORM_WINDOWS
typedef s64 OSSocket;
#elif PLATFORM_UNIX
typedef int OSSocket;
#else
#error new platform
#endif

enum TransportProtocol { NET_UDP, NET_TCP };
enum IPvX { NET_IPV4, NET_IPV6 };
enum NonblockingBool { NET_BLOCKING, NET_NONBLOCKING };

enum TCPRecvResult {
	TCP_OK,
	TCP_TIMEOUT,
	TCP_CLOSED,
	TCP_ERROR,
};

struct UDPSocket {
	OSSocket ipv4, ipv6;
};

struct TCPSocket {
	OSSocket fd;
};

struct IPv4 { u8 bytes[ 4 ]; };
struct IPv6 { u8 bytes[ 16 ]; };

struct NetAddress {
        IPvX type;
        union {
                IPv4 ipv4;
                IPv6 ipv6;
        };
        u16 port;
};

bool operator==( const NetAddress & lhs, const NetAddress & rhs );
bool operator!=( const NetAddress & lhs, const NetAddress & rhs );

void net_init();
void net_term();

UDPSocket net_new_udp( NonblockingBool nonblocking, u16 port = 0 );
void net_send( UDPSocket sock, const void * data, size_t len, const NetAddress & addr );
bool net_tryrecv( UDPSocket sock, void * buf, size_t len, NetAddress * addr, size_t * bytes_received );
void net_destroy( UDPSocket * sock );

bool net_new_tcp( TCPSocket * sock, const NetAddress & addr, NonblockingBool nonblocking );
bool net_send( TCPSocket sock, const void * data, size_t len );
TCPRecvResult net_recv( TCPSocket sock, void * buf, size_t buf_size, size_t * bytes_read, u32 timeout_ms = 0 );
void net_destroy( TCPSocket * sock );

bool dns_first( const char * host, NetAddress * address );
