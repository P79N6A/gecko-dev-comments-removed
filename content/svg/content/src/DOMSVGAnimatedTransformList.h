





#ifndef MOZILLA_DOMSVGANIMATEDTRANSFORMLIST_H__
#define MOZILLA_DOMSVGANIMATEDTRANSFORMLIST_H__

#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsSVGElement.h"
#include "nsWrapperCache.h"
#include "mozilla/Attributes.h"

namespace mozilla {

class DOMSVGTransformList;
class SVGAnimatedTransformList;

















class DOMSVGAnimatedTransformList MOZ_FINAL : public nsISupports,
                                              public nsWrapperCache
{
  friend class DOMSVGTransformList;

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(DOMSVGAnimatedTransformList)

  










  static already_AddRefed<DOMSVGAnimatedTransformList>
    GetDOMWrapper(SVGAnimatedTransformList *aList, nsSVGElement *aElement);

  




  static DOMSVGAnimatedTransformList*
    GetDOMWrapperIfExists(SVGAnimatedTransformList *aList);

  











  void InternalBaseValListWillChangeLengthTo(uint32_t aNewLength);
  void InternalAnimValListWillChangeLengthTo(uint32_t aNewLength);

  



  bool IsAnimating() const;

  
  nsSVGElement* GetParentObject() const { return mElement; }
  virtual JSObject* WrapObject(JSContext* aCx, JSObject* aScope) MOZ_OVERRIDE;
  
  already_AddRefed<DOMSVGTransformList> BaseVal();
  already_AddRefed<DOMSVGTransformList> AnimVal();

private:

  



  explicit DOMSVGAnimatedTransformList(nsSVGElement *aElement)
    : mBaseVal(nullptr)
    , mAnimVal(nullptr)
    , mElement(aElement)
  {
    SetIsDOMBinding();
  }

  ~DOMSVGAnimatedTransformList();

  
  SVGAnimatedTransformList& InternalAList();
  const SVGAnimatedTransformList& InternalAList() const;

  
  
  
  DOMSVGTransformList *mBaseVal;
  DOMSVGTransformList *mAnimVal;

  
  
  nsRefPtr<nsSVGElement> mElement;
};

} 

#endif 
