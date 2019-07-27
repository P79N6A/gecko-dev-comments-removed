





#include "mozilla/dom/Promise.h"
#include "mozilla/dom/TVManager.h"
#include "mozilla/dom/TVSource.h"
#include "mozilla/dom/TVTuner.h"
#include "nsArrayUtils.h"
#include "TVServiceCallbacks.h"

namespace mozilla {
namespace dom {





NS_IMPL_CYCLE_COLLECTION(TVServiceSourceSetterCallback, mTuner, mPromise)

NS_IMPL_CYCLE_COLLECTING_ADDREF(TVServiceSourceSetterCallback)
NS_IMPL_CYCLE_COLLECTING_RELEASE(TVServiceSourceSetterCallback)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(TVServiceSourceSetterCallback)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(nsITVServiceCallback)
NS_INTERFACE_MAP_END

TVServiceSourceSetterCallback::TVServiceSourceSetterCallback(TVTuner* aTuner,
                                                             Promise* aPromise,
                                                             TVSourceType aSourceType)
  : mTuner(aTuner)
  , mPromise(aPromise)
  , mSourceType(aSourceType)
{
  MOZ_ASSERT(mTuner);
  MOZ_ASSERT(mPromise);
}

TVServiceSourceSetterCallback::~TVServiceSourceSetterCallback()
{
}

 NS_IMETHODIMP
TVServiceSourceSetterCallback::NotifySuccess(nsIArray* aDataList)
{
  
  if (aDataList) {
    mPromise->MaybeReject(NS_ERROR_DOM_ABORT_ERR);
    return NS_ERROR_INVALID_ARG;
  }

  nsresult rv = mTuner->SetCurrentSource(mSourceType);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    mPromise->MaybeReject(rv);
    return rv;
  }

  mPromise->MaybeResolve(JS::UndefinedHandleValue);
  return NS_OK;
}

