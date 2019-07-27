





#ifndef mozilla_dom_VideoTrackList_h
#define mozilla_dom_VideoTrackList_h

#include "MediaTrack.h"
#include "MediaTrackList.h"

namespace mozilla {
namespace dom {

class VideoTrack;

class VideoTrackList : public MediaTrackList
{
public:
  VideoTrackList(nsPIDOMWindow* aOwnerWindow, HTMLMediaElement* aMediaElement)
    : MediaTrackList(aOwnerWindow, aMediaElement)
    , mSelectedIndex(-1)
  {}

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  VideoTrack* operator[](uint32_t aIndex);

  virtual void EmptyTracks() MOZ_OVERRIDE;

  
  int32_t SelectedIndex() const
  {
    return mSelectedIndex;
  }

  VideoTrack* IndexedGetter(uint32_t aIndex, bool& aFound);

  VideoTrack* GetTrackById(const nsAString& aId);

  friend class VideoTrack;

protected:
  virtual VideoTrackList* AsVideoTrackList() MOZ_OVERRIDE { return this; }

private:
  int32_t mSelectedIndex;
};

} 
} 

#endif 
