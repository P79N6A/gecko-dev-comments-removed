




#include "mozilla/dom/VideoTrack.h"
#include "mozilla/dom/VideoTrackList.h"
#include "mozilla/dom/VideoTrackListBinding.h"

namespace mozilla {
namespace dom {

JSObject*
VideoTrackList::WrapObject(JSContext* aCx)
{
  return VideoTrackListBinding::Wrap(aCx, this);
}

VideoTrack*
VideoTrackList::operator[](uint32_t aIndex)
{
  MediaTrack* track = MediaTrackList::operator[](aIndex);
  return track->AsVideoTrack();
}

void
VideoTrackList::EmptyTracks()
{
  mSelectedIndex = -1;
  MediaTrackList::EmptyTracks();
}

VideoTrack*
VideoTrackList::IndexedGetter(uint32_t aIndex, bool& aFound)
{
  MediaTrack* track = MediaTrackList::IndexedGetter(aIndex, aFound);
  return track ? track->AsVideoTrack() : nullptr;
}

VideoTrack*
VideoTrackList::GetTrackById(const nsAString& aId)
{
  MediaTrack* track = MediaTrackList::GetTrackById(aId);
  return track ? track->AsVideoTrack() : nullptr;
}

} 
} 
