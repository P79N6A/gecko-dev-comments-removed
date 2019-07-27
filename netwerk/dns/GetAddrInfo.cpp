





#if defined(MOZ_LOGGING)
#define FORCE_PR_LOG
#endif

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

#if DNS_API == DNS_API_WINDOWS_DNS_QUERY




#undef UNICODE
#include "Windns.h"
#endif

namespace mozilla {
namespace net {

#if DNS_API == DNS_API_WINDOWS_DNS_QUERY






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
      MOZ_ASSERT_UNREACHABLE("No mutex available during GetAddrInfo shutdown.");
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
_GetAddrInfo_Windows(const char* aHost, uint16_t aAddressFamily,
                     uint16_t aFlags, AddrInfo** aAddrInfo)
{
  MOZ_ASSERT(aHost);
  MOZ_ASSERT(aAddrInfo);

  nsRefPtr<DnsapiInfo> dnsapi = nullptr;
  {
    OffTheBooksMutexAutoLock lock(*gDnsapiInfoLock);
    dnsapi = gDnsapiInfo;
  }

  if (!dnsapi) {
    LOG_WARNING("GetAddrInfo has been shutdown or has not been initialized.");
    return NS_ERROR_NOT_INITIALIZED;
  }

  WORD dns_type;
  if (aAddressFamily == PR_AF_INET) {
    dns_type = DNS_TYPE_A;
  } else if (aAddressFamily == PR_AF_INET6) {
    dns_type = DNS_TYPE_AAAA;
  } else {
    LOG_WARNING("Unsupported address family %X.\n", aAddressFamily);
    MOZ_ASSERT_UNREACHABLE("Unsupported address family.");
    return NS_ERROR_INVALID_ARG;
  }

  PDNS_RECORDA dnsData = nullptr;
  DNS_STATUS status = dnsapi->mDnsQueryFunc(aHost, dns_type,
      DNS_QUERY_STANDARD, nullptr, &dnsData, nullptr);
  if (status == DNS_INFO_NO_RECORDS || status == DNS_ERROR_RCODE_NAME_ERROR
      || !dnsData) {
    LOG("No DNS records found for %s.\n", aHost);
    return NS_ERROR_UNKNOWN_HOST;
  } else if (status != NOERROR) {
    LOG_WARNING("DnsQuery_A failed with status %X.\n", status);
    return NS_ERROR_FAILURE;
  }

#ifdef PR_LOGGING
  int numARecords = 0;
  int numAaaaRecords = 0;
  int numCnameRecords = 0;
  int numUnknownRecords = 0;
  #define INCREMENT_IF_LOGGING(counter) ++counter
#else
  #define INCREMENT_IF_LOGGING(counter)
#endif

  
  
  nsAutoPtr<AddrInfo> ai(new AddrInfo(aHost, nullptr));
  PDNS_RECORDA curRecord = dnsData;
  while (curRecord) {
    if (curRecord->wType == DNS_TYPE_A) {
      INCREMENT_IF_LOGGING(numARecords);

      NetAddr addr;
      addr.inet.family = AF_INET;
      addr.inet.ip = curRecord->Data.A.IpAddress;

      
      addr.inet.port = 0;

      ai->AddAddress(new NetAddrElement(addr));
    } else if (curRecord->wType == DNS_TYPE_AAAA) {
      INCREMENT_IF_LOGGING(numAaaaRecords);

      NetAddr addr;
      addr.inet6.family = AF_INET6;
      memcpy(&addr.inet6.ip, &curRecord->Data.AAAA.Ip6Address, sizeof(addr.inet6.ip.u8));

      
      addr.inet6.port = 0;

      ai->AddAddress(new NetAddrElement(addr));
    } else if (curRecord->wType == DNS_TYPE_CNAME) {
      INCREMENT_IF_LOGGING(numCnameRecords);

      char* cname = curRecord->Data.CNAME.pNameHost;
      LOG("Got 'CNAME' %s for %s.\n", cname, aHost);

      ai->SetCanonicalName(cname);
    } else {
      INCREMENT_IF_LOGGING(numUnknownRecords);

      LOG("Ignoring record for %s of type %X.\n", aHost, curRecord->wType);
    }

    curRecord = curRecord->pNext;
  }

#ifdef PR_LOGGING
  LOG("Got %d 'A' records, %d 'AAAA' records, %d 'CNAME' records, and %d "
      "unknown records for host %s.\n", numARecords, numAaaaRecords,
      numCnameRecords, numUnknownRecords, aHost);
#endif

  dnsapi->mDnsFreeFunc(dnsData, DNS_FREE_TYPE::DnsFreeRecordList);

  {
    
    OffTheBooksMutexAutoLock lock(*gDnsapiInfoLock);
    dnsapi = nullptr;
  }

  if (ai->mAddresses.isEmpty()) {
    return NS_ERROR_UNKNOWN_HOST;
  }

  *aAddrInfo = ai.forget();
  return NS_OK;
}
#elif DNS_API == DNS_API_PORTABLE




static MOZ_ALWAYS_INLINE nsresult
_GetAddrInfo_Portable(const char* aHost, uint16_t aAddressFamily,
                      uint16_t aFlags, AddrInfo** aAddrInfo)
{
  MOZ_ASSERT(aHost);
  MOZ_ASSERT(aAddrInfo);

  
  
  
  int prFlags = PR_AI_ADDRCONFIG;
  if (!(aFlags & nsHostResolver::RES_CANON_NAME)) {
    prFlags |= PR_AI_NOCANONNAME;
  }

  
  
  bool disableIPv4 = aAddressFamily == PR_AF_INET6;
  if (disableIPv4) {
    aAddressFamily = PR_AF_UNSPEC;
  }

  PRAddrInfo* prai = PR_GetAddrInfoByName(aHost, aAddressFamily, prFlags);

  if (!prai) {
    return NS_ERROR_UNKNOWN_HOST;
  }

  const char* canonName = nullptr;
  if (aFlags & nsHostResolver::RES_CANON_NAME) {
    canonName = PR_GetCanonNameFromAddrInfo(prai);
  }

  nsAutoPtr<AddrInfo> ai(new AddrInfo(aHost, prai, disableIPv4, canonName));
  PR_FreeAddrInfo(prai);
  if (ai->mAddresses.isEmpty()) {
    return NS_ERROR_UNKNOWN_HOST;
  }

  *aAddrInfo = ai.forget();

  return NS_OK;
}
#endif




nsresult
GetAddrInfoInit() {
  LOG("Initializing GetAddrInfo.\n");

#if DNS_API == DNS_API_WINDOWS_DNS_QUERY
  return _GetAddrInfoInit_Windows();
#else
  return NS_OK;
#endif
}

nsresult
GetAddrInfoShutdown() {
  LOG("Shutting down GetAddrInfo.\n");

#if DNS_API == DNS_API_WINDOWS_DNS_QUERY
  return _GetAddrInfoShutdown_Windows();
#else
  return NS_OK;
#endif
}

nsresult
GetAddrInfo(const char* aHost, uint16_t aAddressFamily, uint16_t aFlags,
            AddrInfo** aAddrInfo)
{
  if (NS_WARN_IF(!aHost) || NS_WARN_IF(!aAddrInfo)) {
    return NS_ERROR_NULL_POINTER;
  }

  *aAddrInfo = nullptr;

#if DNS_API == DNS_API_WINDOWS_DNS_QUERY
  return _GetAddrInfo_Windows(aHost, aAddressFamily, aFlags, aAddrInfo);
#elif DNS_API == DNS_API_PORTABLE
  return _GetAddrInfo_Portable(aHost, aAddressFamily, aFlags, aAddrInfo);
#else
  
  
  
  #error Unknown or unspecified DNS_API.
#endif
}

} 
} 
