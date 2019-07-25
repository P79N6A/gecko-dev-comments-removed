



































#ifndef MOZILLA_DOMSVGNUMBERLIST_H__
#define MOZILLA_DOMSVGNUMBERLIST_H__

#include "nsIDOMSVGNumberList.h"
#include "SVGNumberList.h"
#include "DOMSVGAnimatedNumberList.h"
#include "nsCOMArray.h"
#include "nsAutoPtr.h"

class nsSVGElement;

namespace mozilla {

class DOMSVGNumber;


















class DOMSVGNumberList : public nsIDOMSVGNumberList
{
  friend class DOMSVGNumber;

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(DOMSVGNumberList)
  NS_DECL_NSIDOMSVGNUMBERLIST

  DOMSVGNumberList(DOMSVGAnimatedNumberList *aAList,
                   const SVGNumberList &aInternalList)
    : mAList(aAList)
  {
    
    
    
    

    InternalListLengthWillChange(aInternalList.Length()); 
  }

  ~DOMSVGNumberList() {
    
    
    
    if (mAList) {
      ( IsAnimValList() ? mAList->mAnimVal : mAList->mBaseVal ) = nsnull;
    }
  }

  



  PRUint32 Length() const {
    NS_ABORT_IF_FALSE(mItems.Length() == 0 ||
                      mItems.Length() ==
                        const_cast<DOMSVGNumberList*>(this)->InternalList().Length(),
                      "DOM wrapper's list length is out of sync");
    return mItems.Length();
  }

  nsIDOMSVGNumber* GetItemWithoutAddRef(PRUint32 aIndex);

  
  void InternalListLengthWillChange(PRUint32 aNewLength);

private:

  nsSVGElement* Element() {
    return mAList->mElement;
  }

  PRUint8 AttrEnum() const {
    return mAList->mAttrEnum;
  }

  
  PRBool IsAnimValList() const {
    NS_ABORT_IF_FALSE(this == mAList->mBaseVal || this == mAList->mAnimVal,
                      "Calling IsAnimValList() too early?!");
    return this == mAList->mAnimVal;
  }

  







  SVGNumberList& InternalList();

  
  void EnsureItemAt(PRUint32 aIndex);

  void MaybeInsertNullInAnimValListAt(PRUint32 aIndex);
  void MaybeRemoveItemFromAnimValListAt(PRUint32 aIndex);

  
  
  nsTArray<DOMSVGNumber*> mItems;

  nsRefPtr<DOMSVGAnimatedNumberList> mAList;
};

} 

#endif 
