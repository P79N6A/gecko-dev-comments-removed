




#ifndef MOZILLA_SVGANIMATEDLENGTHLIST_H__
#define MOZILLA_SVGANIMATEDLENGTHLIST_H__

#include "mozilla/Attributes.h"
#include "nsAutoPtr.h"
#include "nsISMILAttr.h"
#include "SVGLengthList.h"

class nsSMILValue;
class nsSVGElement;

namespace mozilla {

namespace dom {
class SVGAnimationElement;
}















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

  void ClearBaseValue(uint32_t aAttrEnum);

  const SVGLengthList& GetAnimValue() const {
    return mAnimVal ? *mAnimVal : mBaseVal;
  }

  nsresult SetAnimValue(const SVGLengthList& aValue,
                        nsSVGElement *aElement,
                        uint32_t aAttrEnum);

  void ClearAnimValue(nsSVGElement *aElement,
                      uint32_t aAttrEnum);

  bool IsAnimating() const {
    return !!mAnimVal;
  }

  
  nsISMILAttr* ToSMILAttr(nsSVGElement* aSVGElement, uint8_t aAttrEnum,
                          uint8_t aAxis, bool aCanZeroPadList);

private:

  
  
  
  

  SVGLengthList mBaseVal;
  nsAutoPtr<SVGLengthList> mAnimVal;

  struct SMILAnimatedLengthList : public nsISMILAttr
  {
  public:
    SMILAnimatedLengthList(SVGAnimatedLengthList* aVal,
                           nsSVGElement* aSVGElement,
                           uint8_t aAttrEnum,
                           uint8_t aAxis,
                           bool aCanZeroPadList)
      : mVal(aVal)
      , mElement(aSVGElement)
      , mAttrEnum(aAttrEnum)
      , mAxis(aAxis)
      , mCanZeroPadList(aCanZeroPadList)
    {}

    
    
    
    SVGAnimatedLengthList* mVal;
    nsSVGElement* mElement;
    uint8_t mAttrEnum;
    uint8_t mAxis;
    bool mCanZeroPadList; 

    
    virtual nsresult ValueFromString(const nsAString& aStr,
                                     const dom::SVGAnimationElement* aSrcElement,
                                     nsSMILValue& aValue,
                                     bool& aPreventCachingOfSandwich) const MOZ_OVERRIDE;
    virtual nsSMILValue GetBaseValue() const MOZ_OVERRIDE;
    virtual void ClearAnimValue() MOZ_OVERRIDE;
    virtual nsresult SetAnimValue(const nsSMILValue& aValue) MOZ_OVERRIDE;
  };
};

} 

#endif 
