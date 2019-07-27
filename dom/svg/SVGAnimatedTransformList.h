





#ifndef mozilla_dom_SVGAnimatedTransformList_h
#define mozilla_dom_SVGAnimatedTransformList_h

#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsSVGElement.h"
#include "nsWrapperCache.h"
#include "mozilla/Attributes.h"

namespace mozilla {

class DOMSVGTransformList;
class nsSVGAnimatedTransformList;

namespace dom {

















class SVGAnimatedTransformList MOZ_FINAL : public nsWrapperCache
{
  friend class mozilla::DOMSVGTransformList;

public:
  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(SVGAnimatedTransformList)
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(SVGAnimatedTransformList)

  










  static already_AddRefed<SVGAnimatedTransformList>
    GetDOMWrapper(nsSVGAnimatedTransformList *aList, nsSVGElement *aElement);

  




  static SVGAnimatedTransformList*
    GetDOMWrapperIfExists(nsSVGAnimatedTransformList *aList);

  











  void InternalBaseValListWillChangeLengthTo(uint32_t aNewLength);
  void InternalAnimValListWillChangeLengthTo(uint32_t aNewLength);

  



  bool IsAnimating() const;

  
  nsSVGElement* GetParentObject() const { return mElement; }
  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;
  
  already_AddRefed<DOMSVGTransformList> BaseVal();
  already_AddRefed<DOMSVGTransformList> AnimVal();

private:

  



  explicit SVGAnimatedTransformList(nsSVGElement *aElement)
    : mBaseVal(nullptr)
    , mAnimVal(nullptr)
    , mElement(aElement)
  {
    SetIsDOMBinding();
  }

  ~SVGAnimatedTransformList();

  
  nsSVGAnimatedTransformList& InternalAList();
  const nsSVGAnimatedTransformList& InternalAList() const;

  
  
  
  DOMSVGTransformList *mBaseVal;
  DOMSVGTransformList *mAnimVal;

  
  
  nsRefPtr<nsSVGElement> mElement;
};

} 
} 

#endif 
