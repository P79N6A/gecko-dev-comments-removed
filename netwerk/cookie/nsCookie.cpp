




#include "nsCookie.h"
#include "nsUTF8ConverterService.h"
#include <stdlib.h>








static inline void
StrBlockCopy(const nsACString &aSource1,
             const nsACString &aSource2,
             const nsACString &aSource3,
             const nsACString &aSource4,
             char             *&aDest1,
             char             *&aDest2,
             char             *&aDest3,
             char             *&aDest4,
             char             *&aDestEnd)
{
  char *toBegin = aDest1;
  nsACString::const_iterator fromBegin, fromEnd;

  *copy_string(aSource1.BeginReading(fromBegin), aSource1.EndReading(fromEnd), toBegin) = char(0);
  aDest2 = ++toBegin;
  *copy_string(aSource2.BeginReading(fromBegin), aSource2.EndReading(fromEnd), toBegin) = char(0);
  aDest3 = ++toBegin;
  *copy_string(aSource3.BeginReading(fromBegin), aSource3.EndReading(fromEnd), toBegin) = char(0);
  aDest4 = ++toBegin;
  *copy_string(aSource4.BeginReading(fromBegin), aSource4.EndReading(fromEnd), toBegin) = char(0);
  aDestEnd = toBegin;
}













static int64_t gLastCreationTime;

int64_t
nsCookie::GenerateUniqueCreationTime(int64_t aCreationTime)
{
  
  
  if (aCreationTime > gLastCreationTime) {
    gLastCreationTime = aCreationTime;
    return aCreationTime;
  }

  
  return ++gLastCreationTime;
}

nsCookie *
nsCookie::Create(const nsACString &aName,
                 const nsACString &aValue,
                 const nsACString &aHost,
                 const nsACString &aPath,
                 int64_t           aExpiry,
                 int64_t           aLastAccessed,
                 int64_t           aCreationTime,
                 bool              aIsSession,
                 bool              aIsSecure,
                 bool              aIsHttpOnly)
{
  
  
  nsUTF8ConverterService converter;
  nsAutoCString aUTF8Value;
  converter.ConvertStringToUTF8(aValue, "UTF-8", false, true, 1, aUTF8Value);

  
  const uint32_t stringLength = aName.Length() + aUTF8Value.Length() +
                                aHost.Length() + aPath.Length() + 4;

  
  
  void *place = ::operator new(sizeof(nsCookie) + stringLength);
  if (!place)
    return nullptr;

  
  char *name, *value, *host, *path, *end;
  name = static_cast<char *>(place) + sizeof(nsCookie);
  StrBlockCopy(aName, aUTF8Value, aHost, aPath,
               name, value, host, path, end);

  
  
  if (aCreationTime > gLastCreationTime)
    gLastCreationTime = aCreationTime;

  
  return new (place) nsCookie(name, value, host, path, end,
                              aExpiry, aLastAccessed, aCreationTime,
                              aIsSession, aIsSecure, aIsHttpOnly);
}

size_t
nsCookie::SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
{
    
    
    return aMallocSizeOf(this);
}







NS_IMETHODIMP nsCookie::GetName(nsACString &aName)         { aName = Name();            return NS_OK; }
NS_IMETHODIMP nsCookie::GetValue(nsACString &aValue)       { aValue = Value();          return NS_OK; }
NS_IMETHODIMP nsCookie::GetHost(nsACString &aHost)         { aHost = Host();            return NS_OK; }
NS_IMETHODIMP nsCookie::GetRawHost(nsACString &aHost)      { aHost = RawHost();         return NS_OK; }
NS_IMETHODIMP nsCookie::GetPath(nsACString &aPath)         { aPath = Path();            return NS_OK; }
NS_IMETHODIMP nsCookie::GetExpiry(int64_t *aExpiry)        { *aExpiry = Expiry();       return NS_OK; }
NS_IMETHODIMP nsCookie::GetIsSession(bool *aIsSession)   { *aIsSession = IsSession(); return NS_OK; }
NS_IMETHODIMP nsCookie::GetIsDomain(bool *aIsDomain)     { *aIsDomain = IsDomain();   return NS_OK; }
NS_IMETHODIMP nsCookie::GetIsSecure(bool *aIsSecure)     { *aIsSecure = IsSecure();   return NS_OK; }
NS_IMETHODIMP nsCookie::GetIsHttpOnly(bool *aHttpOnly)   { *aHttpOnly = IsHttpOnly(); return NS_OK; }
NS_IMETHODIMP nsCookie::GetStatus(nsCookieStatus *aStatus) { *aStatus = 0;              return NS_OK; }
NS_IMETHODIMP nsCookie::GetPolicy(nsCookiePolicy *aPolicy) { *aPolicy = 0;              return NS_OK; }
NS_IMETHODIMP nsCookie::GetCreationTime(int64_t *aCreation){ *aCreation = CreationTime(); return NS_OK; }
NS_IMETHODIMP nsCookie::GetLastAccessed(int64_t *aTime)    { *aTime = LastAccessed();   return NS_OK; }



NS_IMETHODIMP
nsCookie::GetExpires(uint64_t *aExpires)
{
  if (IsSession()) {
    *aExpires = 0;
  } else {
    *aExpires = Expiry() > 0 ? Expiry() : 1;
  }
  return NS_OK;
}

NS_IMPL_ISUPPORTS(nsCookie, nsICookie2, nsICookie)
