





#ifndef mozilla_dom_MediaTrackList_h
#define mozilla_dom_MediaTrackList_h

#include "mozilla/DOMEventTargetHelper.h"

namespace mozilla {
namespace dom {

class HTMLMediaElement;
class MediaTrack;
class AudioTrackList;
class VideoTrackList;
class AudioTrack;
class VideoTrack;
class MediaTrackList;






class MediaTrackListListener
{
public:
  explicit MediaTrackListListener(MediaTrackList* aMediaTrackList)
    : mMediaTrackList(aMediaTrackList) {};

  ~MediaTrackListListener()
  {
    mMediaTrackList = nullptr;
  };

  
  
  
  void NotifyMediaTrackCreated(MediaTrack* aTrack);

  
  
  
  void NotifyMediaTrackEnded(const nsAString& aId);

protected:
  
  
  MediaTrackList* mMediaTrackList;
};









class MediaTrackList : public DOMEventTargetHelper
{
public:
  MediaTrackList(nsPIDOMWindow* aOwnerWindow, HTMLMediaElement* aMediaElement);

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(MediaTrackList, DOMEventTargetHelper)

  using DOMEventTargetHelper::DispatchTrustedEvent;

  
  
  MediaTrack* operator[](uint32_t aIndex);

  void AddTrack(MediaTrack* aTrack);

  void RemoveTrack(const nsRefPtr<MediaTrack>& aTrack);

  void RemoveTracks();

  static already_AddRefed<AudioTrack>
  CreateAudioTrack(const nsAString& aId,
                   const nsAString& aKind,
                   const nsAString& aLabel,
                   const nsAString& aLanguage,
                   bool aEnabled);

  static already_AddRefed<VideoTrack>
  CreateVideoTrack(const nsAString& aId,
                   const nsAString& aKind,
                   const nsAString& aLabel,
                   const nsAString& aLanguage);

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

  friend class MediaTrackListListener;
  friend class AudioTrack;
  friend class VideoTrack;

protected:
  virtual ~MediaTrackList();

  void CreateAndDispatchTrackEventRunner(MediaTrack* aTrack,
                                         const nsAString& aEventName);

  virtual AudioTrackList* AsAudioTrackList() { return nullptr; }

  virtual VideoTrackList* AsVideoTrackList() { return nullptr; }

  HTMLMediaElement* GetMediaElement() { return mMediaElement; }

  nsTArray<nsRefPtr<MediaTrack>> mTracks;
  nsRefPtr<HTMLMediaElement> mMediaElement;
};

} 
} 

#endif 
