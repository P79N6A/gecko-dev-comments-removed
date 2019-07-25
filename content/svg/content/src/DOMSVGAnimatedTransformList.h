






































#ifndef MOZILLA_DOMSVGANIMATEDTRANSFORMLIST_H__
#define MOZILLA_DOMSVGANIMATEDTRANSFORMLIST_H__

#include "nsIDOMSVGAnimTransformList.h"
#include "nsCycleCollectionParticipant.h"
#include "nsAutoPtr.h"
#include "nsTArray.h"

class nsSVGElement;

namespace mozilla {

class SVGAnimatedTransformList;
class SVGTransformList;
class DOMSVGTransformList;

















class DOMSVGAnimatedTransformList : public nsIDOMSVGAnimatedTransformList
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
    : mBaseVal(nsnull)
    , mAnimVal(nsnull)
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
