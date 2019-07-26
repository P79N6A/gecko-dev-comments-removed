





#ifndef mozilla_dom_TextTrackCueList_h
#define mozilla_dom_TextTrackCueList_h

#include "nsTArray.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsWrapperCache.h"
#include "mozilla/ErrorResult.h"

namespace mozilla {
namespace dom {

class TextTrackCue;

class TextTrackCueList MOZ_FINAL : public nsISupports
                                 , public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(TextTrackCueList)

  
  TextTrackCueList(nsISupports* aParent);

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  nsISupports* GetParentObject() const
  {
    return mParent;
  }

  uint32_t Length() const
  {
    return mList.Length();
  }

  
  void Update(double aTime);

  TextTrackCue* IndexedGetter(uint32_t aIndex, bool& aFound);
  TextTrackCue* GetCueById(const nsAString& aId);

  void AddCue(TextTrackCue& cue);
  void RemoveCue(TextTrackCue& cue, ErrorResult& aRv);

private:
  nsCOMPtr<nsISupports> mParent;

  nsTArray< nsRefPtr<TextTrackCue> > mList;
};

} 
} 

#endif 
