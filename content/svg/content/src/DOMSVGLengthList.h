




#ifndef MOZILLA_DOMSVGLENGTHLIST_H__
#define MOZILLA_DOMSVGLENGTHLIST_H__

#include "DOMSVGAnimatedLengthList.h"
#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsDebug.h"
#include "nsTArray.h"
#include "SVGLengthList.h"
#include "mozilla/Attributes.h"
#include "mozilla/ErrorResult.h"

class nsIDOMSVGLength;
class nsSVGElement;

namespace mozilla {

class DOMSVGLength;


















class DOMSVGLengthList MOZ_FINAL : public nsISupports,
                                   public nsWrapperCache
{
  friend class DOMSVGLength;

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(DOMSVGLengthList)

  DOMSVGLengthList(DOMSVGAnimatedLengthList *aAList,
                   const SVGLengthList &aInternalList)
    : mAList(aAList)
  {
    SetIsDOMBinding();

    
    
    
    
    
    InternalListLengthWillChange(aInternalList.Length()); 
  }

  ~DOMSVGLengthList() {
    
    
    
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
  void Clear(ErrorResult& aError);
  already_AddRefed<nsIDOMSVGLength> Initialize(nsIDOMSVGLength *newItem,
                                               ErrorResult& error);
  nsIDOMSVGLength* GetItem(uint32_t index, ErrorResult& error)
  {
    bool found;
    nsIDOMSVGLength* item = IndexedGetter(index, found, error);
    if (!found) {
      error.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    }
    return item;
  }
  nsIDOMSVGLength* IndexedGetter(uint32_t index, bool& found,
                                 ErrorResult& error);
  already_AddRefed<nsIDOMSVGLength> InsertItemBefore(nsIDOMSVGLength *newItem,
                                                     uint32_t index,
                                                     ErrorResult& error);
  already_AddRefed<nsIDOMSVGLength> ReplaceItem(nsIDOMSVGLength *newItem,
                                                uint32_t index,
                                                ErrorResult& error);
  already_AddRefed<nsIDOMSVGLength> RemoveItem(uint32_t index,
                                               ErrorResult& error);
  already_AddRefed<nsIDOMSVGLength> AppendItem(nsIDOMSVGLength *newItem,
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

  uint8_t Axis() const {
    return mAList->mAxis;
  }

  
  bool IsAnimValList() const {
    NS_ABORT_IF_FALSE(this == mAList->mBaseVal || this == mAList->mAnimVal,
                      "Calling IsAnimValList() too early?!");
    return this == mAList->mAnimVal;
  }

  







  SVGLengthList& InternalList() const;

  
  void EnsureItemAt(uint32_t aIndex);

  void MaybeInsertNullInAnimValListAt(uint32_t aIndex);
  void MaybeRemoveItemFromAnimValListAt(uint32_t aIndex);

  
  
  nsTArray<DOMSVGLength*> mItems;

  nsRefPtr<DOMSVGAnimatedLengthList> mAList;
};

} 

#endif 
