




#ifndef MOZILLA_DOMSVGNUMBER_H__
#define MOZILLA_DOMSVGNUMBER_H__

#include "DOMSVGNumberList.h"
#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsTArray.h"
#include "mozilla/Attributes.h"
#include "mozilla/ErrorResult.h"
#include "nsWrapperCache.h"

class nsSVGElement;

#define MOZ_SVG_LIST_INDEX_BIT_COUNT 27 // supports > 134 million list items

namespace mozilla {














class DOMSVGNumber MOZ_FINAL : public nsISupports
                             , public nsWrapperCache
{
  friend class AutoChangeNumberNotifier;

  ~DOMSVGNumber() {
    
    
    
    if (mList) {
      mList->mItems[mListIndex] = nullptr;
    }
  }

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(DOMSVGNumber)

  


  DOMSVGNumber(DOMSVGNumberList *aList,
               uint8_t aAttrEnum,
               uint32_t aListIndex,
               bool aIsAnimValItem);

  



  explicit DOMSVGNumber(nsISupports* aParent);

  


  DOMSVGNumber* Clone() {
    DOMSVGNumber *clone = new DOMSVGNumber(mParent);
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
                         uint8_t aAttrEnum,
                         uint32_t aListIndex,
                         bool aIsAnimValItem);

  static uint32_t MaxListIndex() {
    return (1U << MOZ_SVG_LIST_INDEX_BIT_COUNT) - 1;
  }

  
  void UpdateListIndex(uint32_t aListIndex) {
    mListIndex = aListIndex;
  }

  





  void RemovingFromList();

  float ToSVGNumber();

  nsISupports* GetParentObject()
  {
    return mParent;
  }

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  static already_AddRefed<DOMSVGNumber>
  Constructor(const dom::GlobalObject& aGlobal, ErrorResult& aRv);

  static already_AddRefed<DOMSVGNumber>
  Constructor(const dom::GlobalObject& aGlobal, float aValue, ErrorResult& aRv);

  float Value();

  void SetValue(float aValue, ErrorResult& aRv);

private:

  nsSVGElement* Element() {
    return mList->Element();
  }

  uint8_t AttrEnum() const {
    return mAttrEnum;
  }

  








  float& InternalItem();

#ifdef DEBUG
  bool IndexIsValid();
#endif

  nsRefPtr<DOMSVGNumberList> mList;
  nsCOMPtr<nsISupports> mParent;

  
  

  uint32_t mListIndex:MOZ_SVG_LIST_INDEX_BIT_COUNT;
  uint32_t mAttrEnum:4; 
  uint32_t mIsAnimValItem:1;

  
  float mValue;
};

} 

#undef MOZ_SVG_LIST_INDEX_BIT_COUNT

#endif 
