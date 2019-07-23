





































#include "nsHTMLDNSPrefetch.h"
#include "nsCOMPtr.h"
#include "nsString.h"

#include "nsNetUtil.h"

#include "nsIDNSListener.h"
#include "nsIDNSRecord.h"
#include "nsIDNSService.h"
#include "nsICancelable.h"
#include "nsContentUtils.h"
#include "nsGkAtoms.h"
#include "nsIDocument.h"

static NS_DEFINE_CID(kDNSServiceCID, NS_DNSSERVICE_CID);
static PRBool sDisablePrefetchHTTPSPref;
static PRBool sInitialized = PR_FALSE;
static nsIDNSService *sDNSService = nsnull;

nsresult
nsHTMLDNSPrefetch::Initialize()
{
  if (sInitialized) {
    NS_WARNING("Initialize() called twice");
    return NS_OK;
  }
  
  nsContentUtils::AddBoolPrefVarCache("network.dns.disablePrefetchFromHTTPS", 
                                      &sDisablePrefetchHTTPSPref);
  
  
  sDisablePrefetchHTTPSPref = 
    nsContentUtils::GetBoolPref("network.dns.disablePrefetchFromHTTPS", PR_TRUE);
  
  NS_IF_RELEASE(sDNSService);
  nsresult rv;
  rv = CallGetService(kDNSServiceCID, &sDNSService);
  if (NS_FAILED(rv)) return rv;
  
  sInitialized = PR_TRUE;
  return NS_OK;
}

nsresult
nsHTMLDNSPrefetch::Shutdown()
{
  if (!sInitialized) {
    NS_WARNING("Not Initialized");
    return NS_OK;
  }
  sInitialized = PR_FALSE;
  NS_IF_RELEASE(sDNSService);
  return NS_OK;
}

nsHTMLDNSPrefetch::nsHTMLDNSPrefetch(nsAString &hostname, nsIDocument *aDocument)
{
  NS_ASSERTION(aDocument, "Document Required");
  NS_ASSERTION(sInitialized, "nsHTMLDNSPrefetch is not initialized");
  
  mAllowed = IsAllowed(aDocument);
  CopyUTF16toUTF8(hostname, mHostname);
}

nsHTMLDNSPrefetch::nsHTMLDNSPrefetch(nsIURI *aURI, nsIDocument *aDocument)
{
  NS_ASSERTION(aDocument, "Document Required");
  NS_ASSERTION(aURI, "URI Required");
  NS_ASSERTION(sInitialized, "nsHTMLDNSPrefetch is not initialized");

  mAllowed = IsAllowed(aDocument);
  aURI->GetAsciiHost(mHostname);
}

PRBool
nsHTMLDNSPrefetch::IsSecureBaseContext (nsIDocument *aDocument)
{
  nsIURI *docURI = aDocument->GetDocumentURI();
  nsCAutoString scheme;
  docURI->GetScheme(scheme);
  return scheme.EqualsLiteral("https");
}

PRBool
nsHTMLDNSPrefetch::IsAllowed (nsIDocument *aDocument)
{
  if (IsSecureBaseContext(aDocument) && sDisablePrefetchHTTPSPref)
      return PR_FALSE;
    
  
  
  

  nsAutoString control;
  aDocument->GetHeaderData(nsGkAtoms::headerDNSPrefetchControl, control);
  
  if (!control.IsEmpty() && !control.LowerCaseEqualsLiteral("on"))
    return PR_FALSE;

  
  if (!aDocument->GetWindow())
    return PR_FALSE;

  return PR_TRUE;
}

nsresult 
nsHTMLDNSPrefetch::Prefetch(PRUint16 flags)
{
  if (mHostname.IsEmpty())
    return NS_ERROR_NOT_AVAILABLE;
  
  if (!mAllowed)
    return NS_ERROR_NOT_AVAILABLE;

  if (!sDNSService)
    return NS_ERROR_NOT_AVAILABLE;
  
  nsCOMPtr<nsICancelable> tmpOutstanding;
  
  return sDNSService->AsyncResolve(mHostname, flags, this, nsnull,
                                   getter_AddRefs(tmpOutstanding));
}

nsresult
nsHTMLDNSPrefetch::PrefetchLow()
{
  return Prefetch(nsIDNSService::RESOLVE_PRIORITY_LOW);
}

nsresult
nsHTMLDNSPrefetch::PrefetchMedium()
{
  return Prefetch(nsIDNSService::RESOLVE_PRIORITY_MEDIUM);
}

nsresult
nsHTMLDNSPrefetch::PrefetchHigh()
{
  return Prefetch(0);
}

NS_IMPL_THREADSAFE_ISUPPORTS1(nsHTMLDNSPrefetch, nsIDNSListener)

NS_IMETHODIMP
nsHTMLDNSPrefetch::OnLookupComplete(nsICancelable *request,
                                    nsIDNSRecord  *rec,
                                    nsresult       status)
{
  return NS_OK;
}
