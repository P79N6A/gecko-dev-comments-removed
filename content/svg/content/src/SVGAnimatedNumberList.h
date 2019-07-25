




#ifndef MOZILLA_SVGANIMATEDNUMBERLIST_H__
#define MOZILLA_SVGANIMATEDNUMBERLIST_H__

#include "nsAutoPtr.h"
#include "nsISMILAttr.h"
#include "SVGNumberList.h"

class nsISMILAnimationElement;
class nsSMILValue;
class nsSVGElement;

namespace mozilla {















class SVGAnimatedNumberList
{
  
  friend class DOMSVGNumber;
  friend class DOMSVGNumberList;

public:
  SVGAnimatedNumberList() {}

  






  const SVGNumberList& GetBaseValue() const {
    return mBaseVal;
  }

  nsresult SetBaseValueString(const nsAString& aValue);

  void ClearBaseValue(uint32_t aAttrEnum);

  const SVGNumberList& GetAnimValue() const {
    return mAnimVal ? *mAnimVal : mBaseVal;
  }

  nsresult SetAnimValue(const SVGNumberList& aValue,
                        nsSVGElement *aElement,
                        uint32_t aAttrEnum);

  void ClearAnimValue(nsSVGElement *aElement,
                      uint32_t aAttrEnum);

  
  
  
  
  
  bool IsExplicitlySet() const
    { return !!mAnimVal || mIsBaseSet; }
  
  bool IsAnimating() const {
    return !!mAnimVal;
  }

  
  nsISMILAttr* ToSMILAttr(nsSVGElement* aSVGElement, uint8_t aAttrEnum);

private:

  
  
  
  

  SVGNumberList mBaseVal;
  nsAutoPtr<SVGNumberList> mAnimVal;
  bool mIsBaseSet;

  struct SMILAnimatedNumberList : public nsISMILAttr
  {
  public:
    SMILAnimatedNumberList(SVGAnimatedNumberList* aVal,
                           nsSVGElement* aSVGElement,
                           uint8_t aAttrEnum)
      : mVal(aVal)
      , mElement(aSVGElement)
      , mAttrEnum(aAttrEnum)
    {}

    
    
    
    SVGAnimatedNumberList* mVal;
    nsSVGElement* mElement;
    uint8_t mAttrEnum;

    
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
