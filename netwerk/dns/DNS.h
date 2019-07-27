





#ifndef DNS_h_
#define DNS_h_

#include "nscore.h"
#include "prio.h"
#include "prnetdb.h"
#include "plstr.h"
#include "mozilla/LinkedList.h"
#include "mozilla/MemoryReporting.h"

#if !defined(XP_WIN)
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






#ifdef XP_WIN

const int kIPv4CStrBufSize = 22;
const int kIPv6CStrBufSize = 65;
const int kNetAddrMaxCStrBufSize = kIPv6CStrBufSize;
#else
const int kIPv4CStrBufSize = 16;
const int kIPv6CStrBufSize = 46;
const int kLocalCStrBufSize = 108;
const int kNetAddrMaxCStrBufSize = kLocalCStrBufSize;
#endif








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
#if defined(XP_UNIX)
  struct {                          
    uint16_t family;                
    char path[104];                 
  } local;
#endif
  
  bool operator == (const NetAddr& other) const;
};




class NetAddrElement : public LinkedListElement<NetAddrElement> {
public:
  explicit NetAddrElement(const PRNetAddr *prNetAddr);
  NetAddrElement(const NetAddrElement& netAddr);
  ~NetAddrElement();

  NetAddr mAddress;
};

class AddrInfo {
public:
  
  
  AddrInfo(const char *host, const PRAddrInfo *prAddrInfo, bool disableIPv4,
           const char *cname);

  
  AddrInfo(const char *host, const char *cname);
  ~AddrInfo();

  void AddAddress(NetAddrElement *address);

  size_t SizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf) const;

  char *mHostName;
  char *mCanonicalName;
  uint16_t ttl;
  static const uint16_t NO_TTL_DATA = (uint16_t) -1;

  LinkedList<NetAddrElement> mAddresses;

private:
  void Init(const char *host, const char *cname);
};



void PRNetAddrToNetAddr(const PRNetAddr *prAddr, NetAddr *addr);



void NetAddrToPRNetAddr(const NetAddr *addr, PRNetAddr *prAddr);

bool NetAddrToString(const NetAddr *addr, char *buf, uint32_t bufSize);

bool IsLoopBackAddress(const NetAddr *addr);

bool IsIPAddrAny(const NetAddr *addr);

bool IsIPAddrV4Mapped(const NetAddr *addr);

bool IsIPAddrLocal(const NetAddr *addr);

nsresult GetPort(const NetAddr *aAddr, uint16_t *aResult);

} 
} 

#endif 
