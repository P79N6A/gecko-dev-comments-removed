





#ifndef mozilla_dom_TextTrackList_h
#define mozilla_dom_TextTrackList_h

#include "mozilla/dom/TextTrack.h"
#include "nsCycleCollectionParticipant.h"
#include "nsDOMEventTargetHelper.h"

namespace mozilla {
namespace dom {

class HTMLMediaElement;
class TextTrackManager;
class CompareTextTracks;
class TrackEvent;
class TrackEventRunner;

class TextTrackList MOZ_FINAL : public nsDOMEventTargetHelper
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(TextTrackList, nsDOMEventTargetHelper)

  TextTrackList(nsISupports* aGlobal);
  TextTrackList(nsISupports* aGlobal, TextTrackManager* aTextTrackManager);

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  nsISupports* GetParentObject() const
  {
    return mGlobal;
  }

  uint32_t Length() const
  {
    return mTextTracks.Length();
  }

  
  void GetAllActiveCues(nsTArray<nsRefPtr<TextTrackCue> >& aCues);

  TextTrack* IndexedGetter(uint32_t aIndex, bool& aFound);

  already_AddRefed<TextTrack> AddTextTrack(TextTrackKind aKind,
                                           const nsAString& aLabel,
                                           const nsAString& aLanguage,
                                           TextTrackMode aMode,
                                           TextTrackSource aTextTrackSource,
                                           const CompareTextTracks& aCompareTT);
  TextTrack* GetTrackById(const nsAString& aId);

  void AddTextTrack(TextTrack* aTextTrack, const CompareTextTracks& aCompareTT);

  void RemoveTextTrack(TextTrack* aTrack);
  void DidSeek();

  HTMLMediaElement* GetMediaElement();
  void SetTextTrackManager(TextTrackManager* aTextTrackManager);

  nsresult DispatchTrackEvent(nsIDOMEvent* aEvent);
  void CreateAndDispatchChangeEvent();

  IMPL_EVENT_HANDLER(change)
  IMPL_EVENT_HANDLER(addtrack)
  IMPL_EVENT_HANDLER(removetrack)

private:
  nsCOMPtr<nsISupports> mGlobal;
  nsTArray< nsRefPtr<TextTrack> > mTextTracks;
  nsRefPtr<TextTrackManager> mTextTrackManager;

  void CreateAndDispatchTrackEventRunner(TextTrack* aTrack,
                                         const nsAString& aEventName);
};

} 
} 

#endif 
