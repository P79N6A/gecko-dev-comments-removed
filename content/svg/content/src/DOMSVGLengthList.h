




#ifndef MOZILLA_DOMSVGLENGTHLIST_H__
#define MOZILLA_DOMSVGLENGTHLIST_H__

#include "DOMSVGAnimatedLengthList.h"
#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsDebug.h"
#include "nsIDOMSVGLengthList.h"
#include "nsTArray.h"
#include "SVGLengthList.h"
#include "mozilla/Attributes.h"

class nsIDOMSVGLength;
class nsSVGElement;

namespace mozilla {

class DOMSVGLength;


















class DOMSVGLengthList MOZ_FINAL : public nsIDOMSVGLengthList,
                                   public nsWrapperCache
{
  friend class DOMSVGLength;

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(DOMSVGLengthList)
  NS_DECL_NSIDOMSVGLENGTHLIST

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
  };

  virtual JSObject* WrapObject(JSContext *cx, JSObject *scope,
                               bool *triedToWrap);

  nsISupports* GetParentObject()
  {
    return static_cast<nsIContent*>(Element());
  }

  



  uint32_t Length() const {
    NS_ABORT_IF_FALSE(mItems.Length() == 0 ||
                      mItems.Length() == InternalList().Length(),
                      "DOM wrapper's list length is out of sync");
    return mItems.Length();
  }

  
  void InternalListLengthWillChange(uint32_t aNewLength);

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
