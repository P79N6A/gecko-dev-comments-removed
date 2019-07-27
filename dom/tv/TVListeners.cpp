





#include "mozilla/dom/TVSource.h"
#include "mozilla/dom/TVTuner.h"
#include "mozilla/dom/TVUtils.h"
#include "TVListeners.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION(TVSourceListener, mSources)

NS_IMPL_CYCLE_COLLECTING_ADDREF(TVSourceListener)
NS_IMPL_CYCLE_COLLECTING_RELEASE(TVSourceListener)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(TVSourceListener)
  NS_INTERFACE_MAP_ENTRY(nsITVSourceListener)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

void
TVSourceListener::RegisterSource(TVSource* aSource)
{
  mSources.AppendElement(aSource);
}

void
TVSourceListener::UnregisterSource(TVSource* aSource)
{
  for (uint32_t i = 0; i < mSources.Length(); i++) {
    if (mSources[i] == aSource) {
      mSources.RemoveElementsAt(i, 1);
    }
  }
}

 NS_IMETHODIMP
TVSourceListener::NotifyChannelScanned(const nsAString& aTunerId,
                                       const nsAString& aSourceType,
                                       nsITVChannelData* aChannelData)
{
  nsRefPtr<TVSource> source = GetSource(aTunerId, aSourceType);
  source->NotifyChannelScanned(aChannelData);
  return NS_OK;
}

 NS_IMETHODIMP
TVSourceListener::NotifyChannelScanComplete(const nsAString& aTunerId,
                                            const nsAString& aSourceType)
{
  nsRefPtr<TVSource> source = GetSource(aTunerId, aSourceType);
  source->NotifyChannelScanComplete();
  return NS_OK;
}

 NS_IMETHODIMP
TVSourceListener::NotifyChannelScanStopped(const nsAString& aTunerId,
                                           const nsAString& aSourceType)
{
  nsRefPtr<TVSource> source = GetSource(aTunerId, aSourceType);
  source->NotifyChannelScanStopped();
  return NS_OK;
}

 NS_IMETHODIMP
TVSourceListener::NotifyEITBroadcasted(const nsAString& aTunerId,
                                       const nsAString& aSourceType,
                                       nsITVChannelData* aChannelData,
                                       nsITVProgramData** aProgramDataList,
                                       const uint32_t aCount)
{
  nsRefPtr<TVSource> source = GetSource(aTunerId, aSourceType);
  source->NotifyEITBroadcasted(aChannelData, aProgramDataList, aCount);
  return NS_OK;
}

already_AddRefed<TVSource>
TVSourceListener::GetSource(const nsAString& aTunerId,
                            const nsAString& aSourceType)
{
  for (uint32_t i = 0; i < mSources.Length(); i++) {
    nsString tunerId;
    nsRefPtr<TVTuner> tuner = mSources[i]->Tuner();
    tuner->GetId(tunerId);

    nsString sourceType = ToTVSourceTypeStr(mSources[i]->Type());

    if (aTunerId.Equals(tunerId) && aSourceType.Equals(sourceType)) {
      nsRefPtr<TVSource> source = mSources[i];
      return source.forget();
    }
  }

  return nullptr;
}

} 
} 
