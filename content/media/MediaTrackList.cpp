





#include "MediaTrack.h"
#include "MediaTrackList.h"
#include "mozilla/AsyncEventDispatcher.h"
#include "mozilla/dom/AudioTrack.h"
#include "mozilla/dom/VideoTrack.h"
#include "mozilla/dom/TrackEvent.h"
#include "nsThreadUtils.h"

namespace mozilla {
namespace dom {

MediaTrackList::MediaTrackList(nsPIDOMWindow* aOwnerWindow,
                               HTMLMediaElement* aMediaElement)
  : DOMEventTargetHelper(aOwnerWindow)
  , mMediaElement(aMediaElement)
{
}

MediaTrackList::~MediaTrackList()
{
}

NS_IMPL_CYCLE_COLLECTION_INHERITED(MediaTrackList,
                                   DOMEventTargetHelper,
                                   mTracks,
                                   mMediaElement)

NS_IMPL_ADDREF_INHERITED(MediaTrackList, DOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(MediaTrackList, DOMEventTargetHelper)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(MediaTrackList)
NS_INTERFACE_MAP_END_INHERITING(DOMEventTargetHelper)

MediaTrack*
MediaTrackList::operator[](uint32_t aIndex)
{
  return mTracks.ElementAt(aIndex);
}

MediaTrack*
MediaTrackList::IndexedGetter(uint32_t aIndex, bool& aFound)
{
  aFound = aIndex < mTracks.Length();
  return aFound ? mTracks[aIndex] : nullptr;
}

MediaTrack*
MediaTrackList::GetTrackById(const nsAString& aId)
{
  for (uint32_t i = 0; i < mTracks.Length(); ++i) {
    if (aId.Equals(mTracks[i]->GetId())) {
      return mTracks[i];
    }
  }
  return nullptr;
}

void
MediaTrackList::AddTrack(MediaTrack* aTrack)
{
  mTracks.AppendElement(aTrack);
  aTrack->Init(GetOwner());
  aTrack->SetTrackList(this);
  CreateAndDispatchTrackEventRunner(aTrack, NS_LITERAL_STRING("addtrack"));
}

void
MediaTrackList::RemoveTrack(const nsRefPtr<MediaTrack>& aTrack)
{
  mTracks.RemoveElement(aTrack);
  aTrack->SetTrackList(nullptr);
  CreateAndDispatchTrackEventRunner(aTrack, NS_LITERAL_STRING("removetrack"));
}

void
MediaTrackList::EmptyTracks()
{
  for (uint32_t i = 0; i < mTracks.Length(); ++i) {
    mTracks[i]->SetTrackList(nullptr);
  }
  mTracks.Clear();
}

void
MediaTrackList::CreateAndDispatchChangeEvent()
{
  nsRefPtr<AsyncEventDispatcher> asyncDispatcher =
    new AsyncEventDispatcher(this, NS_LITERAL_STRING("change"), false);
  asyncDispatcher->PostDOMEvent();
}

void
MediaTrackList::CreateAndDispatchTrackEventRunner(MediaTrack* aTrack,
                                                  const nsAString& aEventName)
{
  TrackEventInit eventInit;

  if (aTrack->AsAudioTrack()) {
    eventInit.mTrack.SetValue().SetAsAudioTrack() = aTrack->AsAudioTrack();
  } else if (aTrack->AsVideoTrack()) {
    eventInit.mTrack.SetValue().SetAsVideoTrack() = aTrack->AsVideoTrack();
  }

  nsRefPtr<TrackEvent> event =
    TrackEvent::Constructor(this, aEventName, eventInit);

  nsRefPtr<AsyncEventDispatcher> asyncDispatcher =
    new AsyncEventDispatcher(this, event);
  asyncDispatcher->PostDOMEvent();
}

} 
} 
