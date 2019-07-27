





#ifndef mozilla_dom_AudioTrackList_h
#define mozilla_dom_AudioTrackList_h

#include "MediaTrack.h"
#include "MediaTrackList.h"

namespace mozilla {
namespace dom {

class AudioTrack;

class AudioTrackList : public MediaTrackList
{
public:
  AudioTrackList(nsPIDOMWindow* aOwnerWindow, HTMLMediaElement* aMediaElement)
    : MediaTrackList(aOwnerWindow, aMediaElement) {}

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  AudioTrack* operator[](uint32_t aIndex);

  
  AudioTrack* IndexedGetter(uint32_t aIndex, bool& aFound);

  AudioTrack* GetTrackById(const nsAString& aId);

protected:
  virtual AudioTrackList* AsAudioTrackList() MOZ_OVERRIDE { return this; }
};

} 
} 

#endif 
