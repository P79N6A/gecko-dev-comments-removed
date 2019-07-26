



#ifndef mozilla_net_DashboardTypes_h_
#define mozilla_net_DashboardTypes_h_

#include "mozilla/StandardInteger.h"
#include "nsStringGlue.h"
#include "nsTArray.h"

namespace mozilla {
namespace net {

struct SocketInfo
{
    nsCString host;
    uint64_t  sent;
    uint64_t  received;
    uint16_t  port;
    bool      active;
    bool      tcp;
};

struct DNSCacheEntries
{
    nsCString hostname;
    nsTArray<nsCString> hostaddr;
    uint16_t family;
    int64_t expiration;
};

struct HttpConnInfo
{
    uint32_t ttl;
    uint32_t rtt;
    nsString protocolVersion;

    void SetHTTP1ProtocolVersion(uint8_t pv);
    void SetHTTP2ProtocolVersion(uint8_t pv);
};

struct HttpRetParams
{
    nsCString host;
    nsTArray<HttpConnInfo>   active;
    nsTArray<HttpConnInfo>   idle;
    uint32_t  counter;
    uint16_t  port;
    bool      spdy;
    bool      ssl;
};

} }

#endif 
