



































#ifndef MOZILLA_DOMSVGLENGTH_H__
#define MOZILLA_DOMSVGLENGTH_H__

#include "nsIDOMSVGLength.h"
#include "DOMSVGLengthList.h"
#include "SVGLength.h"
#include "nsCycleCollectionParticipant.h"
#include "nsAutoPtr.h"

class nsSVGElement;






#define MOZILLA_DOMSVGLENGTH_IID \
  { 0xA8468350, 0x7F7B, 0x4976, { 0x9A, 0x7E, 0x37, 0x65, 0xA1, 0xDA, 0xDF, 0x9A } }

#define MOZ_SVG_LIST_INDEX_BIT_COUNT 22 // supports > 4 million list items

namespace mozilla {






































class DOMSVGLength : public nsIDOMSVGLength
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(MOZILLA_DOMSVGLENGTH_IID)
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(DOMSVGLength)
  NS_DECL_NSIDOMSVGLENGTH

  


  DOMSVGLength(DOMSVGLengthList *aList,
               PRUint8 aAttrEnum,
               PRUint32 aListIndex,
               PRUint8 aIsAnimValItem);

  



  DOMSVGLength();

  ~DOMSVGLength() {
    
    
    
    if (mList) {
      mList->mItems[mListIndex] = nsnull;
    }
  };

  



  DOMSVGLength* Copy() {
    NS_ASSERTION(mList, "unexpected caller");
    DOMSVGLength *copy = new DOMSVGLength();
    SVGLength &length = InternalItem();
    copy->NewValueSpecifiedUnits(length.GetUnit(), length.GetValueInCurrentUnits());
    return copy;
  }

  bool IsInList() const {
    return !!mList;
  }

  



  bool HasOwner() const {
    return !!mList;
  }

  








  void InsertingIntoList(DOMSVGLengthList *aList,
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

  SVGLength ToSVGLength();

private:

  nsSVGElement* Element() {
    return mList->Element();
  }

  PRUint8 AttrEnum() const {
    return mAttrEnum;
  }

  



  PRUint8 Axis() const {
    return mList->Axis();
  }

  








  SVGLength& InternalItem();

#ifdef DEBUG
  bool IndexIsValid();
#endif

  nsRefPtr<DOMSVGLengthList> mList;

  
  

  PRUint32 mListIndex:MOZ_SVG_LIST_INDEX_BIT_COUNT;
  PRUint32 mAttrEnum:4; 
  PRUint32 mIsAnimValItem:1;

  
  PRUint32 mUnit:5; 
  float mValue;
};

NS_DEFINE_STATIC_IID_ACCESSOR(DOMSVGLength, MOZILLA_DOMSVGLENGTH_IID)

} 

#undef MOZ_SVG_LIST_INDEX_BIT_COUNT

#endif 
