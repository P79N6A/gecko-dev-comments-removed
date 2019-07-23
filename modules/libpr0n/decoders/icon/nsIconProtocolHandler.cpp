






































#include "nsIconChannel.h"
#include "nsIconURI.h"
#include "nsIconProtocolHandler.h"
#include "nsIURL.h"
#include "nsCRT.h"
#include "nsCOMPtr.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsNetCID.h"



nsIconProtocolHandler::nsIconProtocolHandler() 
{
}

nsIconProtocolHandler::~nsIconProtocolHandler() 
{}

NS_IMPL_ISUPPORTS2(nsIconProtocolHandler, nsIProtocolHandler, nsISupportsWeakReference)

    



NS_IMETHODIMP nsIconProtocolHandler::GetScheme(nsACString &result) 
{
  result = "moz-icon";
  return NS_OK;
}

NS_IMETHODIMP nsIconProtocolHandler::GetDefaultPort(PRInt32 *result) 
{
  *result = 0;
  return NS_OK;
}

NS_IMETHODIMP nsIconProtocolHandler::AllowPort(PRInt32 port, const char *scheme, PRBool *_retval)
{
    
    *_retval = PR_FALSE;
    return NS_OK;
}

NS_IMETHODIMP nsIconProtocolHandler::GetProtocolFlags(PRUint32 *result) 
{
  *result = URI_NORELATIVE | URI_NOAUTH | URI_IS_UI_RESOURCE;
  return NS_OK;
}

NS_IMETHODIMP nsIconProtocolHandler::NewURI(const nsACString &aSpec,
                                            const char *aOriginCharset, 
                                            nsIURI *aBaseURI,
                                            nsIURI **result) 
{
  
  nsCOMPtr<nsIURI> uri;
  NS_NEWXPCOM(uri, nsMozIconURI);
  if (!uri) return NS_ERROR_OUT_OF_MEMORY;

  nsresult rv = uri->SetSpec(aSpec);
  if (NS_FAILED(rv)) return rv;

  NS_ADDREF(*result = uri);
  return NS_OK;
}

NS_IMETHODIMP nsIconProtocolHandler::NewChannel(nsIURI* url, nsIChannel* *result)
{
  NS_ENSURE_ARG_POINTER(url);
  nsIconChannel* channel = new nsIconChannel;
  if (!channel)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(channel);

  nsresult rv = channel->Init(url);
  if (NS_FAILED(rv)) {
    NS_RELEASE(channel);
    return rv;
  }

  *result = channel;
  return NS_OK;
}


