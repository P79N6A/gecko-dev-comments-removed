

































static char *RCSSTRING __UNUSED__="$Id: transport_addr.c,v 1.2 2008/04/28 17:59:03 ekr Exp $";


#include <csi_platform.h>
#include <stdio.h>
#include <memory.h>
#include <sys/types.h>
#include <errno.h>
#ifdef WIN32
#include <winsock2.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif
#include <assert.h>
#include "nr_api.h"
#include "transport_addr.h"

int nr_transport_addr_fmt_addr_string(nr_transport_addr *addr)
  {
    int _status;
    
    char buffer[40];

    switch(addr->ip_version){
      case NR_IPV4:
        if (!inet_ntop(AF_INET, &addr->u.addr4.sin_addr,buffer,sizeof(buffer)))
          strcpy(buffer, "[error]");
        snprintf(addr->as_string,sizeof(addr->as_string),"IP4:%s:%d",buffer,ntohs(addr->u.addr4.sin_port));
        break;
      case NR_IPV6:
        if (!inet_ntop(AF_INET6, &addr->u.addr6.sin6_addr,buffer,sizeof(buffer)))
          strcpy(buffer, "[error]");
        snprintf(addr->as_string,sizeof(addr->as_string),"IP6:[%s]:%d",buffer,ntohs(addr->u.addr6.sin6_port));
        break;
      default:
        ABORT(R_INTERNAL);
    }

    _status=0;
  abort:
    return(_status);
  }

int nr_sockaddr_to_transport_addr(struct sockaddr *saddr, int saddr_len, int protocol, int keep, nr_transport_addr *addr)
  {
    int r,_status;

    if(!keep) memset(addr,0,sizeof(nr_transport_addr));

    if(saddr->sa_family==PF_INET){
      if(saddr_len != sizeof(struct sockaddr_in))
        ABORT(R_BAD_ARGS);

      switch(protocol){
        case IPPROTO_TCP:
        case IPPROTO_UDP:
          break;
        default:
          ABORT(R_BAD_ARGS);
      }
      addr->ip_version=NR_IPV4;
      addr->protocol=protocol;

      memcpy(&addr->u.addr4,saddr,sizeof(struct sockaddr_in));
      addr->addr=(struct sockaddr *)&addr->u.addr4;
      addr->addr_len=saddr_len;
    }
    else if(saddr->sa_family==PF_INET6){
      
      ABORT(R_INTERNAL);
    }
    else
      ABORT(R_BAD_ARGS);

    if(r=nr_transport_addr_fmt_addr_string(addr))
      ABORT(r);

    _status=0;
  abort:
    return(_status);
  }


int nr_transport_addr_copy(nr_transport_addr *to, nr_transport_addr *from)
  {
    memcpy(to,from,sizeof(nr_transport_addr));
    to->addr=(struct sockaddr *)((char *)to + ((char *)from->addr - (char *)from));

    return(0);
  }


int nr_ip4_port_to_transport_addr(UINT4 ip4, UINT2 port, int protocol, nr_transport_addr *addr)
  {
    int r,_status;

    memset(addr, 0, sizeof(nr_transport_addr));

    addr->ip_version=NR_IPV4;
    addr->protocol=protocol;
#ifdef HAVE_SIN_LEN
    addr->u.addr4.sin_len=sizeof(struct sockaddr_in);
#endif
    addr->u.addr4.sin_family=PF_INET;
    addr->u.addr4.sin_port=htons(port);
    addr->u.addr4.sin_addr.s_addr=htonl(ip4);
    addr->addr=(struct sockaddr *)&addr->u.addr4;
    addr->addr_len=sizeof(struct sockaddr_in);

    if(r=nr_transport_addr_fmt_addr_string(addr))
      ABORT(r);

    _status=0;
  abort:
    return(_status);
  }

int nr_ip4_str_port_to_transport_addr(char *ip4, UINT2 port, int protocol, nr_transport_addr *addr)
  {
    int r,_status;
    in_addr_t ip_addr;

    ip_addr=inet_addr(ip4);
    
    if(r=nr_ip4_port_to_transport_addr(ntohl(ip_addr),port,protocol,addr))
      ABORT(r);

    _status=0;
  abort:
    return(_status);
  }

