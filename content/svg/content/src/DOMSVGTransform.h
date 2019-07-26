





#ifndef MOZILLA_DOMSVGTRANSFORM_H__
#define MOZILLA_DOMSVGTRANSFORM_H__

#include "DOMSVGTransformList.h"
#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsDebug.h"
#include "nsID.h"
#include "SVGTransform.h"
#include "nsWrapperCache.h"
#include "mozilla/Attributes.h"

class nsSVGElement;

struct gfxMatrix;






#define MOZILLA_DOMSVGTRANSFORM_IID \
  { 0x0A799862, 0x9469, 0x41FE, \
    { 0xB4, 0xCD, 0x20, 0x19, 0xE6, 0x5D, 0x8D, 0xA6 } }

#define MOZ_SVG_LIST_INDEX_BIT_COUNT 31 // supports > 2 billion list items

namespace mozilla {

namespace dom {
class SVGMatrix;
}




class DOMSVGTransform MOZ_FINAL : public nsISupports,
                                  public nsWrapperCache
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(MOZILLA_DOMSVGTRANSFORM_IID)
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(DOMSVGTransform)

  


  DOMSVGTransform(DOMSVGTransformList *aList,
                  uint32_t aListIndex,
                  bool aIsAnimValItem);

  






  explicit DOMSVGTransform();
  explicit DOMSVGTransform(const gfxMatrix &aMatrix);

  


  explicit DOMSVGTransform(const SVGTransform &aMatrix);

  ~DOMSVGTransform();

  



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

  
  DOMSVGTransformList* GetParentObject() const { return mList; }
  virtual JSObject* WrapObject(JSContext* aCx, JSObject* aScope, bool* aTriedToWrap);
  uint16_t Type() const;
  dom::SVGMatrix* Matrix();
  float Angle() const;
  void SetMatrix(dom::SVGMatrix& matrix, ErrorResult& rv);
  void SetTranslate(float tx, float ty, ErrorResult& rv);
  void SetScale(float sx, float sy, ErrorResult& rv);
  void SetRotate(float angle, float cx, float cy, ErrorResult& rv);
  void SetSkewX(float angle, ErrorResult& rv);
  void SetSkewY(float angle, ErrorResult& rv);

protected:
  
  friend class dom::SVGMatrix;
  const bool IsAnimVal() const {
    return mIsAnimValItem;
  }
  const gfxMatrix& Matrixgfx() const {
    return Transform().Matrix();
  }
  void SetMatrix(const gfxMatrix& aMatrix);

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
