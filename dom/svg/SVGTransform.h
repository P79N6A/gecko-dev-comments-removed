





#ifndef mozilla_dom_SVGTransform_h
#define mozilla_dom_SVGTransform_h

#include "DOMSVGTransformList.h"
#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsDebug.h"
#include "nsID.h"
#include "nsSVGTransform.h"
#include "nsWrapperCache.h"
#include "mozilla/Attributes.h"

class nsSVGElement;

class gfxMatrix;

#define MOZ_SVG_LIST_INDEX_BIT_COUNT 31 // supports > 2 billion list items

namespace mozilla {
namespace dom {

class SVGMatrix;




class SVGTransform MOZ_FINAL : public nsWrapperCache
{
  friend class AutoChangeTransformNotifier;

public:
  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(SVGTransform)
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(SVGTransform)

  


  SVGTransform(DOMSVGTransformList *aList,
               uint32_t aListIndex,
               bool aIsAnimValItem);

  






  explicit SVGTransform();
  explicit SVGTransform(const gfxMatrix &aMatrix);

  


  explicit SVGTransform(const nsSVGTransform &aMatrix);

  



  SVGTransform* Clone() {
    NS_ASSERTION(mList, "unexpected caller");
    return new SVGTransform(InternalItem());
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

  nsSVGTransform ToSVGTransform() const {
    return Transform();
  }

  
  DOMSVGTransformList* GetParentObject() const { return mList; }
  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;
  uint16_t Type() const;
  dom::SVGMatrix* GetMatrix();
  float Angle() const;
  void SetMatrix(dom::SVGMatrix& matrix, ErrorResult& rv);
  void SetTranslate(float tx, float ty, ErrorResult& rv);
  void SetScale(float sx, float sy, ErrorResult& rv);
  void SetRotate(float angle, float cx, float cy, ErrorResult& rv);
  void SetSkewX(float angle, ErrorResult& rv);
  void SetSkewY(float angle, ErrorResult& rv);

protected:
  ~SVGTransform();

  
  friend class dom::SVGMatrix;
  const bool IsAnimVal() const {
    return mIsAnimValItem;
  }
  const gfxMatrix& Matrixgfx() const {
    return Transform().GetMatrix();
  }
  void SetMatrix(const gfxMatrix& aMatrix);

private:
  nsSVGElement* Element() {
    return mList->Element();
  }

  



  nsSVGTransform& InternalItem();
  const nsSVGTransform& InternalItem() const;

#ifdef DEBUG
  bool IndexIsValid();
#endif

  const nsSVGTransform& Transform() const {
    return HasOwner() ? InternalItem() : *mTransform;
  }
  nsSVGTransform& Transform() {
    return HasOwner() ? InternalItem() : *mTransform;
  }

  nsRefPtr<DOMSVGTransformList> mList;

  
  

  uint32_t mListIndex:MOZ_SVG_LIST_INDEX_BIT_COUNT;
  uint32_t mIsAnimValItem:1;

  
  
  
  
  
  
  nsAutoPtr<nsSVGTransform> mTransform;
};

} 
} 

#undef MOZ_SVG_LIST_INDEX_BIT_COUNT

#endif 
