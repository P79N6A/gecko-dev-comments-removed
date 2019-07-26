



#ifndef DNS_h_
#define DNS_h_

#include "nscore.h"
#include "prio.h"
#include "prnetdb.h"
#include "mozilla/LinkedList.h"

#if !defined(XP_WIN) && !defined(XP_OS2)
#include <arpa/inet.h>
#endif

#ifdef XP_WIN
#include "winsock2.h"
#endif

#define IPv6ADDR_IS_LOOPBACK(a) \
  (((a)->u32[0] == 0)     &&    \
   ((a)->u32[1] == 0)     &&    \
   ((a)->u32[2] == 0)     &&    \
   ((a)->u8[12] == 0)     &&    \
   ((a)->u8[13] == 0)     &&    \
   ((a)->u8[14] == 0)     &&    \
   ((a)->u8[15] == 0x1U))

#define IPv6ADDR_IS_V4MAPPED(a) \
  (((a)->u32[0] == 0)     &&    \
   ((a)->u32[1] == 0)     &&    \
   ((a)->u8[8] == 0)      &&    \
   ((a)->u8[9] == 0)      &&    \
   ((a)->u8[10] == 0xff)  &&    \
   ((a)->u8[11] == 0xff))

#define IPv6ADDR_V4MAPPED_TO_IPADDR(a) ((a)->u32[3])

#define IPv6ADDR_IS_UNSPECIFIED(a) \
  (((a)->u32[0] == 0)  &&          \
   ((a)->u32[1] == 0)  &&          \
   ((a)->u32[2] == 0)  &&          \
   ((a)->u32[3] == 0))

namespace mozilla {
namespace net {



const int kIPv4CStrBufSize = 16;
const int kIPv6CStrBufSize = 46;








union IPv6Addr {
  uint8_t  u8[16];
  uint16_t u16[8];
  uint32_t u32[4];
  uint64_t u64[2];
};






union NetAddr {
  struct {
    uint16_t family;                
    char data[14];                  
  } raw;
  struct {
    uint16_t family;                
    uint16_t port;                  
    uint32_t ip;                    
  } inet;
  struct {
    uint16_t family;                
    uint16_t port;                  
    uint32_t flowinfo;              
    IPv6Addr ip;                    
    uint32_t scope_id;              
  } inet6;
#if defined(XP_UNIX) || defined(XP_OS2)
  struct {                          
    uint16_t family;                
#ifdef XP_OS2
    char path[108];                 
#else
    char path[104];                 
#endif
  } local;
#endif
};




class NetAddrElement : public LinkedListElement<NetAddrElement> {
public:
  NetAddrElement(const PRNetAddr *prNetAddr);
  ~NetAddrElement();

  NetAddr mAddress;
};
    
class AddrInfo {
public:
  AddrInfo(const char *host, const PRAddrInfo *prAddrInfo);
  ~AddrInfo();

  char *mHostName;
  LinkedList<NetAddrElement> mAddresses;
};



void PRNetAddrToNetAddr(const PRNetAddr *prAddr, NetAddr *addr);



void NetAddrToPRNetAddr(const NetAddr *addr, PRNetAddr *prAddr);

bool NetAddrToString(const NetAddr *addr, char *buf, uint32_t bufSize);

bool IsLoopBackAddress(const NetAddr *addr);

bool IsIPAddrAny(const NetAddr *addr);

bool IsIPAddrV4Mapped(const NetAddr *addr);

} 
} 

#endif 
