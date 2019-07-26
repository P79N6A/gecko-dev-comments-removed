








#include "mozilla/Util.h"

#include "nsEffectiveTLDService.h"
#include "nsIIDNService.h"
#include "nsIMemoryReporter.h"
#include "nsNetUtil.h"
#include "prnetdb.h"

using namespace mozilla;

NS_IMPL_ISUPPORTS1(nsEffectiveTLDService, nsIEffectiveTLDService)



#define ETLD_STR_NUM_1(line) str##line
#define ETLD_STR_NUM(line) ETLD_STR_NUM_1(line)
#define ETLD_ENTRY_OFFSET(name) offsetof(struct etld_string_list, ETLD_STR_NUM(__LINE__))

const ETLDEntry nsDomainEntry::entries[] = {
#define ETLD_ENTRY(name, ex, wild) { ETLD_ENTRY_OFFSET(name), ex, wild },
#include "etld_data.inc"
#undef ETLD_ENTRY
};

const union nsDomainEntry::etld_strings nsDomainEntry::strings = {
  {
#define ETLD_ENTRY(name, ex, wild) name,
#include "etld_data.inc"
#undef ETLD_ENTRY
  }
};



void
nsDomainEntry::FuncForStaticAsserts(void)
{
#define ETLD_ENTRY(name, ex, wild)                                      \
  MOZ_STATIC_ASSERT(ETLD_ENTRY_OFFSET(name) < (1 << ETLD_ENTRY_N_INDEX_BITS), \
                    "invalid strtab index");
#include "etld_data.inc"
#undef ETLD_ENTRY
}

#undef ETLD_ENTRY_OFFSET
#undef ETLD_STR_NUM
#undef ETLD_STR_NUM1



static nsEffectiveTLDService *gService = nullptr;

NS_MEMORY_REPORTER_MALLOC_SIZEOF_FUN(EffectiveTLDServiceMallocSizeOf)

static int64_t
GetEffectiveTLDSize()
{
  return gService->SizeOfIncludingThis(EffectiveTLDServiceMallocSizeOf);
}

NS_MEMORY_REPORTER_IMPLEMENT(
  EffectiveTLDService,
  "explicit/xpcom/effective-TLD-service",
  KIND_HEAP,
  nsIMemoryReporter::UNITS_BYTES,
  GetEffectiveTLDSize,
  "Memory used by the effective TLD service.")

nsresult
nsEffectiveTLDService::Init()
{
  const ETLDEntry *entries = nsDomainEntry::entries;

  
  
  
  
  mHash.Init(ArrayLength(nsDomainEntry::entries));

  nsresult rv;
  mIDNService = do_GetService(NS_IDNSERVICE_CONTRACTID, &rv);
  if (NS_FAILED(rv)) return rv;

  
  for (uint32_t i = 0; i < ArrayLength(nsDomainEntry::entries); i++) {
    const char *domain = nsDomainEntry::GetEffectiveTLDName(entries[i].strtab_index);
#ifdef DEBUG
    nsDependentCString name(domain);
    nsAutoCString normalizedName(domain);
    NS_ASSERTION(NS_SUCCEEDED(NormalizeHostname(normalizedName)),
                 "normalization failure!");
    NS_ASSERTION(name.Equals(normalizedName), "domain not normalized!");
#endif
    nsDomainEntry *entry = mHash.PutEntry(domain);
    NS_ENSURE_TRUE(entry, NS_ERROR_OUT_OF_MEMORY);
    entry->SetData(&entries[i]);
  }

  MOZ_ASSERT(!gService);
  gService = this;
  mReporter = new NS_MEMORY_REPORTER_NAME(EffectiveTLDService);
  (void)::NS_RegisterMemoryReporter(mReporter);

  return NS_OK;
}

nsEffectiveTLDService::~nsEffectiveTLDService()
{
  (void)::NS_UnregisterMemoryReporter(mReporter);
  mReporter = nullptr;
  gService = nullptr;
}

size_t
nsEffectiveTLDService::SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf)
{
  size_t n = aMallocSizeOf(this);
  n += mHash.SizeOfExcludingThis(nullptr, aMallocSizeOf);

  
  
  
  

  return n;
}




NS_IMETHODIMP
nsEffectiveTLDService::GetPublicSuffix(nsIURI     *aURI,
                                       nsACString &aPublicSuffix)
{
  NS_ENSURE_ARG_POINTER(aURI);

  nsCOMPtr<nsIURI> innerURI = NS_GetInnermostURI(aURI);
  NS_ENSURE_ARG_POINTER(innerURI);

  nsAutoCString host;
  nsresult rv = innerURI->GetAsciiHost(host);
  if (NS_FAILED(rv)) return rv;

  return GetBaseDomainInternal(host, 0, aPublicSuffix);
}




