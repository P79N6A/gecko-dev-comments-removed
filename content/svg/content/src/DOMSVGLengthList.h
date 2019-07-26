




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

class nsSVGElement;

namespace mozilla {

class DOMSVGLength;


















class DOMSVGLengthList MOZ_FINAL : public nsISupports,
                                   public nsWrapperCache
{
  friend class AutoChangeLengthListNotifier;
  friend class DOMSVGLength;

  ~DOMSVGLengthList() {
    
    
    
    if (mAList) {
      ( IsAnimValList() ? mAList->mAnimVal : mAList->mBaseVal ) = nullptr;
    }
  }

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

  virtual JSObject* WrapObject(JSContext *cx) MOZ_OVERRIDE;

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
  void Clear(ErrorResult& aError);
  already_AddRefed<DOMSVGLength> Initialize(DOMSVGLength& newItem,
                                            ErrorResult& error);
  already_AddRefed<DOMSVGLength> GetItem(uint32_t index,
                                         ErrorResult& error);
  already_AddRefed<DOMSVGLength> IndexedGetter(uint32_t index, bool& found,
                                               ErrorResult& error);
  already_AddRefed<DOMSVGLength> InsertItemBefore(DOMSVGLength& newItem,
                                                  uint32_t index,
                                                  ErrorResult& error);
  already_AddRefed<DOMSVGLength> ReplaceItem(DOMSVGLength& newItem,
                                             uint32_t index,
                                             ErrorResult& error);
  already_AddRefed<DOMSVGLength> RemoveItem(uint32_t index,
                                            ErrorResult& error);
  already_AddRefed<DOMSVGLength> AppendItem(DOMSVGLength& newItem,
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

  
  already_AddRefed<DOMSVGLength> GetItemAt(uint32_t aIndex);

  void MaybeInsertNullInAnimValListAt(uint32_t aIndex);
  void MaybeRemoveItemFromAnimValListAt(uint32_t aIndex);

  
  
  FallibleTArray<DOMSVGLength*> mItems;

  nsRefPtr<DOMSVGAnimatedLengthList> mAList;
};

} 

#endif 
