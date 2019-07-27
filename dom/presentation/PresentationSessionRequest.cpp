




#include "PresentationSessionRequest.h"
#include "nsIPresentationControlChannel.h"
#include "nsIPresentationDevice.h"

namespace mozilla {
namespace dom {

NS_IMPL_ISUPPORTS(PresentationSessionRequest, nsIPresentationSessionRequest)

PresentationSessionRequest::PresentationSessionRequest(nsIPresentationDevice* aDevice,
                                                       const nsAString& aUrl,
                                                       const nsAString& aPresentationId,
                                                       nsIPresentationControlChannel* aControlChannel)
  : mUrl(aUrl)
  , mPresentationId(aPresentationId)
  , mDevice(aDevice)
  , mControlChannel(aControlChannel)
{
}

PresentationSessionRequest::~PresentationSessionRequest()
{
}



NS_IMETHODIMP
PresentationSessionRequest::GetDevice(nsIPresentationDevice** aRetVal)
{
  NS_ENSURE_ARG_POINTER(aRetVal);

  nsCOMPtr<nsIPresentationDevice> device = mDevice;
  device.forget(aRetVal);

  return NS_OK;
}

NS_IMETHODIMP
PresentationSessionRequest::GetUrl(nsAString& aRetVal)
{
  aRetVal = mUrl;

  return NS_OK;
}

NS_IMETHODIMP
PresentationSessionRequest::GetPresentationId(nsAString& aRetVal)
{
  aRetVal = mPresentationId;

  return NS_OK;
}

NS_IMETHODIMP
PresentationSessionRequest::GetControlChannel(nsIPresentationControlChannel** aRetVal)
{
  NS_ENSURE_ARG_POINTER(aRetVal);

  nsCOMPtr<nsIPresentationControlChannel> controlChannel = mControlChannel;
  controlChannel.forget(aRetVal);

  return NS_OK;
}

} 
} 
