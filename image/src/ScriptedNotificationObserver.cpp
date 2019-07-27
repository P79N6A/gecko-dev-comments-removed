





#include "ScriptedNotificationObserver.h"
#include "imgIScriptedNotificationObserver.h"
#include "nsCycleCollectionParticipant.h"

namespace mozilla {
namespace image {

NS_IMPL_CYCLE_COLLECTION(ScriptedNotificationObserver, mInner)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(ScriptedNotificationObserver)
  NS_INTERFACE_MAP_ENTRY(imgINotificationObserver)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(ScriptedNotificationObserver)
NS_IMPL_CYCLE_COLLECTING_RELEASE(ScriptedNotificationObserver)

ScriptedNotificationObserver::ScriptedNotificationObserver(
    imgIScriptedNotificationObserver* aInner)
: mInner(aInner)
{ }

NS_IMETHODIMP
ScriptedNotificationObserver::Notify(imgIRequest* aRequest,
                                     int32_t aType,
                                     const nsIntRect* )
{
  if (aType == imgINotificationObserver::SIZE_AVAILABLE) {
    return mInner->SizeAvailable(aRequest);
  }
  if (aType == imgINotificationObserver::FRAME_UPDATE) {
    return mInner->FrameUpdate(aRequest);
  }
  if (aType == imgINotificationObserver::FRAME_COMPLETE) {
    return mInner->FrameComplete(aRequest);
  }
  if (aType == imgINotificationObserver::DECODE_COMPLETE) {
    return mInner->DecodeComplete(aRequest);
  }
  if (aType == imgINotificationObserver::LOAD_COMPLETE) {
    return mInner->LoadComplete(aRequest);
  }
  if (aType == imgINotificationObserver::DISCARD) {
    return mInner->Discard(aRequest);
  }
  if (aType == imgINotificationObserver::IS_ANIMATED) {
    return mInner->IsAnimated(aRequest);
  }
  if (aType == imgINotificationObserver::HAS_TRANSPARENCY) {
    return mInner->HasTransparency(aRequest);
  }
  return NS_OK;
}

} 
} 
