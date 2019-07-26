




#ifndef MOZILLA_DOMSVGNUMBERLIST_H__
#define MOZILLA_DOMSVGNUMBERLIST_H__

#include "DOMSVGAnimatedNumberList.h"
#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsDebug.h"
#include "nsTArray.h"
#include "SVGNumberList.h"
#include "nsIDOMSVGNumber.h"
#include "mozilla/Attributes.h"
#include "mozilla/ErrorResult.h"

class nsSVGElement;

namespace mozilla {

class DOMSVGNumber;


















class DOMSVGNumberList MOZ_FINAL : public nsISupports,
                                   public nsWrapperCache
{
  friend class DOMSVGNumber;

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(DOMSVGNumberList)

  DOMSVGNumberList(DOMSVGAnimatedNumberList *aAList,
                   const SVGNumberList &aInternalList)
    : mAList(aAList)
  {
    SetIsDOMBinding();

    
    
    
    

    InternalListLengthWillChange(aInternalList.Length()); 
  }

  ~DOMSVGNumberList() {
    
    
    
    if (mAList) {
      ( IsAnimValList() ? mAList->mAnimVal : mAList->mBaseVal ) = nullptr;
    }
  }

  virtual JSObject* WrapObject(JSContext *cx, JSObject *scope) MOZ_OVERRIDE;

  nsISupports* GetParentObject()
  {
    return static_cast<nsIContent*>(Element());
  }

  



  uint32_t LengthNoFlush() const {
    NS_ABORT_IF_FALSE(mItems.Length() == 0 ||
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
  already_AddRefed<nsIDOMSVGNumber> Initialize(nsIDOMSVGNumber *newItem,
                                               ErrorResult& error);
  nsIDOMSVGNumber* GetItem(uint32_t index, ErrorResult& error)
  {
    bool found;
    nsIDOMSVGNumber* item = IndexedGetter(index, found, error);
    if (!found) {
      error.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    }
    return item;
  }
  nsIDOMSVGNumber* IndexedGetter(uint32_t index, bool& found,
                                 ErrorResult& error);
  already_AddRefed<nsIDOMSVGNumber> InsertItemBefore(nsIDOMSVGNumber *newItem,
                                                     uint32_t index,
                                                     ErrorResult& error);
  already_AddRefed<nsIDOMSVGNumber> ReplaceItem(nsIDOMSVGNumber *newItem,
                                                uint32_t index,
                                                ErrorResult& error);
  already_AddRefed<nsIDOMSVGNumber> RemoveItem(uint32_t index,
                                               ErrorResult& error);
  already_AddRefed<nsIDOMSVGNumber> AppendItem(nsIDOMSVGNumber *newItem,
                                               ErrorResult& error)
  {
    return InsertItemBefore(newItem, LengthNoFlush(), error);
  }
  uint32_t Length() const
  {
    return NumberOfItems();
  }

private:

  nsSVGElement* Element() const {
    return mAList->mElement;
  }

  uint8_t AttrEnum() const {
    return mAList->mAttrEnum;
  }

  
  bool IsAnimValList() const {
    NS_ABORT_IF_FALSE(this == mAList->mBaseVal || this == mAList->mAnimVal,
                      "Calling IsAnimValList() too early?!");
    return this == mAList->mAnimVal;
  }

  







  SVGNumberList& InternalList() const;

  
  void EnsureItemAt(uint32_t aIndex);

  void MaybeInsertNullInAnimValListAt(uint32_t aIndex);
  void MaybeRemoveItemFromAnimValListAt(uint32_t aIndex);

  
  
  nsTArray<DOMSVGNumber*> mItems;

  nsRefPtr<DOMSVGAnimatedNumberList> mAList;
};

} 

#endif 
