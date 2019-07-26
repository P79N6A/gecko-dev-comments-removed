





#ifndef mozilla_dom_TextTrackList_h
#define mozilla_dom_TextTrackList_h

#include "mozilla/dom/TextTrack.h"
#include "nsCycleCollectionParticipant.h"
#include "nsDOMEventTargetHelper.h"

namespace mozilla {
namespace dom {

class TextTrack;

class TextTrackList MOZ_FINAL : public nsDOMEventTargetHelper
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_INHERITED(TextTrackList,
                                                         nsDOMEventTargetHelper)

  TextTrackList(nsISupports* aGlobal);

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

  
  void Update(double aTime);

  TextTrack* IndexedGetter(uint32_t aIndex, bool& aFound);

  already_AddRefed<TextTrack> AddTextTrack(TextTrackKind aKind,
                                           const nsAString& aLabel,
                                           const nsAString& aLanguage);
  void AddTextTrack(TextTrack* aTextTrack) {
    mTextTracks.AppendElement(aTextTrack);
  }

  void RemoveTextTrack(const TextTrack& aTrack);

  IMPL_EVENT_HANDLER(addtrack)
  IMPL_EVENT_HANDLER(removetrack)

private:
  nsCOMPtr<nsISupports> mGlobal;
  nsTArray< nsRefPtr<TextTrack> > mTextTracks;
};

} 
} 

#endif 
