





#include "ExternalHelperAppChild.h"
#include "nsIInputStream.h"
#include "nsIRequest.h"
#include "nsIResumableChannel.h"
#include "nsNetUtil.h"

namespace mozilla {
namespace dom {

NS_IMPL_ISUPPORTS2(ExternalHelperAppChild,
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
                                        uint32_t offset,
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
  nsresult rv = mHandler->OnStartRequest(request, ctx);
  NS_ENSURE_SUCCESS(rv, NS_ERROR_UNEXPECTED);

  nsCString entityID;
  nsCOMPtr<nsIResumableChannel> resumable(do_QueryInterface(request));
  if (resumable)
    resumable->GetEntityID(entityID);
  SendOnStartRequest(entityID);
  return NS_OK;
}


NS_IMETHODIMP
ExternalHelperAppChild::OnStopRequest(nsIRequest *request,
                                      nsISupports *ctx,
                                      nsresult status)
{
  nsresult rv = mHandler->OnStopRequest(request, ctx, status);
  SendOnStopRequest(status);

  NS_ENSURE_SUCCESS(rv, NS_ERROR_UNEXPECTED);
  return NS_OK;
}


bool
ExternalHelperAppChild::RecvCancel(const nsresult& aStatus)
{
  mStatus = aStatus;
  return true;
}

} 
} 
