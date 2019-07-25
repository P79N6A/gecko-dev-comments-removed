



































#ifndef MOZILLA_SVGANIMATEDPOINTLIST_H__
#define MOZILLA_SVGANIMATEDPOINTLIST_H__

#include "SVGPointList.h"

class nsSVGElement;

#ifdef MOZ_SMIL
#include "nsISMILAttr.h"
#endif 

namespace mozilla {
















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

#ifdef MOZ_SMIL
  
  nsISMILAttr* ToSMILAttr(nsSVGElement* aElement);
#endif 

private:

  
  
  
  

  SVGPointList mBaseVal;
  nsAutoPtr<SVGPointList> mAnimVal;

#ifdef MOZ_SMIL
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
                                     const nsISMILAnimationElement* aSrcElement,
                                     nsSMILValue& aValue,
                                     bool& aPreventCachingOfSandwich) const;
    virtual nsSMILValue GetBaseValue() const;
    virtual void ClearAnimValue();
    virtual nsresult SetAnimValue(const nsSMILValue& aValue);
  };
#endif 
};

} 

#endif 
