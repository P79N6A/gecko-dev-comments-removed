




#include "nsDeviceProtocolHandler.h"
#include "nsDeviceChannel.h"
#include "nsAutoPtr.h"
#include "nsSimpleURI.h"



NS_IMPL_ISUPPORTS1(nsDeviceProtocolHandler,
                   nsIProtocolHandler)

nsresult
nsDeviceProtocolHandler::Init(){
  return NS_OK;
}

NS_IMETHODIMP
nsDeviceProtocolHandler::GetScheme(nsACString &aResult)
{
  aResult.AssignLiteral("moz-device");
  return NS_OK;
}

NS_IMETHODIMP
nsDeviceProtocolHandler::GetDefaultPort(int32_t *aResult)
{
  *aResult = -1;        
  return NS_OK;
}

NS_IMETHODIMP
nsDeviceProtocolHandler::GetProtocolFlags(uint32_t *aResult)
{
  *aResult = URI_NORELATIVE | URI_NOAUTH | URI_DANGEROUS_TO_LOAD;
  return NS_OK;
}

NS_IMETHODIMP
nsDeviceProtocolHandler::NewURI(const nsACString &spec,
                                const char *originCharset,
                                nsIURI *baseURI,
                                nsIURI **result)
{
  nsRefPtr<nsSimpleURI> uri = new nsSimpleURI();

  nsresult rv = uri->SetSpec(spec);
  NS_ENSURE_SUCCESS(rv, rv);

  return CallQueryInterface(uri, result);
}

NS_IMETHODIMP
nsDeviceProtocolHandler::NewChannel(nsIURI* aURI, nsIChannel **aResult)
{
  nsRefPtr<nsDeviceChannel> channel = new nsDeviceChannel();
  nsresult rv = channel->Init(aURI);
  NS_ENSURE_SUCCESS(rv, rv);

  return CallQueryInterface(channel, aResult);
}

NS_IMETHODIMP 
nsDeviceProtocolHandler::AllowPort(int32_t port,
                                   const char *scheme,
                                   bool *aResult)
{
  
  *aResult = false;
  return NS_OK;
}
