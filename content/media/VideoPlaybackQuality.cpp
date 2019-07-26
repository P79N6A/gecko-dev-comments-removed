





#include "VideoPlaybackQuality.h"

#include "mozilla/dom/HTMLMediaElement.h"
#include "mozilla/dom/VideoPlaybackQualityBinding.h"
#include "nsContentUtils.h"
#include "nsCycleCollectionParticipant.h"
#include "nsWrapperCache.h"
#include "MediaDecoder.h"

namespace mozilla {
namespace dom {

VideoPlaybackQuality::VideoPlaybackQuality(HTMLMediaElement* aElement,
                                           DOMHighResTimeStamp aCreationTime,
                                           uint64_t aTotalFrames,
                                           uint64_t aDroppedFrames,
                                           uint64_t aCorruptedFrames,
                                           double aPlaybackJitter)
  : mElement(aElement)
  , mCreationTime(aCreationTime)
  , mTotalFrames(aTotalFrames)
  , mDroppedFrames(aDroppedFrames)
  , mCorruptedFrames(aCorruptedFrames)
  , mPlaybackJitter(aPlaybackJitter)
{
  SetIsDOMBinding();
}

HTMLMediaElement*
VideoPlaybackQuality::GetParentObject() const
{
  return mElement;
}

JSObject*
VideoPlaybackQuality::WrapObject(JSContext *aCx, JS::Handle<JSObject*> aScope)
{
  return VideoPlaybackQualityBinding::Wrap(aCx, aScope, this);
}

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(VideoPlaybackQuality, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(VideoPlaybackQuality, Release)

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_1(VideoPlaybackQuality, mElement)

} 
} 
