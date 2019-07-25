



































#ifndef MOZILLA_DOMSVGANIMATEDNUMBERLIST_H__
#define MOZILLA_DOMSVGANIMATEDNUMBERLIST_H__

#include "nsIDOMSVGAnimatedNumberList.h"
#include "nsCycleCollectionParticipant.h"
#include "nsAutoPtr.h"
#include "nsTArray.h"

class nsSVGElement;

namespace mozilla {

class SVGAnimatedNumberList;
class SVGNumberList;
class DOMSVGNumberList;
















class DOMSVGAnimatedNumberList : public nsIDOMSVGAnimatedNumberList
{
  friend class DOMSVGNumberList;

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(DOMSVGAnimatedNumberList)
  NS_DECL_NSIDOMSVGANIMATEDNUMBERLIST

  










  static already_AddRefed<DOMSVGAnimatedNumberList>
    GetDOMWrapper(SVGAnimatedNumberList *aList,
                  nsSVGElement *aElement,
                  PRUint8 aAttrEnum);

  




  static DOMSVGAnimatedNumberList*
    GetDOMWrapperIfExists(SVGAnimatedNumberList *aList);

  











  void InternalBaseValListWillChangeTo(const SVGNumberList& aNewValue);
  void InternalAnimValListWillChangeTo(const SVGNumberList& aNewValue);

  



  bool IsAnimating() const;

private:

  



  DOMSVGAnimatedNumberList(nsSVGElement *aElement, PRUint8 aAttrEnum)
    : mBaseVal(nsnull)
    , mAnimVal(nsnull)
    , mElement(aElement)
    , mAttrEnum(aAttrEnum)
  {}

  ~DOMSVGAnimatedNumberList();

  
  SVGAnimatedNumberList& InternalAList();
  const SVGAnimatedNumberList& InternalAList() const;

  
  
  
  DOMSVGNumberList *mBaseVal;
  DOMSVGNumberList *mAnimVal;

  
  
  nsRefPtr<nsSVGElement> mElement;

  PRUint8 mAttrEnum;
};

} 

#endif 
