




#ifndef MOZILLA_DOMSVGLENGTH_H__
#define MOZILLA_DOMSVGLENGTH_H__

#include "DOMSVGLengthList.h"
#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsDebug.h"
#include "nsIDOMSVGLength.h"
#include "nsTArray.h"
#include "SVGLength.h"
#include "mozilla/Attributes.h"

class nsSVGElement;






#define MOZILLA_DOMSVGLENGTH_IID \
  { 0xA8468350, 0x7F7B, 0x4976, { 0x9A, 0x7E, 0x37, 0x65, 0xA1, 0xDA, 0xDF, 0x9A } }

#define MOZ_SVG_LIST_INDEX_BIT_COUNT 22 // supports > 4 million list items

namespace mozilla {






































class DOMSVGLength MOZ_FINAL : public nsIDOMSVGLength
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(MOZILLA_DOMSVGLENGTH_IID)
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(DOMSVGLength)
  NS_DECL_NSIDOMSVGLENGTH

  


  DOMSVGLength(DOMSVGLengthList *aList,
               uint8_t aAttrEnum,
               uint32_t aListIndex,
               uint8_t aIsAnimValItem);

  



  DOMSVGLength();

  ~DOMSVGLength() {
    
    
    
    if (mList) {
      mList->mItems[mListIndex] = nullptr;
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
                         uint8_t aAttrEnum,
                         uint32_t aListIndex,
                         uint8_t aIsAnimValItem);

  static uint32_t MaxListIndex() {
    return (1U << MOZ_SVG_LIST_INDEX_BIT_COUNT) - 1;
  }

  
  void UpdateListIndex(uint32_t aListIndex) {
    mListIndex = aListIndex;
  }

  





  void RemovingFromList();

  SVGLength ToSVGLength();

private:

  nsSVGElement* Element() {
    return mList->Element();
  }

  uint8_t AttrEnum() const {
    return mAttrEnum;
  }

  



  uint8_t Axis() const {
    return mList->Axis();
  }

  








  SVGLength& InternalItem();

#ifdef DEBUG
  bool IndexIsValid();
#endif

  nsRefPtr<DOMSVGLengthList> mList;

  
  

  uint32_t mListIndex:MOZ_SVG_LIST_INDEX_BIT_COUNT;
  uint32_t mAttrEnum:4; 
  uint32_t mIsAnimValItem:1;

  
  uint32_t mUnit:5; 
  float mValue;
};

NS_DEFINE_STATIC_IID_ACCESSOR(DOMSVGLength, MOZILLA_DOMSVGLENGTH_IID)

} 

#undef MOZ_SVG_LIST_INDEX_BIT_COUNT

#endif 