 NS_IMETHODIMP
TVServiceSourceSetterCallback::NotifyError(uint16_t aErrorCode)
{
  switch (aErrorCode) {
  case nsITVServiceCallback::TV_ERROR_FAILURE:
  case nsITVServiceCallback::TV_ERROR_INVALID_ARG:
    mPromise->MaybeReject(NS_ERROR_DOM_ABORT_ERR);
    return NS_OK;
  case nsITVServiceCallback::TV_ERROR_NO_SIGNAL:
    mPromise->MaybeReject(NS_ERROR_DOM_NETWORK_ERR);
    return NS_OK;
  case nsITVServiceCallback::TV_ERROR_NOT_SUPPORTED:
    mPromise->MaybeReject(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
    return NS_OK;
  }

  mPromise->MaybeReject(NS_ERROR_DOM_ABORT_ERR);
  return NS_ERROR_ILLEGAL_VALUE;
}






NS_IMPL_CYCLE_COLLECTION(TVServiceChannelScanCallback, mSource, mPromise)

NS_IMPL_CYCLE_COLLECTING_ADDREF(TVServiceChannelScanCallback)
NS_IMPL_CYCLE_COLLECTING_RELEASE(TVServiceChannelScanCallback)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(TVServiceChannelScanCallback)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(nsITVServiceCallback)
NS_INTERFACE_MAP_END

TVServiceChannelScanCallback::TVServiceChannelScanCallback(TVSource* aSource,
                                                           Promise* aPromise,
                                                           bool aIsScanning)
  : mSource(aSource)
  , mPromise(aPromise)
  , mIsScanning(aIsScanning)
{
  MOZ_ASSERT(mSource);
  MOZ_ASSERT(mPromise);
}

TVServiceChannelScanCallback::~TVServiceChannelScanCallback()
{
}

 NS_IMETHODIMP
TVServiceChannelScanCallback::NotifySuccess(nsIArray* aDataList)
{
  
  if (aDataList) {
    mPromise->MaybeReject(NS_ERROR_DOM_ABORT_ERR);
    return NS_ERROR_INVALID_ARG;
  }

  mSource->SetIsScanning(mIsScanning);

  mPromise->MaybeResolve(JS::UndefinedHandleValue);
  return NS_OK;
}

 NS_IMETHODIMP
TVServiceChannelScanCallback::NotifyError(uint16_t aErrorCode)
{
  switch (aErrorCode) {
  case nsITVServiceCallback::TV_ERROR_FAILURE:
  case nsITVServiceCallback::TV_ERROR_INVALID_ARG:
    mPromise->MaybeReject(NS_ERROR_DOM_ABORT_ERR);
    return NS_OK;
  case nsITVServiceCallback::TV_ERROR_NO_SIGNAL:
    mPromise->MaybeReject(NS_ERROR_DOM_NETWORK_ERR);
    return NS_OK;
  case nsITVServiceCallback::TV_ERROR_NOT_SUPPORTED:
    mPromise->MaybeReject(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
    return NS_OK;
  }

  mPromise->MaybeReject(NS_ERROR_DOM_ABORT_ERR);
  return NS_ERROR_ILLEGAL_VALUE;
}






NS_IMPL_CYCLE_COLLECTION(TVServiceChannelSetterCallback, mSource, mPromise)

NS_IMPL_CYCLE_COLLECTING_ADDREF(TVServiceChannelSetterCallback)
NS_IMPL_CYCLE_COLLECTING_RELEASE(TVServiceChannelSetterCallback)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(TVServiceChannelSetterCallback)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(nsITVServiceCallback)
NS_INTERFACE_MAP_END

TVServiceChannelSetterCallback::TVServiceChannelSetterCallback(TVSource* aSource,
                                                               Promise* aPromise,
                                                               const nsAString& aChannelNumber)
  : mSource(aSource)
  , mPromise(aPromise)
  , mChannelNumber(aChannelNumber)
{
  MOZ_ASSERT(mSource);
  MOZ_ASSERT(mPromise);
  MOZ_ASSERT(!mChannelNumber.IsEmpty());
}

TVServiceChannelSetterCallback::~TVServiceChannelSetterCallback()
{
}

 NS_IMETHODIMP
TVServiceChannelSetterCallback::NotifySuccess(nsIArray* aDataList)
{
  
  if (!aDataList) {
    mPromise->MaybeReject(NS_ERROR_DOM_ABORT_ERR);
    return NS_ERROR_INVALID_ARG;
  }

  uint32_t length;
  nsresult rv = aDataList->GetLength(&length);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    mPromise->MaybeReject(NS_ERROR_DOM_ABORT_ERR);
    return rv;
  }
  if (length != 1) {
    mPromise->MaybeReject(NS_ERROR_DOM_ABORT_ERR);
    return NS_ERROR_INVALID_ARG;
  }

  nsCOMPtr<nsITVChannelData> channelData = do_QueryElementAt(aDataList, 0);
  if (NS_WARN_IF(!channelData)) {
    mPromise->MaybeReject(NS_ERROR_DOM_ABORT_ERR);
    return rv;
  }

  rv = mSource->SetCurrentChannel(channelData);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    mPromise->MaybeReject(NS_ERROR_DOM_ABORT_ERR);
    return rv;
  }

  mPromise->MaybeResolve(JS::UndefinedHandleValue);
  return NS_OK;
}

