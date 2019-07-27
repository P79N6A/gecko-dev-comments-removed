





#ifndef mozilla_dom_TVServiceRunnables_h
#define mozilla_dom_TVServiceRunnables_h

#include "nsITVService.h"
#include "nsThreadUtils.h"

namespace mozilla {
namespace dom {











class TVServiceNotifyRunnable MOZ_FINAL : public nsRunnable
{
public:
  TVServiceNotifyRunnable(nsITVServiceCallback* aCallback,
                          nsIArray* aDataList,
                          uint16_t aErrorCode = nsITVServiceCallback::TV_ERROR_OK)
    : mCallback(aCallback)
    , mDataList(aDataList)
    , mErrorCode(aErrorCode)
  {}

  NS_IMETHOD Run()
  {
    if (mErrorCode == nsITVServiceCallback::TV_ERROR_OK) {
      return mCallback->NotifySuccess(mDataList);
    } else {
      return mCallback->NotifyError(mErrorCode);
    }
  }

private:
  nsCOMPtr<nsITVServiceCallback> mCallback;
  nsCOMPtr<nsIArray> mDataList;
  uint16_t mErrorCode;
};

} 
} 

#endif 
