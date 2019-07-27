





#include "VideoPlaybackQuality.h"

#include "mozilla/dom/HTMLMediaElement.h"
#include "mozilla/dom/VideoPlaybackQualityBinding.h"
#include "nsCycleCollectionParticipant.h"
#include "nsWrapperCache.h"

namespace mozilla {
namespace dom {

VideoPlaybackQuality::VideoPlaybackQuality(HTMLMediaElement* aElement,
                                           DOMHighResTimeStamp aCreationTime,
                                           uint64_t aTotalFrames,
                                           uint64_t aDroppedFrames,
                                           uint64_t aCorruptedFrames)
  : mElement(aElement)
  , mCreationTime(aCreationTime)
  , mTotalFrames(aTotalFrames)
  , mDroppedFrames(aDroppedFrames)
  , mCorruptedFrames(aCorruptedFrames)
{
}

HTMLMediaElement*
VideoPlaybackQuality::GetParentObject() const
{
  return mElement;
}

JSObject*
VideoPlaybackQuality::WrapObject(JSContext *aCx)
{
  return VideoPlaybackQualityBinding::Wrap(aCx, this);
}

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(VideoPlaybackQuality, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(VideoPlaybackQuality, Release)

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(VideoPlaybackQuality, mElement)

} 
} 
