





#ifndef MOZILLA_DOMSVGTRANSFORM_H__
#define MOZILLA_DOMSVGTRANSFORM_H__

#include "DOMSVGTransformList.h"
#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsDebug.h"
#include "nsID.h"
#include "nsIDOMSVGTransform.h"
#include "nsTArray.h"
#include "SVGTransform.h"
#include "mozilla/Attributes.h"

class nsSVGElement;

struct gfxMatrix;






#define MOZILLA_DOMSVGTRANSFORM_IID \
  { 0x0A799862, 0x9469, 0x41FE, \
    { 0xB4, 0xCD, 0x20, 0x19, 0xE6, 0x5D, 0x8D, 0xA6 } }

#define MOZ_SVG_LIST_INDEX_BIT_COUNT 31 // supports > 2 billion list items

namespace mozilla {

class DOMSVGMatrix;




class DOMSVGTransform MOZ_FINAL : public nsIDOMSVGTransform
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(MOZILLA_DOMSVGTRANSFORM_IID)
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(DOMSVGTransform)
  NS_DECL_NSIDOMSVGTRANSFORM

  


  DOMSVGTransform(DOMSVGTransformList *aList,
                  uint32_t aListIndex,
                  bool aIsAnimValItem);

  






  DOMSVGTransform();
  DOMSVGTransform(const gfxMatrix &aMatrix);

  


  DOMSVGTransform(const SVGTransform &aMatrix);

  ~DOMSVGTransform() {
    
    
    
    NS_ABORT_IF_FALSE(!mMatrixTearoff, "Matrix tear-off pointer not cleared."
        " Transform being destroyed before matrix?");
    
    
    
    if (mList) {
      mList->mItems[mListIndex] = nullptr;
    }
  };

  



  DOMSVGTransform* Clone() {
    NS_ASSERTION(mList, "unexpected caller");
    return new DOMSVGTransform(InternalItem());
  }

  bool IsInList() const {
    return !!mList;
  }

  



  bool HasOwner() const {
    return !!mList;
  }

  









  void InsertingIntoList(DOMSVGTransformList *aList,
                         uint32_t aListIndex,
                         bool aIsAnimValItem);

  static uint32_t MaxListIndex() {
    return (1U << MOZ_SVG_LIST_INDEX_BIT_COUNT) - 1;
  }

  
  void UpdateListIndex(uint32_t aListIndex) {
    mListIndex = aListIndex;
  }

  





  void RemovingFromList();

  SVGTransform ToSVGTransform() const {
    return Transform();
  }

protected:
  
  friend class DOMSVGMatrix;
  const bool IsAnimVal() const {
    return mIsAnimValItem;
  }
  const gfxMatrix& Matrix() const {
    return Transform().Matrix();
  }
  void SetMatrix(const gfxMatrix& aMatrix);
  void ClearMatrixTearoff(DOMSVGMatrix* aMatrix);

private:
  nsSVGElement* Element() {
    return mList->Element();
  }

  



  SVGTransform& InternalItem();
  const SVGTransform& InternalItem() const;

#ifdef DEBUG
  bool IndexIsValid();
#endif

  const SVGTransform& Transform() const {
    return HasOwner() ? InternalItem() : *mTransform;
  }
  SVGTransform& Transform() {
    return HasOwner() ? InternalItem() : *mTransform;
  }
  inline nsAttrValue NotifyElementWillChange();
  void NotifyElementDidChange(const nsAttrValue& aEmptyOrOldValue);

  nsRefPtr<DOMSVGTransformList> mList;

  
  

  uint32_t mListIndex:MOZ_SVG_LIST_INDEX_BIT_COUNT;
  uint32_t mIsAnimValItem:1;

  
  
  
  
  
  
  nsAutoPtr<SVGTransform> mTransform;

  
  
  
  
  
  
  DOMSVGMatrix* mMatrixTearoff;
};

NS_DEFINE_STATIC_IID_ACCESSOR(DOMSVGTransform, MOZILLA_DOMSVGTRANSFORM_IID)

nsAttrValue
DOMSVGTransform::NotifyElementWillChange()
{
  nsAttrValue result;
  if (HasOwner()) {
    result = Element()->WillChangeTransformList();
  }
  return result;
}

} 

#undef MOZ_SVG_LIST_INDEX_BIT_COUNT

#endif 
