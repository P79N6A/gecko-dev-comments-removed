




#ifndef __NS_SVGBOOLEAN_H__
#define __NS_SVGBOOLEAN_H__

#include "nsError.h"
#include "nsISMILAttr.h"
#include "mozilla/Attributes.h"

class nsIAtom;
class nsISMILAnimationElement;
class nsISupports;
class nsSMILValue;
class nsSVGElement;

class nsSVGBoolean
{

public:
  void Init(uint8_t aAttrEnum = 0xff, bool aValue = false) {
    mAnimVal = mBaseVal = aValue;
    mAttrEnum = aAttrEnum;
    mIsAnimated = false;
  }

  nsresult SetBaseValueAtom(const nsIAtom* aValue, nsSVGElement *aSVGElement);
  nsIAtom* GetBaseValueAtom() const;

  void SetBaseValue(bool aValue, nsSVGElement *aSVGElement);
  bool GetBaseValue() const
    { return mBaseVal; }

  void SetAnimValue(bool aValue, nsSVGElement *aSVGElement);
  bool GetAnimValue() const
    { return mAnimVal; }

  nsresult ToDOMAnimatedBoolean(nsISupports **aResult,
                                nsSVGElement* aSVGElement);
  
  nsISMILAttr* ToSMILAttr(nsSVGElement* aSVGElement);

private:

  bool mAnimVal;
  bool mBaseVal;
  bool mIsAnimated;
  uint8_t mAttrEnum; 

public:
  struct SMILBool : public nsISMILAttr
  {
  public:
    SMILBool(nsSVGBoolean* aVal, nsSVGElement* aSVGElement)
      : mVal(aVal), mSVGElement(aSVGElement) {}

    
    
    
    nsSVGBoolean* mVal;
    nsSVGElement* mSVGElement;

    
    virtual nsresult ValueFromString(const nsAString& aStr,
                                     const nsISMILAnimationElement* aSrcElement,
                                     nsSMILValue& aValue,
                                     bool& aPreventCachingOfSandwich) const;
    virtual nsSMILValue GetBaseValue() const;
    virtual void ClearAnimValue();
    virtual nsresult SetAnimValue(const nsSMILValue& aValue);
  };
};
#endif 
