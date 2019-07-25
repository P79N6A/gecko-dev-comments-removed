



































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

  DOMSVGLengthList(DOMSVGAnimatedLengthList *aAList)
    : mAList(aAList)
  {
    
    

    mItems.SetLength(InternalList().Length());
    for (PRUint32 i = 0; i < Length(); ++i) {
      
      mItems[i] = nsnull;
    }
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
    return this == mAList->mAnimVal;
  }

  







  SVGLengthList& InternalList();

  
  void EnsureItemAt(PRUint32 aIndex);

  
  
  nsTArray<DOMSVGLength*> mItems;

  nsRefPtr<DOMSVGAnimatedLengthList> mAList;
};

} 

#endif 