NS_IMETHODIMP
nsEffectiveTLDService::GetBaseDomain(nsIURI     *aURI,
                                     uint32_t    aAdditionalParts,
                                     nsACString &aBaseDomain)
{
  NS_ENSURE_ARG_POINTER(aURI);

  nsCOMPtr<nsIURI> innerURI = NS_GetInnermostURI(aURI);
  NS_ENSURE_ARG_POINTER(innerURI);

  nsAutoCString host;
  nsresult rv = innerURI->GetAsciiHost(host);
  if (NS_FAILED(rv)) return rv;

  return GetBaseDomainInternal(host, aAdditionalParts + 1, aBaseDomain);
}



NS_IMETHODIMP
nsEffectiveTLDService::GetPublicSuffixFromHost(const nsACString &aHostname,
                                               nsACString       &aPublicSuffix)
{
  
  
  nsAutoCString normHostname(aHostname);
  nsresult rv = NormalizeHostname(normHostname);
  if (NS_FAILED(rv)) return rv;

  return GetBaseDomainInternal(normHostname, 0, aPublicSuffix);
}




NS_IMETHODIMP
nsEffectiveTLDService::GetBaseDomainFromHost(const nsACString &aHostname,
                                             uint32_t          aAdditionalParts,
                                             nsACString       &aBaseDomain)
{
  
  
  nsAutoCString normHostname(aHostname);
  nsresult rv = NormalizeHostname(normHostname);
  if (NS_FAILED(rv)) return rv;

  return GetBaseDomainInternal(normHostname, aAdditionalParts + 1, aBaseDomain);
}






nsresult
nsEffectiveTLDService::GetBaseDomainInternal(nsCString  &aHostname,
                                             uint32_t    aAdditionalParts,
                                             nsACString &aBaseDomain)
{
  if (aHostname.IsEmpty())
    return NS_ERROR_INSUFFICIENT_DOMAIN_LEVELS;

  
  bool trailingDot = aHostname.Last() == '.';
  if (trailingDot)
    aHostname.Truncate(aHostname.Length() - 1);

  
  
  if (aHostname.IsEmpty() || aHostname.Last() == '.')
    return NS_ERROR_INVALID_ARG;

  
  PRNetAddr addr;
  PRStatus result = PR_StringToNetAddr(aHostname.get(), &addr);
  if (result == PR_SUCCESS)
    return NS_ERROR_HOST_IS_IP_ADDRESS;

  
  
  
  const char *prevDomain = nullptr;
  const char *currDomain = aHostname.get();
  const char *nextDot = strchr(currDomain, '.');
  const char *end = currDomain + aHostname.Length();
  const char *eTLD = currDomain;
  while (1) {
    
    
    
    if (*currDomain == '.')
      return NS_ERROR_INVALID_ARG;

    
    nsDomainEntry *entry = mHash.GetEntry(currDomain);
    if (entry) {
      if (entry->IsWild() && prevDomain) {
        
        eTLD = prevDomain;
        break;

      } else if (entry->IsNormal() || !nextDot) {
        
        eTLD = currDomain;
        break;

      } else if (entry->IsException()) {
        
        eTLD = nextDot + 1;
        break;
      }
    }

    if (!nextDot) {
      
      eTLD = currDomain;
      break;
    }

    prevDomain = currDomain;
    currDomain = nextDot + 1;
    nextDot = strchr(currDomain, '.');
  }

  
  const char *begin = aHostname.get();
  const char *iter = eTLD;
  while (1) {
    if (iter == begin)
      break;

    if (*(--iter) == '.' && aAdditionalParts-- == 0) {
      ++iter;
      ++aAdditionalParts;
      break;
    }
  }

  if (aAdditionalParts != 0)
    return NS_ERROR_INSUFFICIENT_DOMAIN_LEVELS;

  aBaseDomain = Substring(iter, end);
  
  if (trailingDot)
    aBaseDomain.Append('.');

  return NS_OK;
}




nsresult
nsEffectiveTLDService::NormalizeHostname(nsCString &aHostname)
{
  if (!IsASCII(aHostname)) {
    nsresult rv = mIDNService->ConvertUTF8toACE(aHostname, aHostname);
    if (NS_FAILED(rv))
      return rv;
  }

  ToLowerCase(aHostname);
  return NS_OK;
}
