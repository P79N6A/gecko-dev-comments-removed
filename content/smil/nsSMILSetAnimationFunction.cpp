




































#include "nsSMILSetAnimationFunction.h"

inline PRBool
nsSMILSetAnimationFunction::IsDisallowedAttribute(
    const nsIAtom* aAttribute) const
{
  
  
  
  
  
  
  if (aAttribute == nsGkAtoms::calcMode ||
      aAttribute == nsGkAtoms::values ||
      aAttribute == nsGkAtoms::keyTimes ||
      aAttribute == nsGkAtoms::keySplines ||
      aAttribute == nsGkAtoms::from ||
      aAttribute == nsGkAtoms::by ||
      aAttribute == nsGkAtoms::additive ||
      aAttribute == nsGkAtoms::accumulate) {
    return PR_TRUE;
  }

  return PR_FALSE;
}

PRBool
nsSMILSetAnimationFunction::SetAttr(nsIAtom* aAttribute,
                                    const nsAString& aValue,
                                    nsAttrValue& aResult,
                                    nsresult* aParseResult)
{
  if (IsDisallowedAttribute(aAttribute)) {
    aResult.SetTo(aValue);
    if (aParseResult) {
      
      
      
      
      
      
      
      
      *aParseResult = NS_OK;
    }
    return PR_TRUE;
  }

  return nsSMILAnimationFunction::SetAttr(aAttribute, aValue,
                                          aResult, aParseResult);
}

PRBool
nsSMILSetAnimationFunction::UnsetAttr(nsIAtom* aAttribute)
{
  if (IsDisallowedAttribute(aAttribute)) {
    return PR_TRUE;
  }

  return nsSMILAnimationFunction::UnsetAttr(aAttribute);
}

nsSMILAnimationFunction::nsSMILCalcMode
nsSMILSetAnimationFunction::GetCalcMode() const
{
  return CALC_DISCRETE;
}

PRBool
nsSMILSetAnimationFunction::HasAttr(nsIAtom* aAttName) const
{
  if (IsDisallowedAttribute(aAttName))
    return PR_FALSE;

  return nsSMILAnimationFunction::HasAttr(aAttName);
}

const nsAttrValue*
nsSMILSetAnimationFunction::GetAttr(nsIAtom* aAttName) const
{
  if (IsDisallowedAttribute(aAttName))
    return nsnull;

  return nsSMILAnimationFunction::GetAttr(aAttName);
}

PRBool
nsSMILSetAnimationFunction::GetAttr(nsIAtom* aAttName,
                                    nsAString& aResult) const
{
  if (IsDisallowedAttribute(aAttName))
    return nsnull;

  return nsSMILAnimationFunction::GetAttr(aAttName, aResult);
}
