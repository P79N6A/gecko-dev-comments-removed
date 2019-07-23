




































#ifndef NS_SVGTRANSFORMSMILATTR_H_
#define NS_SVGTRANSFORMSMILATTR_H_

#include "nsISMILAttr.h"
#include "nsIAtom.h"
#include "nsString.h"

class nsSVGElement;
class nsSVGAnimatedTransformList;
class nsISMILType;
class nsIDOMSVGTransform;
class nsIDOMSVGTransformList;
class nsSVGSMILTransform;

class nsSVGTransformSMILAttr : public nsISMILAttr
{
public:
  nsSVGTransformSMILAttr(nsSVGAnimatedTransformList* aTransform,
                         nsSVGElement* aSVGElement)
    : mVal(aTransform), mSVGElement(aSVGElement) {}

  
  virtual nsresult ValueFromString(const nsAString& aStr,
                                   const nsISMILAnimationElement* aSrcElement,
                                   nsSMILValue& aValue,
                                   PRBool& aCanCache) const;
  virtual nsSMILValue  GetBaseValue() const;
  virtual void         ClearAnimValue();
  virtual nsresult     SetAnimValue(const nsSMILValue& aValue);

protected:
  static void ParseValue(const nsAString& aSpec,
                         const nsIAtom* aTransformType,
                         nsSMILValue& aResult);
  static PRInt32  ParseParameterList(const nsAString& aSpec, float* aVars,
                                     PRInt32 aNVars);
  static nsresult AppendSVGTransformToSMILValue(nsIDOMSVGTransform* transform,
                                                nsSMILValue& aValue);
  static nsresult UpdateFromSMILValue(nsIDOMSVGTransformList* aTransformList,
                                      const nsSMILValue& aValue);
  static nsresult GetSVGTransformFromSMILValue(
                    const nsSVGSMILTransform& aSMILTransform,
                    nsIDOMSVGTransform* aSVGTransform);

private:
  
  
  
  nsSVGAnimatedTransformList* mVal;
  nsSVGElement* mSVGElement;
};

#endif 
