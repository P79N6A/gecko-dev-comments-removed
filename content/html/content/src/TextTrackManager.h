






#ifndef mozilla_dom_TextTrackManager_h
#define mozilla_dom_TextTrackManager_h

#include "mozilla/dom/TextTrack.h"
#include "mozilla/dom/TextTrackList.h"
#include "mozilla/dom/TextTrackCueList.h"
#include "mozilla/StaticPtr.h"

class nsIWebVTTParserWrapper;

namespace mozilla {
namespace dom {

class HTMLMediaElement;
class TextTrack;
class TextTrackCue;

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
  void RemoveTextTrack(TextTrack* aTextTrack, bool aPendingListOnly);
  void DidSeek();

  void AddCue(TextTrackCue& aCue);
  void AddCues(TextTrack* aTextTrack);

  























  



  void UpdateCueDisplay();

  void PopulatePendingList();

  
  
  
  
  HTMLMediaElement* mMediaElement;
private:
  
  nsRefPtr<TextTrackList> mTextTracks;
  
  nsRefPtr<TextTrackList> mPendingTextTracks;
  
  nsRefPtr<TextTrackCueList> mNewCues;

  static StaticRefPtr<nsIWebVTTParserWrapper> sParserWrapper;
};

} 
} 

#endif 
