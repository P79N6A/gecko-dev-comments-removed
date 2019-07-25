




































#include "nsSMILSetAnimationFunction.h"

inline bool
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

bool
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

bool
nsSMILSetAnimationFunction::UnsetAttr(nsIAtom* aAttribute)
{
  if (IsDisallowedAttribute(aAttribute)) {
    return PR_TRUE;
  }

  return nsSMILAnimationFunction::UnsetAttr(aAttribute);
}

bool
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

bool
nsSMILSetAnimationFunction::GetAttr(nsIAtom* aAttName,
                                    nsAString& aResult) const
{
  if (IsDisallowedAttribute(aAttName))
    return nsnull;

  return nsSMILAnimationFunction::GetAttr(aAttName, aResult);
}

bool
nsSMILSetAnimationFunction::WillReplace() const
{
  return PR_TRUE;
}
