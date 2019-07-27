





#ifndef netwerk_dns_GetAddrInfo_h
#define netwerk_dns_GetAddrInfo_h

#include "nsError.h"
#include "nscore.h"

#if defined(XP_WIN) && !defined(RELEASE_BUILD)
#define DNSQUERY_AVAILABLE 1
#define TTL_AVAILABLE 1
#else
#define DNSQUERY_AVAILABLE 0
#define TTL_AVAILABLE 0
#endif

namespace mozilla {
namespace net {

class AddrInfo;
















nsresult
GetAddrInfo(const char* aHost, uint16_t aAddressFamily, uint16_t aFlags,
            AddrInfo** aAddrInfo, bool aGetTtl);







nsresult
GetAddrInfoInit();








nsresult
GetAddrInfoShutdown();

} 
} 

#endif 
