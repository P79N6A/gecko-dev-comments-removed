



































#ifndef MOZILLA_SVGANIMATEDNUMBERLIST_H__
#define MOZILLA_SVGANIMATEDNUMBERLIST_H__

#include "SVGNumberList.h"

class nsSVGElement;

#include "nsISMILAttr.h"

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

  void ClearBaseValue(PRUint32 aAttrEnum);

  const SVGNumberList& GetAnimValue() const {
    return mAnimVal ? *mAnimVal : mBaseVal;
  }

  nsresult SetAnimValue(const SVGNumberList& aValue,
                        nsSVGElement *aElement,
                        PRUint32 aAttrEnum);

  void ClearAnimValue(nsSVGElement *aElement,
                      PRUint32 aAttrEnum);

  
  
  
  
  
  bool IsExplicitlySet() const
    { return !!mAnimVal || mIsBaseSet; }
  
  bool IsAnimating() const {
    return !!mAnimVal;
  }

  
  nsISMILAttr* ToSMILAttr(nsSVGElement* aSVGElement, PRUint8 aAttrEnum);

private:

  
  
  
  

  SVGNumberList mBaseVal;
  nsAutoPtr<SVGNumberList> mAnimVal;
  bool mIsBaseSet;

  struct SMILAnimatedNumberList : public nsISMILAttr
  {
  public:
    SMILAnimatedNumberList(SVGAnimatedNumberList* aVal,
                           nsSVGElement* aSVGElement,
                           PRUint8 aAttrEnum)
      : mVal(aVal)
      , mElement(aSVGElement)
      , mAttrEnum(aAttrEnum)
    {}

    
    
    
    SVGAnimatedNumberList* mVal;
    nsSVGElement* mElement;
    PRUint8 mAttrEnum;

    
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