int nr_transport_addr_get_addrstring(nr_transport_addr *addr, char *str, int maxlen)
  {
    int _status;
    const char *res;

    switch(addr->ip_version){
      case NR_IPV4:
        res = inet_ntop(AF_INET, &addr->u.addr4.sin_addr,str,maxlen);
        break;
      case NR_IPV6:
        res = inet_ntop(AF_INET6, &addr->u.addr6.sin6_addr,str,maxlen);
        break;
      default:
        ABORT(R_INTERNAL);
    }

    if(!res){
      if (errno == ENOSPC){
        ABORT(R_BAD_ARGS);
      }
      ABORT(R_INTERNAL);
    }

    _status=0;
  abort:
    return(_status);
  }

int nr_transport_addr_get_port(nr_transport_addr *addr, int *port)
  {
    int _status;

    switch(addr->ip_version){
      case NR_IPV4:
        *port=ntohs(addr->u.addr4.sin_port);
        break;
      case NR_IPV6:
        *port=ntohs(addr->u.addr6.sin6_port);
        break;
      default:
        ABORT(R_INTERNAL);
    }

    _status=0;
  abort:
    return(_status);
  }

int nr_transport_addr_set_port(nr_transport_addr *addr, int port)
  {
    int _status;

    switch(addr->ip_version){
      case NR_IPV4:
        addr->u.addr4.sin_port=htons(port);
        break;
      case NR_IPV6:
        addr->u.addr6.sin6_port=htons(port);
        break;
      default:
        ABORT(R_INTERNAL);
    }

    _status=0;
  abort:
    return(_status);
  }

int nr_transport_addr_get_ip4(nr_transport_addr *addr, UINT4 *ip4p)
  {
    int _status;

    switch(addr->ip_version){
      case NR_IPV4:
        *ip4p=ntohl(addr->u.addr4.sin_addr.s_addr);
        break;
      case NR_IPV6:
        ABORT(R_NOT_FOUND);
        break;
      default:
        ABORT(R_INTERNAL);
    }

    _status=0;
  abort:
    return(_status);
  }



int nr_transport_addr_cmp(nr_transport_addr *addr1,nr_transport_addr *addr2,int mode)
  {
    assert(mode);

    if(addr1->ip_version != addr2->ip_version)
      return(1);

    if(mode < NR_TRANSPORT_ADDR_CMP_MODE_PROTOCOL)
      return(0);

    if(addr1->protocol != addr2->protocol)
      return(1);

    if(mode < NR_TRANSPORT_ADDR_CMP_MODE_ADDR)
      return(0);

    assert(addr1->addr_len == addr2->addr_len);
    switch(addr1->ip_version){
      case NR_IPV4:
        if(addr1->u.addr4.sin_addr.s_addr != addr2->u.addr4.sin_addr.s_addr)
          return(1);
        if(mode < NR_TRANSPORT_ADDR_CMP_MODE_ALL)
          return(0);
        if(addr1->u.addr4.sin_port != addr2->u.addr4.sin_port)
          return(1);
        break;
      case NR_IPV6:
        UNIMPLEMENTED;
      default:
        abort();
    }

    return(0);
  }

int nr_transport_addr_is_loopback(nr_transport_addr *addr)
  {
    switch(addr->ip_version){
      case NR_IPV4:
        switch(addr->u.addr4.sin_family){
          case AF_INET:
            if (((ntohl(addr->u.addr4.sin_addr.s_addr)>>24)&0xff)==0x7f)
              return 1;
            break;
          default:
            UNIMPLEMENTED;
            break;
        }
        break;

      default:
        UNIMPLEMENTED;
    }

    return(0);
  }

int nr_transport_addr_is_wildcard(nr_transport_addr *addr)
  {
    switch(addr->ip_version){
      case NR_IPV4:
        if(addr->u.addr4.sin_addr.s_addr==INADDR_ANY)
          return(1);
        if(addr->u.addr4.sin_port==0)
          return(1);
        break;
      default:
        UNIMPLEMENTED;
    }

    return(0);
  }
