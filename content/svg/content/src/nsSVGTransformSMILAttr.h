




































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
    : mVal(aTransform),
      mSVGElement(aSVGElement) {}

  
  virtual nsISMILType* GetSMILType() const;
  virtual nsresult     ValueFromString(const nsAString& aStr,
                                   const nsISMILAnimationElement* aSrcElement,
                                   nsSMILValue& aValue) const;
  virtual nsSMILValue  GetBaseValue() const;
  virtual void         ClearAnimValue();
  virtual nsresult     SetAnimValue(const nsSMILValue& aValue);

protected:
  nsresult ParseValue(const nsAString& aSpec,
                      const nsIAtom* aTransformType,
                      nsSMILValue& aResult) const;
  PRInt32  ParseParameterList(const nsAString& aSpec, float* aVars,
                              PRInt32 aNVars) const;
  PRBool   IsSpace(const char c) const;
  void     SkipWsp(nsACString::const_iterator& aIter,
                   const nsACString::const_iterator& aIterEnd) const;
  nsresult AppendSVGTransformToSMILValue(nsIDOMSVGTransform* transform,
                                         nsSMILValue& aValue) const;
  nsresult UpdateFromSMILValue(nsIDOMSVGTransformList* aTransformList,
                               const nsSMILValue& aValue);
  nsresult GetSVGTransformFromSMILValue(
                        const nsSVGSMILTransform& aSMILTransform,
                        nsIDOMSVGTransform* aSVGTransform) const;
  already_AddRefed<nsIDOMSVGTransform> GetSVGTransformFromSMILValue(
                                        const nsSMILValue& aValue) const;

private:
  
  
  
  nsSVGAnimatedTransformList* mVal;
  nsSVGElement* mSVGElement;
};

#endif 
