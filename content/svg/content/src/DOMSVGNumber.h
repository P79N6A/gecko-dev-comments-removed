



































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

#define MOZ_SVG_LIST_INDEX_BIT_COUNT 27 // supports > 134 million list items

namespace mozilla {














class DOMSVGNumber : public nsIDOMSVGNumber
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(MOZILLA_DOMSVGNUMBER_IID)
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(DOMSVGNumber)
  NS_DECL_NSIDOMSVGNUMBER

  


  DOMSVGNumber(DOMSVGNumberList *aList,
               PRUint8 aAttrEnum,
               PRUint32 aListIndex,
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

  bool IsInList() const {
    return !!mList;
  }

  



  bool HasOwner() const {
    return !!mList;
  }

  








  void InsertingIntoList(DOMSVGNumberList *aList,
                         PRUint8 aAttrEnum,
                         PRUint32 aListIndex,
                         PRUint8 aIsAnimValItem);

  static PRUint32 MaxListIndex() {
    return (1U << MOZ_SVG_LIST_INDEX_BIT_COUNT) - 1;
  }

  
  void UpdateListIndex(PRUint32 aListIndex) {
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
  bool IndexIsValid();
#endif

  nsRefPtr<DOMSVGNumberList> mList;

  
  

  PRUint32 mListIndex:MOZ_SVG_LIST_INDEX_BIT_COUNT;
  PRUint32 mAttrEnum:4; 
  PRUint32 mIsAnimValItem:1;

  
  float mValue;
};

NS_DEFINE_STATIC_IID_ACCESSOR(DOMSVGNumber, MOZILLA_DOMSVGNUMBER_IID)

} 

#undef MOZ_SVG_LIST_INDEX_BIT_COUNT

#endif 
