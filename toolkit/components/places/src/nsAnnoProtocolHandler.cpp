














































#include "nsAnnoProtocolHandler.h"
#include "nsFaviconService.h"
#include "nsIChannel.h"
#include "nsIInputStreamChannel.h"
#include "nsILoadGroup.h"
#include "nsIStandardURL.h"
#include "nsIStringStream.h"
#include "nsISupportsUtils.h"
#include "nsIURI.h"
#include "nsNetUtil.h"
#include "nsServiceManagerUtils.h"
#include "nsStringStream.h"

NS_IMPL_ISUPPORTS1(nsAnnoProtocolHandler, nsIProtocolHandler)



NS_IMETHODIMP
nsAnnoProtocolHandler::GetScheme(nsACString& aScheme)
{
  aScheme.AssignLiteral("moz-anno");
  return NS_OK;
}






NS_IMETHODIMP
nsAnnoProtocolHandler::GetDefaultPort(PRInt32 *aDefaultPort)
{
  *aDefaultPort = -1;
  return NS_OK;
}




NS_IMETHODIMP
nsAnnoProtocolHandler::GetProtocolFlags(PRUint32 *aProtocolFlags)
{
  *aProtocolFlags = (URI_NORELATIVE | URI_NOAUTH | URI_DANGEROUS_TO_LOAD);
  return NS_OK;
}




NS_IMETHODIMP
nsAnnoProtocolHandler::NewURI(const nsACString& aSpec,
                              const char *aOriginCharset,
                              nsIURI *aBaseURI, nsIURI **_retval)
{
  nsCOMPtr <nsIURI> uri = do_CreateInstance(NS_SIMPLEURI_CONTRACTID);
  if (!uri)
    return NS_ERROR_OUT_OF_MEMORY;
  nsresult rv = uri->SetSpec(aSpec);
  NS_ENSURE_SUCCESS(rv, rv);

  *_retval = nsnull;
  uri.swap(*_retval);
  return NS_OK;
}





NS_IMETHODIMP
nsAnnoProtocolHandler::NewChannel(nsIURI *aURI, nsIChannel **_retval)
{
  NS_ENSURE_ARG_POINTER(aURI);
  nsresult rv;

  nsCAutoString path;
  rv = aURI->GetPath(path);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIAnnotationService> annotationService = do_GetService(
                              "@mozilla.org/browser/annotation-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIURI> annoURI;
  nsCAutoString annoName;
  rv = ParseAnnoURI(aURI, getter_AddRefs(annoURI), annoName);
  NS_ENSURE_SUCCESS(rv, rv);

  
  PRUint8* data;
  PRUint32 dataLen;
  nsCAutoString mimeType;

  if (annoName.EqualsLiteral(FAVICON_ANNOTATION_NAME)) {
    
    nsFaviconService* faviconService = nsFaviconService::GetFaviconService();
    if (! faviconService) {
      NS_WARNING("Favicon service is unavailable.");
      return GetDefaultIcon(_retval);
    }
    rv = faviconService->GetFaviconData(annoURI, mimeType, &dataLen, &data);
    if (NS_FAILED(rv))
      return GetDefaultIcon(_retval);

    
    if (mimeType.IsEmpty()) {
      NS_Free(data);
      return GetDefaultIcon(_retval);
    }
  } else {
    
    rv = annotationService->GetPageAnnotationBinary(annoURI, annoName, &data,
                                                    &dataLen, mimeType);
    NS_ENSURE_SUCCESS(rv, rv);

    
    if (mimeType.IsEmpty()) {
      NS_Free(data);
      return NS_ERROR_NOT_AVAILABLE;
    }
  }

  nsCOMPtr<nsIStringInputStream> stream = do_CreateInstance(
                                          NS_STRINGINPUTSTREAM_CONTRACTID, &rv);
  if (NS_FAILED(rv)) {
    NS_Free(data);
    return rv;
  }
  rv = stream->AdoptData((char*)data, dataLen);
  if (NS_FAILED(rv)) {
    NS_Free(data);
    return rv;
  }

  nsCOMPtr<nsIChannel> channel;
  rv = NS_NewInputStreamChannel(getter_AddRefs(channel), aURI, stream, mimeType);
  NS_ENSURE_SUCCESS(rv, rv);

  *_retval = channel;
  NS_ADDREF(*_retval);
  return NS_OK;
}






NS_IMETHODIMP
nsAnnoProtocolHandler::AllowPort(PRInt32 port, const char *scheme,
                                 PRBool *_retval)
{
  *_retval = PR_FALSE;
  return NS_OK;
}






nsresult
nsAnnoProtocolHandler::ParseAnnoURI(nsIURI* aURI,
                                    nsIURI** aResultURI, nsCString& aName)
{
  nsresult rv;
  nsCAutoString path;
  rv = aURI->GetPath(path);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 firstColon = path.FindChar(':');
  if (firstColon <= 0)
    return NS_ERROR_MALFORMED_URI;

  rv = NS_NewURI(aResultURI, Substring(path, firstColon + 1));
  NS_ENSURE_SUCCESS(rv, rv);

  aName = Substring(path, 0, firstColon);
  return NS_OK;
}






nsresult
nsAnnoProtocolHandler::GetDefaultIcon(nsIChannel** aChannel)
{
  nsresult rv;
  nsCOMPtr<nsIURI> uri;
  rv = NS_NewURI(getter_AddRefs(uri), NS_LITERAL_CSTRING(FAVICON_DEFAULT_URL));
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_NewChannel(aChannel, uri);
}
