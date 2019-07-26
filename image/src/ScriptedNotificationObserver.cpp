





#include "ScriptedNotificationObserver.h"
#include "imgIScriptedNotificationObserver.h"
#include "nsCycleCollectionParticipant.h"

using namespace mozilla::image;

NS_IMPL_CYCLE_COLLECTION_1(ScriptedNotificationObserver, mInner)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(ScriptedNotificationObserver)
  NS_INTERFACE_MAP_ENTRY(imgINotificationObserver)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(ScriptedNotificationObserver)
NS_IMPL_CYCLE_COLLECTING_RELEASE(ScriptedNotificationObserver)

ScriptedNotificationObserver::ScriptedNotificationObserver(
    imgIScriptedNotificationObserver* aInner)
: mInner(aInner)
{
}

NS_IMETHODIMP
ScriptedNotificationObserver::Notify(imgIRequest* aRequest,
                                     int32_t aType,
                                     const nsIntRect* )
{
  if (aType == imgINotificationObserver::START_REQUEST)
    return mInner->StartRequest(aRequest);
  if (aType == imgINotificationObserver::START_CONTAINER)
    return mInner->StartContainer(aRequest);
  if (aType == imgINotificationObserver::START_FRAME)
    return mInner->StartFrame(aRequest);
  if (aType == imgINotificationObserver::START_DECODE)
    return mInner->StartDecode(aRequest);
  if (aType == imgINotificationObserver::DATA_AVAILABLE)
    return mInner->DataAvailable(aRequest);
  if (aType == imgINotificationObserver::STOP_FRAME)
    return mInner->StopFrame(aRequest);
  if (aType == imgINotificationObserver::STOP_DECODE)
    return mInner->StopDecode(aRequest);
  if (aType == imgINotificationObserver::STOP_REQUEST)
    return mInner->StopRequest(aRequest);
  if (aType == imgINotificationObserver::DISCARD)
    return mInner->Discard(aRequest);
  if (aType == imgINotificationObserver::IS_ANIMATED)
    return mInner->IsAnimated(aRequest);
  if (aType == imgINotificationObserver::FRAME_CHANGED)
    return mInner->FrameChanged(aRequest);
  return NS_OK;
}
