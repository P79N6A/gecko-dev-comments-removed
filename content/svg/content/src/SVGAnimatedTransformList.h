





































#ifndef MOZILLA_SVGANIMATEDTRANSFORMLIST_H__
#define MOZILLA_SVGANIMATEDTRANSFORMLIST_H__

#include "SVGTransformList.h"

class nsSVGElement;

#ifdef MOZ_SMIL
#include "nsISMILAttr.h"
#endif 

namespace mozilla {















class SVGAnimatedTransformList
{
  
  friend class DOMSVGTransform;
  friend class DOMSVGTransformList;

public:
  SVGAnimatedTransformList() : mIsAttrSet(PR_FALSE) { }

  






  const SVGTransformList& GetBaseValue() const {
    return mBaseVal;
  }

  nsresult SetBaseValueString(const nsAString& aValue);

  void ClearBaseValue();

  const SVGTransformList& GetAnimValue() const {
    return mAnimVal ? *mAnimVal : mBaseVal;
  }

  nsresult SetAnimValue(const SVGTransformList& aNewAnimValue,
                        nsSVGElement *aElement);

  void ClearAnimValue(nsSVGElement *aElement);

  PRBool IsExplicitlySet() const;

  PRBool IsAnimating() const {
    return !!mAnimVal;
  }

#ifdef MOZ_SMIL
  
  nsISMILAttr* ToSMILAttr(nsSVGElement* aSVGElement);
#endif 

private:

  
  
  
  

  SVGTransformList mBaseVal;
  nsAutoPtr<SVGTransformList> mAnimVal;
  PRPackedBool mIsAttrSet;

#ifdef MOZ_SMIL
  struct SMILAnimatedTransformList : public nsISMILAttr
  {
  public:
    SMILAnimatedTransformList(SVGAnimatedTransformList* aVal,
                              nsSVGElement* aSVGElement)
      : mVal(aVal)
      , mElement(aSVGElement)
    {}

    
    virtual nsresult ValueFromString(const nsAString& aStr,
                                     const nsISMILAnimationElement* aSrcElement,
                                     nsSMILValue& aValue,
                                     PRBool& aPreventCachingOfSandwich) const;
    virtual nsSMILValue GetBaseValue() const;
    virtual void ClearAnimValue();
    virtual nsresult SetAnimValue(const nsSMILValue& aValue);

  protected:
    static void ParseValue(const nsAString& aSpec,
                           const nsIAtom* aTransformType,
                           nsSMILValue& aResult);
    static PRInt32 ParseParameterList(const nsAString& aSpec, float* aVars,
                                      PRInt32 aNVars);

    
    
    
    SVGAnimatedTransformList* mVal;
    nsSVGElement* mElement;
  };
#endif 
};

} 

#endif 
