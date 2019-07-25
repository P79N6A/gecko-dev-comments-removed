




#ifndef MOZILLA_SVGANIMATEDLENGTHLIST_H__
#define MOZILLA_SVGANIMATEDLENGTHLIST_H__

#include "nsAutoPtr.h"
#include "nsISMILAttr.h"
#include "SVGLengthList.h"

class nsISMILAnimationElement;
class nsSMILValue;
class nsSVGElement;

namespace mozilla {















class SVGAnimatedLengthList
{
  
  friend class DOMSVGLength;
  friend class DOMSVGLengthList;

public:
  SVGAnimatedLengthList() {}

  






  const SVGLengthList& GetBaseValue() const {
    return mBaseVal;
  }

  nsresult SetBaseValueString(const nsAString& aValue);

  void ClearBaseValue(PRUint32 aAttrEnum);

  const SVGLengthList& GetAnimValue() const {
    return mAnimVal ? *mAnimVal : mBaseVal;
  }

  nsresult SetAnimValue(const SVGLengthList& aValue,
                        nsSVGElement *aElement,
                        PRUint32 aAttrEnum);

  void ClearAnimValue(nsSVGElement *aElement,
                      PRUint32 aAttrEnum);

  bool IsAnimating() const {
    return !!mAnimVal;
  }

  
  nsISMILAttr* ToSMILAttr(nsSVGElement* aSVGElement, PRUint8 aAttrEnum,
                          PRUint8 aAxis, bool aCanZeroPadList);

private:

  
  
  
  

  SVGLengthList mBaseVal;
  nsAutoPtr<SVGLengthList> mAnimVal;

  struct SMILAnimatedLengthList : public nsISMILAttr
  {
  public:
    SMILAnimatedLengthList(SVGAnimatedLengthList* aVal,
                           nsSVGElement* aSVGElement,
                           PRUint8 aAttrEnum,
                           PRUint8 aAxis,
                           bool aCanZeroPadList)
      : mVal(aVal)
      , mElement(aSVGElement)
      , mAttrEnum(aAttrEnum)
      , mAxis(aAxis)
      , mCanZeroPadList(aCanZeroPadList)
    {}

    
    
    
    SVGAnimatedLengthList* mVal;
    nsSVGElement* mElement;
    PRUint8 mAttrEnum;
    PRUint8 mAxis;
    bool mCanZeroPadList; 

    
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
