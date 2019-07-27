





#include "RtspChannelChild.h"
#include "RtspChannelParent.h"
#include "RtspHandler.h"
#include "nsILoadGroup.h"
#include "nsIInterfaceRequestor.h"
#include "nsIURI.h"
#include "nsAutoPtr.h"
#include "nsStandardURL.h"
#include "mozilla/net/NeckoChild.h"

namespace mozilla {
namespace net {

NS_IMPL_ISUPPORTS(RtspHandler, nsIProtocolHandler)





NS_IMETHODIMP
RtspHandler::GetScheme(nsACString &aScheme)
{
  aScheme.AssignLiteral("rtsp");
  return NS_OK;
}

NS_IMETHODIMP
RtspHandler::GetDefaultPort(int32_t *aDefaultPort)
{
  *aDefaultPort = kDefaultRtspPort;
  return NS_OK;
}

NS_IMETHODIMP
RtspHandler::GetProtocolFlags(uint32_t *aProtocolFlags)
{
  *aProtocolFlags = URI_NORELATIVE | URI_NOAUTH | URI_LOADABLE_BY_ANYONE |
    URI_NON_PERSISTABLE | URI_SYNC_LOAD_IS_OK;

  return NS_OK;
}

NS_IMETHODIMP
RtspHandler::NewURI(const nsACString & aSpec,
                    const char *aOriginCharset,
                    nsIURI *aBaseURI, nsIURI **aResult)
{
  int32_t port;

  nsresult rv = GetDefaultPort(&port);
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<nsStandardURL> url = new nsStandardURL();
  rv = url->Init(nsIStandardURL::URLTYPE_AUTHORITY, port, aSpec,
                 aOriginCharset, aBaseURI);
  NS_ENSURE_SUCCESS(rv, rv);

  url.forget(aResult);
  return NS_OK;
}

NS_IMETHODIMP
RtspHandler::NewChannel(nsIURI *aURI, nsIChannel **aResult)
{
  bool isRtsp = false;
  nsRefPtr<nsBaseChannel> rtspChannel;

  nsresult rv = aURI->SchemeIs("rtsp", &isRtsp);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(isRtsp, NS_ERROR_UNEXPECTED);

  if (IsNeckoChild()) {
    rtspChannel = new RtspChannelChild(aURI);
  } else {
    rtspChannel = new RtspChannelParent(aURI);
  }

  rv = rtspChannel->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  rtspChannel.forget(aResult);
  return NS_OK;
}

NS_IMETHODIMP
RtspHandler::AllowPort(int32_t port, const char *scheme, bool *aResult)
{
  
  *aResult = false;
  return NS_OK;
}

} 
} 
