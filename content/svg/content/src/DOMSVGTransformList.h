





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
#include "mozilla/ErrorResult.h"

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

  



  uint32_t LengthNoFlush() const {
    NS_ABORT_IF_FALSE(mItems.IsEmpty() ||
      mItems.Length() == InternalList().Length(),
      "DOM wrapper's list length is out of sync");
    return mItems.Length();
  }

  
  void InternalListLengthWillChange(uint32_t aNewLength);

  uint32_t NumberOfItems() const
  {
    if (IsAnimValList()) {
      Element()->FlushAnimations();
    }
    return LengthNoFlush();
  }
  void Clear(ErrorResult& error);
  already_AddRefed<nsIDOMSVGTransform> Initialize(nsIDOMSVGTransform *newItem,
                                                  ErrorResult& error);
  nsIDOMSVGTransform* GetItem(uint32_t index, ErrorResult& error)
  {
    bool found;
    nsIDOMSVGTransform* item = IndexedGetter(index, found, error);
    if (!found) {
      error.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    }
    return item;
  }
  nsIDOMSVGTransform* IndexedGetter(uint32_t index, bool& found,
                                    ErrorResult& error);
  already_AddRefed<nsIDOMSVGTransform> InsertItemBefore(nsIDOMSVGTransform *newItem,
                                                        uint32_t index,
                                                        ErrorResult& error);
  already_AddRefed<nsIDOMSVGTransform> ReplaceItem(nsIDOMSVGTransform *newItem,
                                                   uint32_t index,
                                                   ErrorResult& error);
  already_AddRefed<nsIDOMSVGTransform> RemoveItem(uint32_t index,
                                                  ErrorResult& error);
  already_AddRefed<nsIDOMSVGTransform> AppendItem(nsIDOMSVGTransform *newItem,
                                                  ErrorResult& error)
  {
    return InsertItemBefore(newItem, LengthNoFlush(), error);
  }
  already_AddRefed<nsIDOMSVGTransform>
    CreateSVGTransformFromMatrix(nsIDOMSVGMatrix *matrix, ErrorResult& error);
  already_AddRefed<nsIDOMSVGTransform> Consolidate(ErrorResult& error);
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

  
  void EnsureItemAt(uint32_t aIndex);

  void MaybeInsertNullInAnimValListAt(uint32_t aIndex);
  void MaybeRemoveItemFromAnimValListAt(uint32_t aIndex);

  
  
  nsTArray<DOMSVGTransform*> mItems;

  nsRefPtr<DOMSVGAnimatedTransformList> mAList;
};

} 

#endif 
