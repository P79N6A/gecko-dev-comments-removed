



































#ifndef MOZILLA_SVGANIMATEDPATHSEGLIST_H__
#define MOZILLA_SVGANIMATEDPATHSEGLIST_H__

#include "SVGPathData.h"

class nsSVGElement;

#include "nsISMILAttr.h"

namespace mozilla {
















class SVGAnimatedPathSegList
{
  
  friend class DOMSVGPathSeg;
  friend class DOMSVGPathSegList;

public:
  SVGAnimatedPathSegList() {}

  





  const SVGPathData& GetBaseValue() const {
    return mBaseVal;
  }

  nsresult SetBaseValueString(const nsAString& aValue);

  void ClearBaseValue();

  


  const SVGPathData& GetAnimValue() const {
    return mAnimVal ? *mAnimVal : mBaseVal;
  }

  nsresult SetAnimValue(const SVGPathData& aValue,
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

  
  
  
  

  SVGPathData mBaseVal;
  nsAutoPtr<SVGPathData> mAnimVal;

  struct SMILAnimatedPathSegList : public nsISMILAttr
  {
  public:
    SMILAnimatedPathSegList(SVGAnimatedPathSegList* aVal,
                            nsSVGElement* aElement)
      : mVal(aVal)
      , mElement(aElement)
    {}

    
    
    
    SVGAnimatedPathSegList *mVal;
    nsSVGElement *mElement;

    
    virtual nsresult ValueFromString(const nsAString& aStr,
                                     const nsISMILAnimationElement* aSrcElement,
                                     nsSMILValue& aValue,
                                     bool& aPreventCachingOfSandwich) const;
    virtual nsSMILValue GetBaseValue() const;
    virtual void ClearAnimValue();
    virtual nsresult SetAnimValue(const nsSMILValue& aValue);
  };
};

} 

#endif 
