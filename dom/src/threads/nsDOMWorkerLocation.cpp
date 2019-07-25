





































#include "nsDOMWorkerLocation.h"

#include "nsIClassInfoImpl.h"
#include "nsITextToSubURI.h"
#include "nsIURL.h"

#include "nsDOMWorkerMacros.h"

#include "nsAutoPtr.h"
#include "nsEscape.h"
#include "nsNetUtil.h"

#define XPC_MAP_CLASSNAME nsDOMWorkerLocation
#define XPC_MAP_QUOTED_CLASSNAME "WorkerLocation"

#define XPC_MAP_FLAGS                                      \
  nsIXPCScriptable::USE_JSSTUB_FOR_ADDPROPERTY           | \
  nsIXPCScriptable::USE_JSSTUB_FOR_DELPROPERTY           | \
  nsIXPCScriptable::USE_JSSTUB_FOR_SETPROPERTY           | \
  nsIXPCScriptable::DONT_ENUM_QUERY_INTERFACE            | \
  nsIXPCScriptable::CLASSINFO_INTERFACES_ONLY            | \
  nsIXPCScriptable::DONT_REFLECT_INTERFACE_NAMES

#include "xpc_map_end.h"

NS_IMPL_THREADSAFE_ISUPPORTS3(nsDOMWorkerLocation, nsIWorkerLocation,
                                                   nsIClassInfo,
                                                   nsIXPCScriptable)

NS_IMPL_CI_INTERFACE_GETTER1(nsDOMWorkerLocation, nsIWorkerLocation)

NS_IMPL_THREADSAFE_DOM_CI_GETINTERFACES(nsDOMWorkerLocation)
NS_IMPL_THREADSAFE_DOM_CI_ALL_THE_REST(nsDOMWorkerLocation)

NS_IMETHODIMP
nsDOMWorkerLocation::GetHelperForLanguage(PRUint32 aLanguage,
                                          nsISupports** _retval)
{
  if (aLanguage == nsIProgrammingLanguage::JAVASCRIPT) {
    NS_ADDREF(*_retval = NS_ISUPPORTS_CAST(nsIWorkerLocation*, this));
  }
  else {
    *_retval = nsnull;
  }
  return NS_OK;
}


already_AddRefed<nsIWorkerLocation>
nsDOMWorkerLocation::NewLocation(nsIURL* aURL)
{
  NS_ASSERTION(aURL, "Don't hand me a null pointer!");

  nsAutoPtr<nsDOMWorkerLocation> location(new nsDOMWorkerLocation());
  NS_ENSURE_TRUE(location, nsnull);

  nsresult rv = aURL->GetSpec(location->mHref);
  NS_ENSURE_SUCCESS(rv, nsnull);

  rv = aURL->GetHost(location->mHostname);
  NS_ENSURE_SUCCESS(rv, nsnull);

  rv = aURL->GetPath(location->mPathname);
  NS_ENSURE_SUCCESS(rv, nsnull);

  nsCString temp;

  rv = aURL->GetQuery(temp);
  NS_ENSURE_SUCCESS(rv, nsnull);
  if (!temp.IsEmpty()) {
    location->mSearch.AssignLiteral("?");
    location->mSearch.Append(temp);
  }

  rv = aURL->GetRef(temp);
  NS_ENSURE_SUCCESS(rv, nsnull);

  if (!temp.IsEmpty()) {
    nsAutoString unicodeRef;

    nsCOMPtr<nsITextToSubURI> converter =
      do_GetService(NS_ITEXTTOSUBURI_CONTRACTID, &rv);
    if (NS_SUCCEEDED(rv)) {
      nsCString charset;
      rv = aURL->GetOriginCharset(charset);
      if (NS_SUCCEEDED(rv)) {
        rv = converter->UnEscapeURIForUI(charset, temp, unicodeRef);
        if (NS_SUCCEEDED(rv)) {
          location->mHash.AssignLiteral("#");
          location->mHash.Append(NS_ConvertUTF16toUTF8(unicodeRef));
        }
      }
    }

    if (NS_FAILED(rv)) {
      location->mHash.AssignLiteral("#");
      location->mHash.Append(temp);
    }
  }

  rv = aURL->GetScheme(location->mProtocol);
  NS_ENSURE_SUCCESS(rv, nsnull);

  location->mProtocol.AppendLiteral(":");

  PRInt32 port;
  rv = aURL->GetPort(&port);
  if (NS_SUCCEEDED(rv) && port != -1) {
    location->mPort.AppendInt(port);

    nsCAutoString host(location->mHostname);
    host.AppendLiteral(":");
    host.Append(location->mPort);

    location->mHost.Assign(host);
  }
  else {
    location->mHost.Assign(location->mHostname);
  }

  NS_ADDREF(location);
  return location.forget();
}

NS_IMETHODIMP
nsDOMWorkerLocation::GetHref(nsACString& aHref)
{
  aHref.Assign(mHref);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMWorkerLocation::GetProtocol(nsACString& aProtocol)
{
  aProtocol.Assign(mProtocol);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMWorkerLocation::GetHost(nsACString& aHost)
{
  aHost.Assign(mHost);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMWorkerLocation::GetHostname(nsACString& aHostname)
{
  aHostname.Assign(mHostname);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMWorkerLocation::GetPort(nsACString& aPort)
{
  aPort.Assign(mPort);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMWorkerLocation::GetPathname(nsACString& aPathname)
{
  aPathname.Assign(mPathname);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMWorkerLocation::GetSearch(nsACString& aSearch)
{
  aSearch.Assign(mSearch);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMWorkerLocation::GetHash(nsACString& aHash)
{
  aHash.Assign(mHash);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMWorkerLocation::ToString(nsACString& _retval)
{
  return GetHref(_retval);
}
