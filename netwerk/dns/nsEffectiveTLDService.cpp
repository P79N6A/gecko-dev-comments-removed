











































#include "nsEffectiveTLDService.h"
#include "nsIIDNService.h"
#include "nsNetUtil.h"
#include "prnetdb.h"

#include "mozilla/FunctionTimer.h"

NS_IMPL_ISUPPORTS1(nsEffectiveTLDService, nsIEffectiveTLDService)



static const ETLDEntry gEntries[] =
#include "etld_data.inc"
;



nsresult
nsEffectiveTLDService::Init()
{
  NS_TIME_FUNCTION;

  
  
  
  
  if (!mHash.Init(NS_ARRAY_LENGTH(gEntries) - 1))
    return NS_ERROR_OUT_OF_MEMORY;

  nsresult rv;
  mIDNService = do_GetService(NS_IDNSERVICE_CONTRACTID, &rv);
  if (NS_FAILED(rv)) return rv;

  
  for (PRUint32 i = 0; i < NS_ARRAY_LENGTH(gEntries) - 1; i++) {
#ifdef DEBUG
    nsDependentCString name(gEntries[i].domain);
    nsCAutoString normalizedName(gEntries[i].domain);
    NS_ASSERTION(NS_SUCCEEDED(NormalizeHostname(normalizedName)),
                 "normalization failure!");
    NS_ASSERTION(name.Equals(normalizedName), "domain not normalized!");
#endif
    nsDomainEntry *entry = mHash.PutEntry(gEntries[i].domain);
    NS_ENSURE_TRUE(entry, NS_ERROR_OUT_OF_MEMORY);
    entry->SetData(&gEntries[i]);
  }
  return NS_OK;
}




NS_IMETHODIMP
nsEffectiveTLDService::GetPublicSuffix(nsIURI     *aURI,
                                       nsACString &aPublicSuffix)
{
  NS_ENSURE_ARG_POINTER(aURI);

  nsCOMPtr<nsIURI> innerURI = NS_GetInnermostURI(aURI);
  NS_ENSURE_ARG_POINTER(innerURI);

  nsCAutoString host;
  nsresult rv = innerURI->GetAsciiHost(host);
  if (NS_FAILED(rv)) return rv;

  return GetBaseDomainInternal(host, 0, aPublicSuffix);
}




NS_IMETHODIMP
nsEffectiveTLDService::GetBaseDomain(nsIURI     *aURI,
                                     PRUint32    aAdditionalParts,
                                     nsACString &aBaseDomain)
{
  NS_ENSURE_ARG_POINTER(aURI);

  nsCOMPtr<nsIURI> innerURI = NS_GetInnermostURI(aURI);
  NS_ENSURE_ARG_POINTER(innerURI);

  nsCAutoString host;
  nsresult rv = innerURI->GetAsciiHost(host);
  if (NS_FAILED(rv)) return rv;

  return GetBaseDomainInternal(host, aAdditionalParts + 1, aBaseDomain);
}



NS_IMETHODIMP
nsEffectiveTLDService::GetPublicSuffixFromHost(const nsACString &aHostname,
                                               nsACString       &aPublicSuffix)
{
  
  
  nsCAutoString normHostname(aHostname);
  nsresult rv = NormalizeHostname(normHostname);
  if (NS_FAILED(rv)) return rv;

  return GetBaseDomainInternal(normHostname, 0, aPublicSuffix);
}




NS_IMETHODIMP
nsEffectiveTLDService::GetBaseDomainFromHost(const nsACString &aHostname,
                                             PRUint32          aAdditionalParts,
                                             nsACString       &aBaseDomain)
{
  
  
  nsCAutoString normHostname(aHostname);
  nsresult rv = NormalizeHostname(normHostname);
  if (NS_FAILED(rv)) return rv;

  return GetBaseDomainInternal(normHostname, aAdditionalParts + 1, aBaseDomain);
}






nsresult
nsEffectiveTLDService::GetBaseDomainInternal(nsCString  &aHostname,
                                             PRUint32    aAdditionalParts,
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

  
  
  
  const char *prevDomain = nsnull;
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
