



































#ifndef MOZILLA_DOMSVGLENGTHLIST_H__
#define MOZILLA_DOMSVGLENGTHLIST_H__

#include "nsIDOMSVGLengthList.h"
#include "SVGLengthList.h"
#include "SVGLength.h"
#include "DOMSVGAnimatedLengthList.h"
#include "nsCOMArray.h"
#include "nsAutoPtr.h"

class nsSVGElement;

namespace mozilla {

class DOMSVGLength;


















class DOMSVGLengthList : public nsIDOMSVGLengthList
{
  friend class DOMSVGLength;

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(DOMSVGLengthList)
  NS_DECL_NSIDOMSVGLENGTHLIST

  DOMSVGLengthList(DOMSVGAnimatedLengthList *aAList,
                   const SVGLengthList &aInternalList)
    : mAList(aAList)
  {
    
    
    
    
    
    InternalListLengthWillChange(aInternalList.Length()); 
  }

  ~DOMSVGLengthList() {
    
    
    
    if (mAList) {
      ( IsAnimValList() ? mAList->mAnimVal : mAList->mBaseVal ) = nsnull;
    }
  };

  



  PRUint32 Length() const {
    NS_ABORT_IF_FALSE(mItems.Length() == 0 ||
                      mItems.Length() ==
                        const_cast<DOMSVGLengthList*>(this)->InternalList().Length(),
                      "DOM wrapper's list length is out of sync");
    return mItems.Length();
  }

  nsIDOMSVGLength* GetItemWithoutAddRef(PRUint32 aIndex);

  
  void InternalListLengthWillChange(PRUint32 aNewLength);

private:

  nsSVGElement* Element() {
    return mAList->mElement;
  }

  PRUint8 AttrEnum() const {
    return mAList->mAttrEnum;
  }

  PRUint8 Axis() const {
    return mAList->mAxis;
  }

  
  PRBool IsAnimValList() const {
    NS_ABORT_IF_FALSE(this == mAList->mBaseVal || this == mAList->mAnimVal,
                      "Calling IsAnimValList() too early?!");
    return this == mAList->mAnimVal;
  }

  







  SVGLengthList& InternalList();

  
  void EnsureItemAt(PRUint32 aIndex);

  void MaybeInsertNullInAnimValListAt(PRUint32 aIndex);
  void MaybeRemoveItemFromAnimValListAt(PRUint32 aIndex);

  
  
  nsTArray<DOMSVGLength*> mItems;

  nsRefPtr<DOMSVGAnimatedLengthList> mAList;
};

} 

#endif 
