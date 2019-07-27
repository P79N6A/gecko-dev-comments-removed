





#ifndef MOZILLA_DOMSVGTRANSFORMLIST_H__
#define MOZILLA_DOMSVGTRANSFORMLIST_H__

#include "mozilla/dom/SVGAnimatedTransformList.h"
#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsDebug.h"
#include "nsTArray.h"
#include "SVGTransformList.h"
#include "mozilla/Attributes.h"
#include "mozilla/ErrorResult.h"

class nsSVGElement;

namespace mozilla {

namespace dom {
class SVGMatrix;
class SVGTransform;
}









class DOMSVGTransformList MOZ_FINAL : public nsISupports,
                                      public nsWrapperCache
{
  friend class AutoChangeTransformListNotifier;
  friend class dom::SVGTransform;

  ~DOMSVGTransformList() {
    
    
    
    if (mAList) {
      ( IsAnimValList() ? mAList->mAnimVal : mAList->mBaseVal ) = nullptr;
    }
  }

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(DOMSVGTransformList)

  DOMSVGTransformList(dom::SVGAnimatedTransformList *aAList,
                      const SVGTransformList &aInternalList)
    : mAList(aAList)
  {
    SetIsDOMBinding();

    
    
    
    

    InternalListLengthWillChange(aInternalList.Length()); 
  }

  virtual JSObject* WrapObject(JSContext *cx) MOZ_OVERRIDE;

  nsISupports* GetParentObject()
  {
    return static_cast<nsIContent*>(Element());
  }

  



  uint32_t LengthNoFlush() const {
    NS_ABORT_IF_FALSE(mItems.IsEmpty() ||
      mItems.Length() == InternalList().Length(),
      "DOM wrapper's list length is out of sync");
    return mItems.Length();
  }

  
  void InternalListLengthWillChange(uint32_t aNewLength);

  



  bool IsAnimating() const {
    return mAList->IsAnimating();
  }

  uint32_t NumberOfItems() const
  {
    if (IsAnimValList()) {
      Element()->FlushAnimations();
    }
    return LengthNoFlush();
  }
  void Clear(ErrorResult& error);
  already_AddRefed<dom::SVGTransform> Initialize(dom::SVGTransform& newItem,
                                                 ErrorResult& error);
  already_AddRefed<dom::SVGTransform> GetItem(uint32_t index,
                                              ErrorResult& error);
  already_AddRefed<dom::SVGTransform> IndexedGetter(uint32_t index, bool& found,
                                                    ErrorResult& error);
  already_AddRefed<dom::SVGTransform> InsertItemBefore(dom::SVGTransform& newItem,
                                                       uint32_t index,
                                                       ErrorResult& error);
  already_AddRefed<dom::SVGTransform> ReplaceItem(dom::SVGTransform& newItem,
                                                  uint32_t index,
                                                  ErrorResult& error);
  already_AddRefed<dom::SVGTransform> RemoveItem(uint32_t index,
                                                 ErrorResult& error);
  already_AddRefed<dom::SVGTransform> AppendItem(dom::SVGTransform& newItem,
                                                 ErrorResult& error)
  {
    return InsertItemBefore(newItem, LengthNoFlush(), error);
  }
  already_AddRefed<dom::SVGTransform> CreateSVGTransformFromMatrix(dom::SVGMatrix& matrix);
  already_AddRefed<dom::SVGTransform> Consolidate(ErrorResult& error);
  uint32_t Length() const
  {
    return NumberOfItems();
  }

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

  
  already_AddRefed<dom::SVGTransform> GetItemAt(uint32_t aIndex);

  void MaybeInsertNullInAnimValListAt(uint32_t aIndex);
  void MaybeRemoveItemFromAnimValListAt(uint32_t aIndex);

  
  
  FallibleTArray<dom::SVGTransform*> mItems;

  nsRefPtr<dom::SVGAnimatedTransformList> mAList;
};

} 

#endif 
