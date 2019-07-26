




#ifndef MOZILLA_SVGANIMATEDPOINTLIST_H__
#define MOZILLA_SVGANIMATEDPOINTLIST_H__

#include "nsAutoPtr.h"
#include "nsISMILAttr.h"
#include "SVGPointList.h"

class nsSMILValue;
class nsSVGElement;

namespace mozilla {

namespace dom {
class SVGAnimationElement;
}
















class SVGAnimatedPointList
{
  
  friend class DOMSVGPoint;
  friend class DOMSVGPointList;

public:
  SVGAnimatedPointList() {}

  





  const SVGPointList& GetBaseValue() const {
    return mBaseVal;
  }

  nsresult SetBaseValueString(const nsAString& aValue);

  void ClearBaseValue();

  


  const SVGPointList& GetAnimValue() const {
    return mAnimVal ? *mAnimVal : mBaseVal;
  }

  nsresult SetAnimValue(const SVGPointList& aValue,
                        nsSVGElement *aElement);

  void ClearAnimValue(nsSVGElement *aElement);

  



  void *GetBaseValKey() const {
    return (void*)&mBaseVal;
  }
  void *GetAnimValKey() const {
    return (void*)&mAnimVal;
  }

  bool IsAnimating() const {
    return !!mAnimVal;
  }

  
  nsISMILAttr* ToSMILAttr(nsSVGElement* aElement);

private:

  
  
  
  

  SVGPointList mBaseVal;
  nsAutoPtr<SVGPointList> mAnimVal;

  struct SMILAnimatedPointList : public nsISMILAttr
  {
  public:
    SMILAnimatedPointList(SVGAnimatedPointList* aVal,
                          nsSVGElement* aElement)
      : mVal(aVal)
      , mElement(aElement)
    {}

    
    
    
    SVGAnimatedPointList *mVal;
    nsSVGElement *mElement;

    
    virtual nsresult ValueFromString(const nsAString& aStr,
                                     const dom::SVGAnimationElement* aSrcElement,
                                     nsSMILValue& aValue,
                                     bool& aPreventCachingOfSandwich) const;
    virtual nsSMILValue GetBaseValue() const;
    virtual void ClearAnimValue();
    virtual nsresult SetAnimValue(const nsSMILValue& aValue);
  };
};

} 

#endif 
