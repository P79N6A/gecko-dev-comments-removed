





#ifndef mozilla_dom_MediaTrackList_h
#define mozilla_dom_MediaTrackList_h

#include "mozilla/DOMEventTargetHelper.h"

namespace mozilla {
namespace dom {

class HTMLMediaElement;
class MediaTrack;
class AudioTrackList;
class VideoTrackList;









class MediaTrackList : public DOMEventTargetHelper
{
public:
  MediaTrackList(nsPIDOMWindow* aOwnerWindow, HTMLMediaElement* aMediaElement);
  virtual ~MediaTrackList();

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(MediaTrackList, DOMEventTargetHelper)

  using DOMEventTargetHelper::DispatchTrustedEvent;

  
  
  MediaTrack* operator[](uint32_t aIndex);

  void AddTrack(MediaTrack* aTrack);

  void RemoveTrack(const nsRefPtr<MediaTrack>& aTrack);

  virtual void EmptyTracks();

  void CreateAndDispatchChangeEvent();

  
  MediaTrack* IndexedGetter(uint32_t aIndex, bool& aFound);

  MediaTrack* GetTrackById(const nsAString& aId);

  uint32_t Length() const
  {
    return mTracks.Length();
  }

  IMPL_EVENT_HANDLER(change)
  IMPL_EVENT_HANDLER(addtrack)
  IMPL_EVENT_HANDLER(removetrack)

protected:
  void CreateAndDispatchTrackEventRunner(MediaTrack* aTrack,
                                         const nsAString& aEventName);

  nsTArray<nsRefPtr<MediaTrack>> mTracks;
  nsRefPtr<HTMLMediaElement> mMediaElement;
};

} 
} 

#endif 
