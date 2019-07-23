





































#include "nsCookie.h"
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












static PRInt64 gLastCreationID;

nsCookie *
nsCookie::Create(const nsACString &aName,
                 const nsACString &aValue,
                 const nsACString &aHost,
                 const nsACString &aPath,
                 PRInt64           aExpiry,
                 PRInt64           aCreationID,
                 PRBool            aIsSession,
                 PRBool            aIsSecure,
                 PRBool            aIsHttpOnly)
{
  
  const PRUint32 stringLength = aName.Length() + aValue.Length() +
                                aHost.Length() + aPath.Length() + 4;

  
  
  void *place = ::operator new(sizeof(nsCookie) + stringLength);
  if (!place)
    return nsnull;

  
  char *name, *value, *host, *path, *end;
  name = NS_STATIC_CAST(char *, place) + sizeof(nsCookie);
  StrBlockCopy(aName, aValue, aHost, aPath,
               name, value, host, path, end);

  
  
  if (aCreationID > gLastCreationID)
    gLastCreationID = aCreationID;
  else
    aCreationID = ++gLastCreationID;

  
  return new (place) nsCookie(name, value, host, path, end,
                              aExpiry, aCreationID,
                              aIsSession, aIsSecure, aIsHttpOnly);
}







NS_IMETHODIMP nsCookie::GetName(nsACString &aName)         { aName = Name();            return NS_OK; }
NS_IMETHODIMP nsCookie::GetValue(nsACString &aValue)       { aValue = Value();          return NS_OK; }
NS_IMETHODIMP nsCookie::GetHost(nsACString &aHost)         { aHost = Host();            return NS_OK; }
NS_IMETHODIMP nsCookie::GetRawHost(nsACString &aHost)      { aHost = RawHost();         return NS_OK; }
NS_IMETHODIMP nsCookie::GetPath(nsACString &aPath)         { aPath = Path();            return NS_OK; }
NS_IMETHODIMP nsCookie::GetExpiry(PRInt64 *aExpiry)        { *aExpiry = Expiry();       return NS_OK; }
NS_IMETHODIMP nsCookie::GetIsSession(PRBool *aIsSession)   { *aIsSession = IsSession(); return NS_OK; }
NS_IMETHODIMP nsCookie::GetIsDomain(PRBool *aIsDomain)     { *aIsDomain = IsDomain();   return NS_OK; }
NS_IMETHODIMP nsCookie::GetIsSecure(PRBool *aIsSecure)     { *aIsSecure = IsSecure();   return NS_OK; }
NS_IMETHODIMP nsCookie::GetIsHttpOnly(PRBool *aHttpOnly)   { *aHttpOnly = IsHttpOnly(); return NS_OK; }
NS_IMETHODIMP nsCookie::GetStatus(nsCookieStatus *aStatus) { *aStatus = 0;              return NS_OK; }
NS_IMETHODIMP nsCookie::GetPolicy(nsCookiePolicy *aPolicy) { *aPolicy = 0;              return NS_OK; }



NS_IMETHODIMP
nsCookie::GetExpires(PRUint64 *aExpires)
{
  if (IsSession()) {
    *aExpires = 0;
  } else {
    *aExpires = Expiry() > 0 ? Expiry() : 1;
  }
  return NS_OK;
}

NS_IMPL_ISUPPORTS2(nsCookie, nsICookie2, nsICookie)
