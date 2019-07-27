





#ifndef netwerk_dns_GetAddrInfo_h
#define netwerk_dns_GetAddrInfo_h

#include "nsError.h"

#define DNS_API_PORTABLE (0)          // Portable: PR_GetAddrInfoByName()
#define DNS_API_WINDOWS_DNS_QUERY (1) // Windows: DnsQuery()

#ifdef XP_WIN
#define DNS_API DNS_API_WINDOWS_DNS_QUERY
#else
#define DNS_API DNS_API_PORTABLE
#endif

namespace mozilla {
namespace net {

class AddrInfo;















nsresult
GetAddrInfo(const char* aHost, uint16_t aAf, uint16_t aFlags, AddrInfo** aAddrInfo);







nsresult
GetAddrInfoInit();








nsresult
GetAddrInfoShutdown();

} 
} 

#endif 
