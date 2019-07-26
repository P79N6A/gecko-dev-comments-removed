




#ifndef MOZILLA_DOMSVGANIMATEDLENGTHLIST_H__
#define MOZILLA_DOMSVGANIMATEDLENGTHLIST_H__

#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsIDOMSVGAnimatedLengthList.h"
#include "nsSVGElement.h"
#include "mozilla/Attributes.h"

namespace mozilla {

class SVGAnimatedLengthList;
class SVGLengthList;
class DOMSVGLengthList;






















































































class DOMSVGAnimatedLengthList MOZ_FINAL : public nsIDOMSVGAnimatedLengthList
{
  friend class DOMSVGLengthList;

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(DOMSVGAnimatedLengthList)
  NS_DECL_NSIDOMSVGANIMATEDLENGTHLIST

  










  static already_AddRefed<DOMSVGAnimatedLengthList>
    GetDOMWrapper(SVGAnimatedLengthList *aList,
                  nsSVGElement *aElement,
                  PRUint8 aAttrEnum,
                  PRUint8 aAxis);

  




  static DOMSVGAnimatedLengthList*
    GetDOMWrapperIfExists(SVGAnimatedLengthList *aList);

  











  void InternalBaseValListWillChangeTo(const SVGLengthList& aNewValue);
  void InternalAnimValListWillChangeTo(const SVGLengthList& aNewValue);

  



  bool IsAnimating() const;

private:

  



  DOMSVGAnimatedLengthList(nsSVGElement *aElement, PRUint8 aAttrEnum, PRUint8 aAxis)
    : mBaseVal(nullptr)
    , mAnimVal(nullptr)
    , mElement(aElement)
    , mAttrEnum(aAttrEnum)
    , mAxis(aAxis)
  {}

  ~DOMSVGAnimatedLengthList();

  
  SVGAnimatedLengthList& InternalAList();
  const SVGAnimatedLengthList& InternalAList() const;

  
  
  
  DOMSVGLengthList *mBaseVal;
  DOMSVGLengthList *mAnimVal;

  
  
  nsRefPtr<nsSVGElement> mElement;

  PRUint8 mAttrEnum;
  PRUint8 mAxis;
};

} 

#endif 
