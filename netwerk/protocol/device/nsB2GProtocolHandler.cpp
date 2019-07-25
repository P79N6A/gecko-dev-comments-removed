




#include "nsB2GProtocolHandler.h"
#include "nsDeviceChannel.h"
#include "nsNetCID.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsSimpleURI.h"
#include "mozilla/Preferences.h"


NS_IMPL_THREADSAFE_ISUPPORTS1(nsB2GProtocolHandler,
                              nsIProtocolHandler)

nsresult
nsB2GProtocolHandler::Init(){
  return NS_OK;
}

NS_IMETHODIMP
nsB2GProtocolHandler::GetScheme(nsACString &aResult)
{
  aResult.AssignLiteral("b2g-camera");
  return NS_OK;
}

NS_IMETHODIMP
nsB2GProtocolHandler::GetDefaultPort(int32_t *aResult)
{
  *aResult = -1;        
  return NS_OK;
}

NS_IMETHODIMP
nsB2GProtocolHandler::GetProtocolFlags(uint32_t *aResult)
{
  *aResult = URI_NORELATIVE | URI_NOAUTH | URI_LOADABLE_BY_ANYONE | URI_IS_LOCAL_RESOURCE;
  return NS_OK;
}

NS_IMETHODIMP
nsB2GProtocolHandler::NewURI(const nsACString &spec,
                                const char *originCharset,
                                nsIURI *baseURI,
                                nsIURI **result)
{
  nsRefPtr<nsSimpleURI> uri = new nsSimpleURI();
  nsresult rv;

  
  
  nsCString key("b2g.camera.");
  key.Append(spec);
  nsCString pref;
  rv = mozilla::Preferences::GetCString(key.get(), &pref);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = uri->SetSpec(pref);
  mozilla::Preferences::ClearUser(key.BeginReading());
  NS_ENSURE_SUCCESS(rv, rv);

  return CallQueryInterface(uri, result);
}

NS_IMETHODIMP
nsB2GProtocolHandler::NewChannel(nsIURI* aURI, nsIChannel **aResult)
{
  nsRefPtr<nsDeviceChannel> channel = new nsDeviceChannel();
  nsresult rv = channel->Init(aURI);
  NS_ENSURE_SUCCESS(rv, rv);

  return CallQueryInterface(channel, aResult);
}

NS_IMETHODIMP 
nsB2GProtocolHandler::AllowPort(int32_t port,
                                   const char *scheme,
                                   bool *aResult)
{
  
  *aResult = false;
  return NS_OK;
}
