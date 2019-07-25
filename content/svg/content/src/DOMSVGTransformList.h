





#ifndef MOZILLA_DOMSVGTRANSFORMLIST_H__
#define MOZILLA_DOMSVGTRANSFORMLIST_H__

#include "DOMSVGAnimatedTransformList.h"
#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsDebug.h"
#include "nsIDOMSVGTransformList.h"
#include "nsTArray.h"
#include "SVGTransformList.h"
#include "mozilla/Attributes.h"

class nsIDOMSVGTransform;
class nsSVGElement;

namespace mozilla {

class DOMSVGTransform;









class DOMSVGTransformList MOZ_FINAL : public nsIDOMSVGTransformList,
                                      public nsWrapperCache
{
  friend class DOMSVGTransform;

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(DOMSVGTransformList)
  NS_DECL_NSIDOMSVGTRANSFORMLIST

  DOMSVGTransformList(DOMSVGAnimatedTransformList *aAList,
                      const SVGTransformList &aInternalList)
    : mAList(aAList)
  {
    SetIsDOMBinding();

    
    
    
    

    InternalListLengthWillChange(aInternalList.Length()); 
  }

  ~DOMSVGTransformList() {
    
    
    
    if (mAList) {
      ( IsAnimValList() ? mAList->mAnimVal : mAList->mBaseVal ) = nullptr;
    }
  }

  virtual JSObject* WrapObject(JSContext *cx, JSObject *scope,
                               bool *triedToWrap);

  nsISupports* GetParentObject()
  {
    return static_cast<nsIContent*>(Element());
  }

  



  uint32_t Length() const {
    NS_ABORT_IF_FALSE(mItems.IsEmpty() ||
      mItems.Length() == InternalList().Length(),
      "DOM wrapper's list length is out of sync");
    return mItems.Length();
  }

  
  void InternalListLengthWillChange(uint32_t aNewLength);

private:

  nsSVGElement* Element() const {
    return mAList->mElement;
  }

  
  bool IsAnimValList() const {
    NS_ABORT_IF_FALSE(this == mAList->mBaseVal || this == mAList->mAnimVal,
                      "Calling IsAnimValList() too early?!");
    return this == mAList->mAnimVal;
  }

  







  SVGTransformList& InternalList() const;

  
  void EnsureItemAt(uint32_t aIndex);

  void MaybeInsertNullInAnimValListAt(uint32_t aIndex);
  void MaybeRemoveItemFromAnimValListAt(uint32_t aIndex);

  
  
  nsTArray<DOMSVGTransform*> mItems;

  nsRefPtr<DOMSVGAnimatedTransformList> mAList;
};

} 

#endif 
