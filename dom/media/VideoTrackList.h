





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

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  VideoTrack* operator[](uint32_t aIndex);

  virtual void EmptyTracks() override;

  
  int32_t SelectedIndex() const
  {
    return mSelectedIndex;
  }

  VideoTrack* IndexedGetter(uint32_t aIndex, bool& aFound);

  VideoTrack* GetTrackById(const nsAString& aId);

  friend class VideoTrack;

protected:
  virtual VideoTrackList* AsVideoTrackList() override { return this; }

private:
  int32_t mSelectedIndex;
};

} 
} 

#endif 