 NS_IMETHODIMP
TVServiceChannelSetterCallback::NotifyError(uint16_t aErrorCode)
{
  switch (aErrorCode) {
  case nsITVServiceCallback::TV_ERROR_FAILURE:
  case nsITVServiceCallback::TV_ERROR_INVALID_ARG:
    mPromise->MaybeReject(NS_ERROR_DOM_ABORT_ERR);
    return NS_OK;
  case nsITVServiceCallback::TV_ERROR_NO_SIGNAL:
    mPromise->MaybeReject(NS_ERROR_DOM_NETWORK_ERR);
    return NS_OK;
  case nsITVServiceCallback::TV_ERROR_NOT_SUPPORTED:
    mPromise->MaybeReject(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
    return NS_OK;
  }

  mPromise->MaybeReject(NS_ERROR_DOM_ABORT_ERR);
  return NS_ERROR_ILLEGAL_VALUE;
}






NS_IMPL_CYCLE_COLLECTION(TVServiceTunerGetterCallback, mManager)

NS_IMPL_CYCLE_COLLECTING_ADDREF(TVServiceTunerGetterCallback)
NS_IMPL_CYCLE_COLLECTING_RELEASE(TVServiceTunerGetterCallback)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(TVServiceTunerGetterCallback)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(nsITVServiceCallback)
NS_INTERFACE_MAP_END

TVServiceTunerGetterCallback::TVServiceTunerGetterCallback(TVManager* aManager)
  : mManager(aManager)
{
  MOZ_ASSERT(mManager);
}

TVServiceTunerGetterCallback::~TVServiceTunerGetterCallback()
{
}

 NS_IMETHODIMP
TVServiceTunerGetterCallback::NotifySuccess(nsIArray* aDataList)
{
  if (!aDataList) {
    mManager->RejectPendingGetTunersPromises(NS_ERROR_DOM_ABORT_ERR);
    return NS_ERROR_INVALID_ARG;
  }

  uint32_t length;
  nsresult rv = aDataList->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  nsTArray<nsRefPtr<TVTuner>> tuners(length);
  for (uint32_t i = 0; i < length; i++) {
    nsCOMPtr<nsITVTunerData> tunerData = do_QueryElementAt(aDataList, i);
    if (NS_WARN_IF(!tunerData)) {
      continue;
    }

    nsRefPtr<TVTuner> tuner = TVTuner::Create(mManager->GetOwner(), tunerData);
    NS_ENSURE_TRUE(tuner, NS_ERROR_DOM_ABORT_ERR);

    tuners.AppendElement(tuner);
  }
  mManager->SetTuners(tuners);

  return NS_OK;
}

 NS_IMETHODIMP
TVServiceTunerGetterCallback::NotifyError(uint16_t aErrorCode)
{
  switch (aErrorCode) {
  case nsITVServiceCallback::TV_ERROR_FAILURE:
  case nsITVServiceCallback::TV_ERROR_INVALID_ARG:
    mManager->RejectPendingGetTunersPromises(NS_ERROR_DOM_ABORT_ERR);
    return NS_OK;
  case nsITVServiceCallback::TV_ERROR_NO_SIGNAL:
    mManager->RejectPendingGetTunersPromises(NS_ERROR_DOM_NETWORK_ERR);
    return NS_OK;
  case nsITVServiceCallback::TV_ERROR_NOT_SUPPORTED:
    mManager->RejectPendingGetTunersPromises(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
    return NS_OK;
  }

  mManager->RejectPendingGetTunersPromises(NS_ERROR_DOM_ABORT_ERR);
  return NS_ERROR_ILLEGAL_VALUE;
}






NS_IMPL_CYCLE_COLLECTION(TVServiceChannelGetterCallback, mSource, mPromise)

NS_IMPL_CYCLE_COLLECTING_ADDREF(TVServiceChannelGetterCallback)
NS_IMPL_CYCLE_COLLECTING_RELEASE(TVServiceChannelGetterCallback)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(TVServiceChannelGetterCallback)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(nsITVServiceCallback)
NS_INTERFACE_MAP_END

TVServiceChannelGetterCallback::TVServiceChannelGetterCallback(TVSource* aSource,
                                                               Promise* aPromise)
  : mSource(aSource)
  , mPromise(aPromise)
{
  MOZ_ASSERT(mSource);
  MOZ_ASSERT(mPromise);
}

TVServiceChannelGetterCallback::~TVServiceChannelGetterCallback()
{
}

 NS_IMETHODIMP
TVServiceChannelGetterCallback::NotifySuccess(nsIArray* aDataList)
{
  if (!aDataList) {
    mPromise->MaybeReject(NS_ERROR_DOM_ABORT_ERR);
    return NS_ERROR_INVALID_ARG;
  }

  uint32_t length;
  nsresult rv = aDataList->GetLength(&length);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    mPromise->MaybeReject(NS_ERROR_DOM_ABORT_ERR);
    return rv;
  }

  nsTArray<nsRefPtr<TVChannel>> channels(length);
  for (uint32_t i = 0; i < length; i++) {
    nsCOMPtr<nsITVChannelData> channelData = do_QueryElementAt(aDataList, i);
    if (NS_WARN_IF(!channelData)) {
      mPromise->MaybeReject(NS_ERROR_DOM_ABORT_ERR);
      return NS_ERROR_DOM_ABORT_ERR;
    }

    nsRefPtr<TVChannel> channel = new TVChannel(mSource->GetOwner());
    channels.AppendElement(channel);
  }

  mPromise->MaybeResolve(channels);

  return NS_OK;
}

 NS_IMETHODIMP
TVServiceChannelGetterCallback::NotifyError(uint16_t aErrorCode)
{
  switch (aErrorCode) {
  case nsITVServiceCallback::TV_ERROR_FAILURE:
  case nsITVServiceCallback::TV_ERROR_INVALID_ARG:
    mPromise->MaybeReject(NS_ERROR_DOM_ABORT_ERR);
    return NS_OK;
  case nsITVServiceCallback::TV_ERROR_NO_SIGNAL:
    mPromise->MaybeReject(NS_ERROR_DOM_NETWORK_ERR);
    return NS_OK;
  case nsITVServiceCallback::TV_ERROR_NOT_SUPPORTED:
    mPromise->MaybeReject(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
    return NS_OK;
  }

  mPromise->MaybeReject(NS_ERROR_DOM_ABORT_ERR);
  return NS_ERROR_ILLEGAL_VALUE;
}





NS_IMPL_CYCLE_COLLECTION_0(TVServiceProgramGetterCallback)

NS_IMPL_CYCLE_COLLECTING_ADDREF(TVServiceProgramGetterCallback)
NS_IMPL_CYCLE_COLLECTING_RELEASE(TVServiceProgramGetterCallback)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(TVServiceProgramGetterCallback)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(nsITVServiceCallback)
NS_INTERFACE_MAP_END

TVServiceProgramGetterCallback::TVServiceProgramGetterCallback(TVChannel* aChannel,
                                                               Promise* aPromise,
                                                               bool aIsSingular)
  : mChannel(aChannel)
  , mPromise(aPromise)
  , mIsSingular(aIsSingular)
{
  MOZ_ASSERT(mChannel);
  MOZ_ASSERT(mPromise);
}

TVServiceProgramGetterCallback::~TVServiceProgramGetterCallback()
{
}

 NS_IMETHODIMP
TVServiceProgramGetterCallback::NotifySuccess(nsIArray* aDataList)
{
  if (!aDataList) {
    return NS_ERROR_INVALID_ARG;
  }

  

  return NS_OK;
}

 NS_IMETHODIMP
TVServiceProgramGetterCallback::NotifyError(uint16_t aErrorCode)
{
  switch (aErrorCode) {
  case nsITVServiceCallback::TV_ERROR_FAILURE:
  case nsITVServiceCallback::TV_ERROR_INVALID_ARG:
    mPromise->MaybeReject(NS_ERROR_DOM_ABORT_ERR);
    return NS_OK;
  case nsITVServiceCallback::TV_ERROR_NO_SIGNAL:
    mPromise->MaybeReject(NS_ERROR_DOM_NETWORK_ERR);
    return NS_OK;
  case nsITVServiceCallback::TV_ERROR_NOT_SUPPORTED:
    mPromise->MaybeReject(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
    return NS_OK;
  }

  mPromise->MaybeReject(NS_ERROR_DOM_ABORT_ERR);
  return NS_ERROR_ILLEGAL_VALUE;
}

} 
} 
