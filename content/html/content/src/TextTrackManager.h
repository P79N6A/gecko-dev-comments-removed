






#ifndef mozilla_dom_TextTrackManager_h
#define mozilla_dom_TextTrackManager_h

#include "mozilla/dom/TextTrack.h"
#include "mozilla/dom/TextTrackList.h"

namespace mozilla {
namespace dom {

class HTMLMediaElement;

class TextTrackManager
{
public:
  NS_DECL_CYCLE_COLLECTION_NATIVE_CLASS(TextTrackManager)
  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(TextTrackManager);

  TextTrackManager(HTMLMediaElement *aMediaElement);
  ~TextTrackManager();

  TextTrackList* TextTracks() const;
  already_AddRefed<TextTrack> AddTextTrack(TextTrackKind aKind,
                                           const nsAString& aLabel,
                                           const nsAString& aLanguage);
  void AddTextTrack(TextTrack* aTextTrack);
  void RemoveTextTrack(TextTrack* aTextTrack);
  void DidSeek();

  
  
  void Update(double aTime);

private:
  
  
  
  
  HTMLMediaElement* mMediaElement;
  
  nsRefPtr<TextTrackList> mTextTracks;
};

} 
} 

#endif 
