





#ifndef MOZILLA_DOMSVGANIMATEDTRANSFORMLIST_H__
#define MOZILLA_DOMSVGANIMATEDTRANSFORMLIST_H__

#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsIDOMSVGAnimTransformList.h"
#include "nsSVGElement.h"
#include "mozilla/Attributes.h"

namespace mozilla {

class DOMSVGTransformList;
class SVGAnimatedTransformList;

















class DOMSVGAnimatedTransformList MOZ_FINAL : public nsIDOMSVGAnimatedTransformList
{
  friend class DOMSVGTransformList;

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(DOMSVGAnimatedTransformList)
  NS_DECL_NSIDOMSVGANIMATEDTRANSFORMLIST

  










  static already_AddRefed<DOMSVGAnimatedTransformList>
    GetDOMWrapper(SVGAnimatedTransformList *aList, nsSVGElement *aElement);

  




  static DOMSVGAnimatedTransformList*
    GetDOMWrapperIfExists(SVGAnimatedTransformList *aList);

  











  void InternalBaseValListWillChangeLengthTo(PRUint32 aNewLength);
  void InternalAnimValListWillChangeLengthTo(PRUint32 aNewLength);

  



  bool IsAnimating() const;

private:

  



  DOMSVGAnimatedTransformList(nsSVGElement *aElement)
    : mBaseVal(nullptr)
    , mAnimVal(nullptr)
    , mElement(aElement)
  {}

  ~DOMSVGAnimatedTransformList();

  
  SVGAnimatedTransformList& InternalAList();
  const SVGAnimatedTransformList& InternalAList() const;

  
  
  
  DOMSVGTransformList *mBaseVal;
  DOMSVGTransformList *mAnimVal;

  
  
  nsRefPtr<nsSVGElement> mElement;
};

} 

#endif 
