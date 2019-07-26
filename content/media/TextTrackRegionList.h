





#ifndef mozilla_dom_TextTrackRegionList_h
#define mozilla_dom_TextTrackRegionList_h

#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsTArray.h"
#include "nsWrapperCache.h"

namespace mozilla {
namespace dom {

class TextTrackRegion;

class TextTrackRegionList MOZ_FINAL : public nsISupports,
                                      public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(TextTrackRegionList)

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  TextTrackRegionList(nsISupports* aGlobal);

  nsISupports* GetParentObject() const
  {
    return mParent;
  }

  
  uint32_t Length() const
  {
    return mTextTrackRegions.Length();
  }

  TextTrackRegion* IndexedGetter(uint32_t aIndex, bool& aFound);

  TextTrackRegion* GetRegionById(const nsAString& aId);

  

  void AddTextTrackRegion(TextTrackRegion* aRegion);

  void RemoveTextTrackRegion(const TextTrackRegion& aRegion);

private:
  nsCOMPtr<nsISupports> mParent;
  nsTArray<nsRefPtr<TextTrackRegion> > mTextTrackRegions;
};

} 
} 

#endif 
