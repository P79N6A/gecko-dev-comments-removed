



































#ifndef MOZILLA_DOMSVGNUMBER_H__
#define MOZILLA_DOMSVGNUMBER_H__

#include "nsIDOMSVGNumber.h"
#include "DOMSVGNumberList.h"
#include "nsCycleCollectionParticipant.h"
#include "nsAutoPtr.h"

class nsSVGElement;






#define MOZILLA_DOMSVGNUMBER_IID \
  { 0x2CA92412, 0x2E1F, 0x4DDB, \
    { 0xA1, 0x6C, 0x52, 0xB3, 0xB5, 0x82, 0x27, 0x0D } }

namespace mozilla {














class DOMSVGNumber : public nsIDOMSVGNumber
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(MOZILLA_DOMSVGNUMBER_IID)
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(DOMSVGNumber)
  NS_DECL_NSIDOMSVGNUMBER

  


  DOMSVGNumber(DOMSVGNumberList *aList,
               PRUint32 aAttrEnum,
               PRUint8 aListIndex,
               PRUint8 aIsAnimValItem);

  



  DOMSVGNumber();

  ~DOMSVGNumber() {
    
    
    
    if (mList) {
      mList->mItems[mListIndex] = nsnull;
    }
  };

  


  DOMSVGNumber* Clone() {
    DOMSVGNumber *clone = new DOMSVGNumber();
    clone->mValue = ToSVGNumber();
    return clone;
  }

  PRBool IsInList() const {
    return !!mList;
  }

  



  PRBool HasOwner() const {
    return !!mList;
  }

  








  void InsertingIntoList(DOMSVGNumberList *aList,
                         PRUint32 aAttrEnum,
                         PRUint8 aListIndex,
                         PRUint8 aIsAnimValItem);

  
  void UpdateListIndex(PRUint8 aListIndex) {
    mListIndex = aListIndex;
  }

  





  void RemovingFromList();

  float ToSVGNumber();

private:

  nsSVGElement* Element() {
    return mList->Element();
  }

  PRUint8 AttrEnum() const {
    return mAttrEnum;
  }

  








  float& InternalItem();

#ifdef DEBUG
  PRBool IndexIsValid();
#endif

  nsRefPtr<DOMSVGNumberList> mList;

  
  

  PRUint32 mListIndex:27; 
  PRUint32 mAttrEnum:4; 
  PRUint32 mIsAnimValItem:1;

  
  float mValue;
};

NS_DEFINE_STATIC_IID_ACCESSOR(DOMSVGNumber, MOZILLA_DOMSVGNUMBER_IID)

} 

#endif 
