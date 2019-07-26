




#ifndef MOZILLA_DOMSVGANIMATEDLENGTHLIST_H__
#define MOZILLA_DOMSVGANIMATEDLENGTHLIST_H__

#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsSVGElement.h"
#include "mozilla/Attributes.h"

namespace mozilla {

class SVGAnimatedLengthList;
class SVGLengthList;
class DOMSVGLengthList;






















































































class DOMSVGAnimatedLengthList MOZ_FINAL : public nsWrapperCache
{
  friend class DOMSVGLengthList;

public:
  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(DOMSVGAnimatedLengthList)
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(DOMSVGAnimatedLengthList)

  










  static already_AddRefed<DOMSVGAnimatedLengthList>
    GetDOMWrapper(SVGAnimatedLengthList *aList,
                  nsSVGElement *aElement,
                  uint8_t aAttrEnum,
                  uint8_t aAxis);

  




  static DOMSVGAnimatedLengthList*
    GetDOMWrapperIfExists(SVGAnimatedLengthList *aList);

  











  void InternalBaseValListWillChangeTo(const SVGLengthList& aNewValue);
  void InternalAnimValListWillChangeTo(const SVGLengthList& aNewValue);

  



  bool IsAnimating() const;

  
  nsSVGElement* GetParentObject() const { return mElement; }
  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;
  
  already_AddRefed<DOMSVGLengthList> BaseVal();
  already_AddRefed<DOMSVGLengthList> AnimVal();

private:

  



  DOMSVGAnimatedLengthList(nsSVGElement *aElement, uint8_t aAttrEnum, uint8_t aAxis)
    : mBaseVal(nullptr)
    , mAnimVal(nullptr)
    , mElement(aElement)
    , mAttrEnum(aAttrEnum)
    , mAxis(aAxis)
  {
    SetIsDOMBinding();
  }

  ~DOMSVGAnimatedLengthList();

  
  SVGAnimatedLengthList& InternalAList();
  const SVGAnimatedLengthList& InternalAList() const;

  
  
  
  DOMSVGLengthList *mBaseVal;
  DOMSVGLengthList *mAnimVal;

  
  
  nsRefPtr<nsSVGElement> mElement;

  uint8_t mAttrEnum;
  uint8_t mAxis;
};

} 

#endif 
