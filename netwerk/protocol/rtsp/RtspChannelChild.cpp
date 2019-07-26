





#include "RtspChannelChild.h"
#include "nsIURI.h"
#include "nsAutoPtr.h"
#include "nsStandardURL.h"

namespace mozilla {
namespace net {

NS_IMPL_ISUPPORTS_INHERITED1(RtspChannelChild,
                             nsBaseChannel,
                             nsIChannel)





NS_IMETHODIMP
RtspChannelChild::AsyncOpen(nsIStreamListener *aListener, nsISupports *aContext)
{
  MOZ_ASSERT(aListener);

  nsCOMPtr<nsIURI> uri = nsBaseChannel::URI();
  NS_ENSURE_TRUE(uri, NS_ERROR_ILLEGAL_VALUE);

  nsAutoCString uriSpec;
  uri->GetSpec(uriSpec);

  mListener = aListener;
  mListenerContext = aContext;

  
  
  
  mListener->OnStartRequest(this, aContext);
  return NS_OK;
}

NS_IMETHODIMP
RtspChannelChild::GetContentType(nsACString& aContentType)
{
  aContentType.AssignLiteral("RTSP");
  return NS_OK;
}

NS_IMETHODIMP
RtspChannelChild::Init(nsIURI* aUri)
{
  MOZ_ASSERT(aUri);

  nsBaseChannel::Init();
  nsBaseChannel::SetURI(aUri);
  return NS_OK;
}

} 
} 
