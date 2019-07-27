





#include "mozilla/dom/AudioTrack.h"
#include "mozilla/dom/AudioTrackBinding.h"
#include "mozilla/dom/AudioTrackList.h"
#include "mozilla/dom/HTMLMediaElement.h"

namespace mozilla {
namespace dom {

AudioTrack::AudioTrack(const nsAString& aId,
                       const nsAString& aKind,
                       const nsAString& aLabel,
                       const nsAString& aLanguage,
                       bool aEnabled)
  : MediaTrack(aId, aKind, aLabel, aLanguage)
  , mEnabled(aEnabled)
{
}

JSObject*
AudioTrack::WrapObject(JSContext* aCx)
{
  return AudioTrackBinding::Wrap(aCx, this);
}

void
AudioTrack::SetEnabled(bool aEnabled)
{
  SetEnabledInternal(aEnabled, MediaTrack::DEFAULT);
}

void
AudioTrack::SetEnabledInternal(bool aEnabled, int aFlags)
{
  if (aEnabled == mEnabled) {
    return;
  }
  mEnabled = aEnabled;

  
  
  if (!mList) {
    return;
  }

  if (!(aFlags & MediaTrack::FIRE_NO_EVENTS)) {
    mList->CreateAndDispatchChangeEvent();
  }
}

} 
} 
