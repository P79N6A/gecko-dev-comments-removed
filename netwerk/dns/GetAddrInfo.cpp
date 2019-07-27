





#include "GetAddrInfo.h"
#include "mozilla/net/DNS.h"
#include "prnetdb.h"
#include "nsHostResolver.h"
#include "nsError.h"
#include "mozilla/Mutex.h"
#include "nsAutoPtr.h"
#include "mozilla/StaticPtr.h"
#include "MainThreadUtils.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/net/DNS.h"
#include <algorithm>
#include "prerror.h"

#if defined(ANDROID) && ANDROID_VERSION > 19
#include <resolv_netid.h>
#endif

#include "prlog.h"
#if defined(PR_LOGGING)
static PRLogModuleInfo *gGetAddrInfoLog = PR_NewLogModule("GetAddrInfo");
#define LOG(msg, ...) \
  PR_LOG(gGetAddrInfoLog, PR_LOG_DEBUG, ("[DNS]: " msg, ##__VA_ARGS__))
#define LOG_WARNING(msg, ...) \
  PR_LOG(gGetAddrInfoLog, PR_LOG_WARNING, ("[DNS]: " msg, ##__VA_ARGS__))
#else
#define LOG(args)
#define LOG_WARNING(args)
#endif

#if DNSQUERY_AVAILABLE




#undef UNICODE
#include <ws2tcpip.h>
#undef GetAddrInfo
#include <windns.h>
#endif

namespace mozilla {
namespace net {

#if DNSQUERY_AVAILABLE






static_assert(PR_AF_INET == AF_INET && PR_AF_INET6 == AF_INET6
    && PR_AF_UNSPEC == AF_UNSPEC, "PR_AF_* must match AF_*");





static OffTheBooksMutex* gDnsapiInfoLock = nullptr;

struct DnsapiInfo
{
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(DnsapiInfo);

  HMODULE mLibrary;
  decltype(&DnsQuery_A) mDnsQueryFunc;
  decltype(&DnsFree) mDnsFreeFunc;

private:
  
  
  
  
  ~DnsapiInfo()
  {
    if (gDnsapiInfoLock) {
      gDnsapiInfoLock->AssertCurrentThreadOwns();
    } else {
      MOZ_ASSERT_UNREACHABLE("No mutex available during GetAddrInfo "
                             "shutdown.");
      return;
    }

    LOG("Freeing Dnsapi.dll");
    MOZ_ASSERT(mLibrary);
    DebugOnly<BOOL> rv = FreeLibrary(mLibrary);
    NS_WARN_IF_FALSE(rv, "Failed to free Dnsapi.dll.");
  }
};

static StaticRefPtr<DnsapiInfo> gDnsapiInfo;

static MOZ_ALWAYS_INLINE nsresult
_GetAddrInfoInit_Windows()
{
  
  
  
  MOZ_ASSERT(NS_IsMainThread(),
             "Do not initialize GetAddrInfo off main thread!");

  if (!gDnsapiInfoLock) {
    gDnsapiInfoLock = new OffTheBooksMutex("GetAddrInfo.cpp::gDnsapiInfoLock");
  }
  OffTheBooksMutexAutoLock lock(*gDnsapiInfoLock);

  if (gDnsapiInfo) {
    MOZ_ASSERT_UNREACHABLE("GetAddrInfo is being initialized multiple times!");
    return NS_ERROR_ALREADY_INITIALIZED;
  }

  HMODULE library = LoadLibraryA("Dnsapi.dll");
  if (NS_WARN_IF(!library)) {
    return NS_ERROR_FAILURE;
  }

  FARPROC dnsQueryFunc = GetProcAddress(library, "DnsQuery_A");
  FARPROC dnsFreeFunc = GetProcAddress(library, "DnsFree");
  if (NS_WARN_IF(!dnsQueryFunc) || NS_WARN_IF(!dnsFreeFunc)) {
    DebugOnly<BOOL> rv = FreeLibrary(library);
    NS_WARN_IF_FALSE(rv, "Failed to free Dnsapi.dll.");
    return NS_ERROR_FAILURE;
  }

  DnsapiInfo* info = new DnsapiInfo;
  info->mLibrary = library;
  info->mDnsQueryFunc = (decltype(info->mDnsQueryFunc)) dnsQueryFunc;
  info->mDnsFreeFunc = (decltype(info->mDnsFreeFunc)) dnsFreeFunc;
  gDnsapiInfo = info;

  return NS_OK;
}

static MOZ_ALWAYS_INLINE nsresult
_GetAddrInfoShutdown_Windows()
{
  OffTheBooksMutexAutoLock lock(*gDnsapiInfoLock);

  if (NS_WARN_IF(!gDnsapiInfo) || NS_WARN_IF(!gDnsapiInfoLock)) {
    MOZ_ASSERT_UNREACHABLE("GetAddrInfo not initialized!");
    return NS_ERROR_NOT_INITIALIZED;
  }

  gDnsapiInfo = nullptr;

  return NS_OK;
}

static MOZ_ALWAYS_INLINE nsresult
_GetTTLData_Windows(const char* aHost, uint16_t* aResult)
{
  MOZ_ASSERT(aHost);
  MOZ_ASSERT(aResult);

  nsRefPtr<DnsapiInfo> dnsapi = nullptr;
  {
    OffTheBooksMutexAutoLock lock(*gDnsapiInfoLock);
    dnsapi = gDnsapiInfo;
  }

  if (!dnsapi) {
    LOG_WARNING("GetAddrInfo has been shutdown or has not been initialized.");
    return NS_ERROR_NOT_INITIALIZED;
  }

  PDNS_RECORDA dnsData = nullptr;
  DNS_STATUS status = dnsapi->mDnsQueryFunc(
      aHost,
      DNS_TYPE_ANY,
      (DNS_QUERY_STANDARD | DNS_QUERY_NO_NETBT | DNS_QUERY_NO_HOSTS_FILE
        | DNS_QUERY_NO_MULTICAST | DNS_QUERY_ACCEPT_TRUNCATED_RESPONSE
        | DNS_QUERY_DONT_RESET_TTL_VALUES),
      nullptr,
      &dnsData,
      nullptr);
  if (status == DNS_INFO_NO_RECORDS || status == DNS_ERROR_RCODE_NAME_ERROR
      || !dnsData) {
    LOG("No DNS records found for %s.\n", aHost);
    return NS_ERROR_UNKNOWN_HOST;
  } else if (status != NOERROR) {
    LOG_WARNING("DnsQuery_A failed with status %X.\n", status);
    return NS_ERROR_FAILURE;
  }

  unsigned int ttl = -1;
  PDNS_RECORDA curRecord = dnsData;
  for (; curRecord; curRecord = curRecord->pNext) {
    
    if (curRecord->Flags.S.Section != DnsSectionAnswer) {
      continue;
    }

    if (curRecord->wType == DNS_TYPE_A || curRecord->wType == DNS_TYPE_AAAA) {
      ttl = std::min<unsigned int>(ttl, curRecord->dwTtl);
    }
  }

  dnsapi->mDnsFreeFunc(dnsData, DNS_FREE_TYPE::DnsFreeRecordList);

  {
    
    OffTheBooksMutexAutoLock lock(*gDnsapiInfoLock);
    dnsapi = nullptr;
  }

  if (ttl == -1) {
    LOG("No useable TTL found.");
    return NS_ERROR_FAILURE;
  }

  *aResult = ttl;
  return NS_OK;
}
#endif

#if defined(ANDROID) && ANDROID_VERSION >= 19

static MOZ_ALWAYS_INLINE PRAddrInfo*
_Android_GetAddrInfoForNetInterface(const char* hostname,
                                   uint16_t af,
                                   uint16_t flags,
                                   const char* aNetworkInterface)
{
  if ((af != PR_AF_INET && af != PR_AF_UNSPEC) ||
      (flags & ~ PR_AI_NOCANONNAME) != PR_AI_ADDRCONFIG) {
    PR_SetError(PR_INVALID_ARGUMENT_ERROR, 0);
    return nullptr;
  }

  struct addrinfo *res, hints;
  int rv;
  memset(&hints, 0, sizeof(hints));
  if (!(flags & PR_AI_NOCANONNAME)) {
    hints.ai_flags |= AI_CANONNAME;
  }

#ifdef AI_ADDRCONFIG
  if ((flags & PR_AI_ADDRCONFIG) &&
      strcmp(hostname, "localhost") != 0 &&
      strcmp(hostname, "localhost.localdomain") != 0 &&
      strcmp(hostname, "localhost6") != 0 &&
      strcmp(hostname, "localhost6.localdomain6") != 0) {
    hints.ai_flags |= AI_ADDRCONFIG;
  }
#endif

  hints.ai_family = (af == PR_AF_INET) ? AF_INET : AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

#if ANDROID_VERSION == 19
  rv = android_getaddrinfoforiface(hostname, NULL, &hints, aNetworkInterface,
                                   0, &res);
#else
  uint32_t netId = atoi(aNetworkInterface);
  rv = android_getaddrinfofornet(hostname, NULL, &hints, netId, 0, &res);
#endif

#ifdef AI_ADDRCONFIG
  if (rv == EAI_BADFLAGS && (hints.ai_flags & AI_ADDRCONFIG)) {
    hints.ai_flags &= ~AI_ADDRCONFIG;
#if ANDROID_VERSION == 19
    rv = android_getaddrinfoforiface(hostname, NULL, &hints, aNetworkInterface,
                                     0, &res);
#else
    uint32_t netId = atoi(aNetworkInterface);
    rv = android_getaddrinfofornet(hostname, NULL, &hints, netId, 0, &res);
#endif
  }
#endif

  if (rv == 0) {
    return (PRAddrInfo *) res;
  }

  PR_SetError(PR_DIRECTORY_LOOKUP_ERROR, rv);
  return nullptr;
}
#endif





static MOZ_ALWAYS_INLINE nsresult
_GetAddrInfo_Portable(const char* aCanonHost, uint16_t aAddressFamily,
                      uint16_t aFlags, const char* aNetworkInterface,
                      AddrInfo** aAddrInfo)
{
  MOZ_ASSERT(aCanonHost);
  MOZ_ASSERT(aAddrInfo);

  
  
  
  int prFlags = PR_AI_ADDRCONFIG;
  if (!(aFlags & nsHostResolver::RES_CANON_NAME)) {
    prFlags |= PR_AI_NOCANONNAME;
  }

  
  
  bool disableIPv4 = aAddressFamily == PR_AF_INET6;
  if (disableIPv4) {
    aAddressFamily = PR_AF_UNSPEC;
  }

  PRAddrInfo* prai;
#if defined(ANDROID) && ANDROID_VERSION >= 19
  if (aNetworkInterface && aNetworkInterface[0] != '\0') {
    prai = _Android_GetAddrInfoForNetInterface(aCanonHost,
                                               aAddressFamily,
                                               prFlags,
                                               aNetworkInterface);
  } else
#endif
  {
    prai = PR_GetAddrInfoByName(aCanonHost, aAddressFamily, prFlags);
  }

  if (!prai) {
    return NS_ERROR_UNKNOWN_HOST;
  }

  const char* canonName = nullptr;
  if (aFlags & nsHostResolver::RES_CANON_NAME) {
    canonName = PR_GetCanonNameFromAddrInfo(prai);
  }

  nsAutoPtr<AddrInfo> ai(new AddrInfo(aCanonHost, prai, disableIPv4, canonName));
  PR_FreeAddrInfo(prai);
  if (ai->mAddresses.isEmpty()) {
    return NS_ERROR_UNKNOWN_HOST;
  }

  *aAddrInfo = ai.forget();

  return NS_OK;
}




nsresult
GetAddrInfoInit() {
  LOG("Initializing GetAddrInfo.\n");

#if DNSQUERY_AVAILABLE
  return _GetAddrInfoInit_Windows();
#else
  return NS_OK;
#endif
}

nsresult
GetAddrInfoShutdown() {
  LOG("Shutting down GetAddrInfo.\n");

#if DNSQUERY_AVAILABLE
  return _GetAddrInfoShutdown_Windows();
#else
  return NS_OK;
#endif
}

nsresult
GetAddrInfo(const char* aHost, uint16_t aAddressFamily, uint16_t aFlags,
            const char* aNetworkInterface, AddrInfo** aAddrInfo, bool aGetTtl)
{
  if (NS_WARN_IF(!aHost) || NS_WARN_IF(!aAddrInfo)) {
    return NS_ERROR_NULL_POINTER;
  }

#if DNSQUERY_AVAILABLE
  
  if (aGetTtl) {
    aFlags |= nsHostResolver::RES_CANON_NAME;
  }
#endif

  *aAddrInfo = nullptr;
  nsresult rv = _GetAddrInfo_Portable(aHost, aAddressFamily, aFlags,
                                      aNetworkInterface, aAddrInfo);

#if DNSQUERY_AVAILABLE
  if (aGetTtl && NS_SUCCEEDED(rv)) {
    
    
    const char *name = nullptr;
    if (*aAddrInfo != nullptr && (*aAddrInfo)->mCanonicalName) {
      name = (*aAddrInfo)->mCanonicalName;
    } else {
      name = aHost;
    }

    LOG("Getting TTL for %s (cname = %s).", aHost, name);
    uint16_t ttl = 0;
    nsresult ttlRv = _GetTTLData_Windows(name, &ttl);
    if (NS_SUCCEEDED(ttlRv)) {
      (*aAddrInfo)->ttl = ttl;
      LOG("Got TTL %u for %s (name = %s).", ttl, aHost, name);
    } else {
      LOG_WARNING("Could not get TTL for %s (cname = %s).", aHost, name);
    }
  }
#endif

  return rv;
}

} 
} 
