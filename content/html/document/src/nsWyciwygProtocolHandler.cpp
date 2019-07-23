






































#include "nsWyciwygChannel.h"
#include "nsWyciwygProtocolHandler.h"
#include "nsIURL.h"
#include "nsIComponentManager.h"
#include "nsNetCID.h"



nsWyciwygProtocolHandler::nsWyciwygProtocolHandler() 
{

#if defined(PR_LOGGING)
  gWyciwygLog = PR_NewLogModule("nsWyciwygChannel");
#endif
}

nsWyciwygProtocolHandler::~nsWyciwygProtocolHandler() 
{
}

NS_IMPL_ISUPPORTS1(nsWyciwygProtocolHandler, nsIProtocolHandler)





NS_IMETHODIMP
nsWyciwygProtocolHandler::GetScheme(nsACString &result)
{
  result = "wyciwyg";
  return NS_OK;
}

NS_IMETHODIMP
nsWyciwygProtocolHandler::GetDefaultPort(PRInt32 *result) 
{
  return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP 
nsWyciwygProtocolHandler::AllowPort(PRInt32 port, const char *scheme, PRBool *_retval)
{
  
  *_retval = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
nsWyciwygProtocolHandler::NewURI(const nsACString &aSpec,
                                 const char *aCharset, 
                                 nsIURI *aBaseURI,
                                 nsIURI **result) 
{
  nsresult rv;

  nsCOMPtr<nsIURI> url = do_CreateInstance(NS_SIMPLEURI_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = url->SetSpec(aSpec);
  NS_ENSURE_SUCCESS(rv, rv);

  *result = url;
  NS_ADDREF(*result);

  return rv;
}

NS_IMETHODIMP
nsWyciwygProtocolHandler::NewChannel(nsIURI* url, nsIChannel* *result)
{
  NS_ENSURE_ARG_POINTER(url);
  nsresult rv;
    
  nsWyciwygChannel* channel = new nsWyciwygChannel();
  if (!channel)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(channel);
  rv = channel->Init(url);
  if (NS_FAILED(rv)) {
    NS_RELEASE(channel);
    return rv;
  }

  *result = channel;
  return NS_OK;
}

NS_IMETHODIMP
nsWyciwygProtocolHandler::GetProtocolFlags(PRUint32 *result) 
{
  
  

  
  
  
  
  
  *result = URI_NORELATIVE | URI_NOAUTH | URI_DANGEROUS_TO_LOAD |
    URI_INHERITS_SECURITY_CONTEXT;
  return NS_OK;
}
