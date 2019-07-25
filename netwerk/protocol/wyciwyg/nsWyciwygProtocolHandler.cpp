






































#include "nsWyciwyg.h"
#include "nsWyciwygChannel.h"
#include "nsWyciwygProtocolHandler.h"
#include "nsIURL.h"
#include "nsIComponentManager.h"
#include "nsNetCID.h"
#include "nsServiceManagerUtils.h"
#include "plstr.h"
#include "nsNetUtil.h"

#include "mozilla/net/NeckoChild.h"

using namespace mozilla::net;
#include "mozilla/net/WyciwygChannelChild.h"



nsWyciwygProtocolHandler::nsWyciwygProtocolHandler() 
{
#if defined(PR_LOGGING)
  if (!gWyciwygLog)
    gWyciwygLog = PR_NewLogModule("nsWyciwygChannel");
#endif

  LOG(("Creating nsWyciwygProtocolHandler [this=%x].\n", this));
}

nsWyciwygProtocolHandler::~nsWyciwygProtocolHandler() 
{
  LOG(("Deleting nsWyciwygProtocolHandler [this=%x]\n", this));
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
  if (mozilla::net::IsNeckoChild())
    mozilla::net::NeckoChild::InitNeckoChild();

  NS_ENSURE_ARG_POINTER(url);
  nsresult rv;

  nsCOMPtr<nsIWyciwygChannel> channel;
  if (IsNeckoChild()) {
    NS_ENSURE_TRUE(gNeckoChild != nsnull, NS_ERROR_FAILURE);

    WyciwygChannelChild *wcc = static_cast<WyciwygChannelChild *>(
                                 gNeckoChild->SendPWyciwygChannelConstructor());
    if (!wcc)
      return NS_ERROR_OUT_OF_MEMORY;

    channel = wcc;
    rv = wcc->Init(url);
    if (NS_FAILED(rv))
      PWyciwygChannelChild::Send__delete__(wcc);
  } else
  {
    
    
    nsCAutoString path;
    rv = url->GetPath(path);
    NS_ENSURE_SUCCESS(rv, rv);
    PRInt32 slashIndex = path.FindChar('/', 2);
    if (slashIndex == kNotFound)
      return NS_ERROR_FAILURE;
    if (path.Length() < (PRUint32)slashIndex + 1 + 5)
      return NS_ERROR_FAILURE;
    if (!PL_strncasecmp(path.get() + slashIndex + 1, "https", 5))
      net_EnsurePSMInit();

    nsWyciwygChannel *wc = new nsWyciwygChannel();
    channel = wc;
    rv = wc->Init(url);
  }

  if (NS_FAILED(rv))
    return rv;

  *result = channel.forget().get();
  return NS_OK;
}

NS_IMETHODIMP
nsWyciwygProtocolHandler::GetProtocolFlags(PRUint32 *result) 
{
  
  

  
  
  
  
  
  *result = URI_NORELATIVE | URI_NOAUTH | URI_DANGEROUS_TO_LOAD |
    URI_INHERITS_SECURITY_CONTEXT;
  return NS_OK;
}
