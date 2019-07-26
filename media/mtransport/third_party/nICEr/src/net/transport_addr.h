

































#ifndef _transport_addr_h
#define _transport_addr_h

#include <sys/types.h>
#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#endif

#ifdef WIN32
#define MAXIFNAME IFNAMSIZ
#else
#define MAXIFNAME 16
#endif





typedef struct nr_transport_addr_ {
  UCHAR ip_version;  
#define NR_IPV4 4
#define NR_IPV6 6
  UCHAR protocol;    
  struct sockaddr *addr;
  int addr_len;
  union {
    struct sockaddr_in addr4;
    struct sockaddr_in6 addr6;
  } u;
  char ifname[MAXIFNAME];
  

  char as_string[52];
} nr_transport_addr;

int nr_sockaddr_to_transport_addr(struct sockaddr *saddr, int saddr_len, int protocol, int keep, nr_transport_addr *addr);


int nr_ip4_port_to_transport_addr(UINT4 ip4, UINT2 port, int protocol, nr_transport_addr *addr);
int nr_ip4_str_port_to_transport_addr(char *ip4, UINT2 port, int protocol, nr_transport_addr *addr);

int nr_transport_addr_get_addrstring(nr_transport_addr *addr, char *str, int maxlen);
int nr_transport_addr_get_port(nr_transport_addr *addr, int *port);
int nr_transport_addr_get_ip4(nr_transport_addr *addr, UINT4 *ip4p);
int nr_transport_addr_cmp(nr_transport_addr *addr1,nr_transport_addr *addr2,int mode);
#define NR_TRANSPORT_ADDR_CMP_MODE_VERSION   1
#define NR_TRANSPORT_ADDR_CMP_MODE_PROTOCOL  2
#define NR_TRANSPORT_ADDR_CMP_MODE_ADDR      3
#define NR_TRANSPORT_ADDR_CMP_MODE_ALL       4

int nr_transport_addr_is_wildcard(nr_transport_addr *addr);
int nr_transport_addr_is_loopback(nr_transport_addr *addr);
int nr_transport_addr_copy(nr_transport_addr *to, nr_transport_addr *from);
int nr_transport_addr_fmt_addr_string(nr_transport_addr *addr);
int nr_transport_addr_set_port(nr_transport_addr *addr, int port);

#endif

