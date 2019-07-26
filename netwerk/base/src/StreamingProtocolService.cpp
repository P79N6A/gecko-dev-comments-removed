



#include "base/basictypes.h"
#include "mozilla/ClearOnShutdown.h"
#include "StreamingProtocolService.h"
#include "mozilla/net/NeckoChild.h"
#include "nsIURI.h"

#ifdef MOZ_RTSP
#include "RtspControllerChild.h"
#include "RtspController.h"
#endif

namespace mozilla {
namespace net {

NS_IMPL_ISUPPORTS1(StreamingProtocolControllerService,
                   nsIStreamingProtocolControllerService)


StaticRefPtr<StreamingProtocolControllerService> sSingleton;


already_AddRefed<StreamingProtocolControllerService>
StreamingProtocolControllerService::GetInstance()
{
  if (!sSingleton) {
    sSingleton = new StreamingProtocolControllerService();
    ClearOnShutdown(&sSingleton);
  }
  nsRefPtr<StreamingProtocolControllerService> service = sSingleton.get();
  return service.forget();
}

NS_IMETHODIMP
StreamingProtocolControllerService::Create(nsIChannel *aChannel, nsIStreamingProtocolController **aResult)
{
  nsRefPtr<nsIStreamingProtocolController> mediacontroller;
  nsCOMPtr<nsIURI> uri;
  nsAutoCString scheme;

  NS_ENSURE_ARG_POINTER(aChannel);
  aChannel->GetURI(getter_AddRefs(uri));

  nsresult rv = uri->GetScheme(scheme);
  if (NS_FAILED(rv)) return rv;

#ifdef MOZ_RTSP
  if (scheme.EqualsLiteral("rtsp")) {
    if (IsNeckoChild()) {
      mediacontroller = new RtspControllerChild(aChannel);
    } else {
      mediacontroller = new RtspController(aChannel);
    }
  }
#endif

  if (!mediacontroller) {
    return NS_ERROR_NO_INTERFACE;
  }

  mediacontroller->Init(uri);

  mediacontroller.forget(aResult);
  return NS_OK;
}

} 
} 
