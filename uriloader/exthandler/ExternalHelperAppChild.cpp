





#include "ExternalHelperAppChild.h"
#include "mozilla/net/ChannelDiverterChild.h"
#include "nsIDivertableChannel.h"
#include "nsIInputStream.h"
#include "nsIFTPChannel.h"
#include "nsIRequest.h"
#include "nsIResumableChannel.h"
#include "nsNetUtil.h"

namespace mozilla {
namespace dom {

NS_IMPL_ISUPPORTS(ExternalHelperAppChild,
                  nsIStreamListener,
                  nsIRequestObserver)

ExternalHelperAppChild::ExternalHelperAppChild()
  : mStatus(NS_OK)
{
}

ExternalHelperAppChild::~ExternalHelperAppChild()
{
}




NS_IMETHODIMP
ExternalHelperAppChild::OnDataAvailable(nsIRequest *request,
                                        nsISupports *ctx,
                                        nsIInputStream *input,
                                        uint64_t offset,
                                        uint32_t count)
{
  if (NS_FAILED(mStatus))
    return mStatus;

  nsCString data;
  nsresult rv = NS_ReadInputStreamToString(input, data, count);
  if (NS_FAILED(rv))
    return rv;

  if (!SendOnDataAvailable(data, offset, count))
    return NS_ERROR_UNEXPECTED;

  return NS_OK;
}





NS_IMETHODIMP
ExternalHelperAppChild::OnStartRequest(nsIRequest *request, nsISupports *ctx)
{
  nsCOMPtr<nsIDivertableChannel> divertable = do_QueryInterface(request);
  if (divertable) {
    return DivertToParent(divertable, request);
  }

  nsresult rv = mHandler->OnStartRequest(request, ctx);
  NS_ENSURE_SUCCESS(rv, NS_ERROR_UNEXPECTED);

  nsCString entityID;
  nsCOMPtr<nsIResumableChannel> resumable(do_QueryInterface(request));
  if (resumable) {
    resumable->GetEntityID(entityID);
  }
  SendOnStartRequest(entityID);
  return NS_OK;
}

NS_IMETHODIMP
ExternalHelperAppChild::OnStopRequest(nsIRequest *request,
                                      nsISupports *ctx,
                                      nsresult status)
{
  
  if (mHandler) {
    nsresult rv = mHandler->OnStopRequest(request, ctx, status);
    SendOnStopRequest(status);
    NS_ENSURE_SUCCESS(rv, NS_ERROR_UNEXPECTED);
  }

  return NS_OK;
}

nsresult
ExternalHelperAppChild::DivertToParent(nsIDivertableChannel *divertable, nsIRequest *request)
{
  mozilla::net::ChannelDiverterChild *diverter = nullptr;
  nsresult rv = divertable->DivertToParent(&diverter);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  MOZ_ASSERT(diverter);
  if (SendDivertToParentUsing(diverter)) {
    mHandler->DidDivertRequest(request);
    mHandler = nullptr;
    return NS_OK;
  }

  return NS_ERROR_FAILURE;
}

bool
ExternalHelperAppChild::RecvCancel(const nsresult& aStatus)
{
  mStatus = aStatus;
  return true;
}

} 
} 
