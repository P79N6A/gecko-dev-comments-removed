





#ifndef mozilla_dom_VideoPlaybackQuality_h_
#define mozilla_dom_VideoPlaybackQuality_h_

#include "mozilla/dom/HTMLMediaElement.h"
#include "nsCycleCollectionParticipant.h"
#include "nsDOMNavigationTiming.h"
#include "nsWrapperCache.h"

namespace mozilla {
namespace dom {

class VideoPlaybackQuality MOZ_FINAL : public nsWrapperCache
{
public:
  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(VideoPlaybackQuality)
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(VideoPlaybackQuality)

  VideoPlaybackQuality(HTMLMediaElement* aElement, DOMHighResTimeStamp aCreationTime,
                       uint64_t aTotalFrames, uint64_t aDroppedFrames,
                       uint64_t aCorruptedFrames, double aPlaybackJitter);

  HTMLMediaElement* GetParentObject() const;

  JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  DOMHighResTimeStamp CreationTime() const
  {
    return mCreationTime;
  }

  uint64_t TotalVideoFrames()
  {
    return mTotalFrames;
  }

  uint64_t DroppedVideoFrames()
  {
    return mDroppedFrames;
  }

  uint64_t CorruptedVideoFrames()
  {
    return mCorruptedFrames;
  }

  double PlaybackJitter()
  {
    return mPlaybackJitter;
  }

private:
  nsRefPtr<HTMLMediaElement> mElement;
  DOMHighResTimeStamp mCreationTime;
  uint64_t mTotalFrames;
  uint64_t mDroppedFrames;
  uint64_t mCorruptedFrames;
  double mPlaybackJitter;
};

} 
} 
#endif 
