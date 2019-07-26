





#ifndef MOZILLA_SVGANIMATEDTRANSFORMLIST_H__
#define MOZILLA_SVGANIMATEDTRANSFORMLIST_H__

#include "mozilla/Attributes.h"
#include "nsAutoPtr.h"
#include "nsISMILAttr.h"
#include "SVGTransformList.h"

class nsIAtom;
class nsSMILValue;
class nsSVGElement;

namespace mozilla {

namespace dom {
class SVGAnimationElement;
class SVGTransform;
}















class nsSVGAnimatedTransformList
{
  
  friend class dom::SVGTransform;
  friend class DOMSVGTransformList;

public:
  nsSVGAnimatedTransformList() : mIsAttrSet(false) { }

  






  const SVGTransformList& GetBaseValue() const {
    return mBaseVal;
  }

  nsresult SetBaseValue(const SVGTransformList& aValue);

  nsresult SetBaseValueString(const nsAString& aValue);

  void ClearBaseValue();

  const SVGTransformList& GetAnimValue() const {
    return mAnimVal ? *mAnimVal : mBaseVal;
  }

  nsresult SetAnimValue(const SVGTransformList& aNewAnimValue,
                        nsSVGElement *aElement);

  void ClearAnimValue(nsSVGElement *aElement);

  




  bool IsExplicitlySet() const;

  





  bool HasTransform() const
    { return (mAnimVal && !mAnimVal->IsEmpty()) || !mBaseVal.IsEmpty(); }

  bool IsAnimating() const {
    return !!mAnimVal;
  }

  
  nsISMILAttr* ToSMILAttr(nsSVGElement* aSVGElement);

private:

  
  
  
  

  SVGTransformList mBaseVal;
  nsAutoPtr<SVGTransformList> mAnimVal;
  bool mIsAttrSet;

  struct SMILAnimatedTransformList : public nsISMILAttr
  {
  public:
    SMILAnimatedTransformList(nsSVGAnimatedTransformList* aVal,
                              nsSVGElement* aSVGElement)
      : mVal(aVal)
      , mElement(aSVGElement)
    {}

    
    virtual nsresult ValueFromString(const nsAString& aStr,
                                     const dom::SVGAnimationElement* aSrcElement,
                                     nsSMILValue& aValue,
                                     bool& aPreventCachingOfSandwich) const MOZ_OVERRIDE;
    virtual nsSMILValue GetBaseValue() const MOZ_OVERRIDE;
    virtual void ClearAnimValue() MOZ_OVERRIDE;
    virtual nsresult SetAnimValue(const nsSMILValue& aValue) MOZ_OVERRIDE;

  protected:
    static void ParseValue(const nsAString& aSpec,
                           const nsIAtom* aTransformType,
                           nsSMILValue& aResult);
    static int32_t ParseParameterList(const nsAString& aSpec, float* aVars,
                                      int32_t aNVars);

    
    
    
    nsSVGAnimatedTransformList* mVal;
    nsSVGElement* mElement;
  };
};

} 

#endif 
