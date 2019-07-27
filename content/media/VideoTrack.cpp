





#include "mozilla/dom/HTMLMediaElement.h"
#include "mozilla/dom/VideoTrack.h"
#include "mozilla/dom/VideoTrackBinding.h"
#include "mozilla/dom/VideoTrackList.h"

namespace mozilla {
namespace dom {

VideoTrack::VideoTrack(const nsAString& aId,
                       const nsAString& aKind,
                       const nsAString& aLabel,
                       const nsAString& aLanguage)
  : MediaTrack(aId, aKind, aLabel, aLanguage)
  , mSelected(false)
{
}

JSObject*
VideoTrack::WrapObject(JSContext* aCx)
{
  return VideoTrackBinding::Wrap(aCx, this);
}

void VideoTrack::SetSelected(bool aSelected)
{
  SetEnabledInternal(aSelected, MediaTrack::DEFAULT);
}

void
VideoTrack::SetEnabledInternal(bool aEnabled, int aFlags)
{
  if (aEnabled == mSelected) {
    return;
  }

  mSelected = aEnabled;

  
  
  if (!mList) {
    return;
  }

  VideoTrackList& list = static_cast<VideoTrackList&>(*mList);
  if (mSelected) {
    uint32_t curIndex = 0;

    
    for (uint32_t i = 0; i < list.Length(); ++i) {
      if (list[i] == this) {
        curIndex = i;
        continue;
      }

      VideoTrack* track = list[i];
      track->SetSelected(false);
    }

    
    list.mSelectedIndex = curIndex;
  } else {
    list.mSelectedIndex = -1;
  }

  
  
  if (!(aFlags & MediaTrack::FIRE_NO_EVENTS)) {
    list.CreateAndDispatchChangeEvent();

    HTMLMediaElement* element = mList->GetMediaElement();
    if (element) {
      element->NotifyMediaTrackEnabled(this);
    }
  }
}

} 
} 
