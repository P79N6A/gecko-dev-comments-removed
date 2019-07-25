




































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
    return true;
  }

  return false;
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
    return true;
  }

  return nsSMILAnimationFunction::SetAttr(aAttribute, aValue,
                                          aResult, aParseResult);
}

bool
nsSMILSetAnimationFunction::UnsetAttr(nsIAtom* aAttribute)
{
  if (IsDisallowedAttribute(aAttribute)) {
    return true;
  }

  return nsSMILAnimationFunction::UnsetAttr(aAttribute);
}

bool
nsSMILSetAnimationFunction::HasAttr(nsIAtom* aAttName) const
{
  if (IsDisallowedAttribute(aAttName))
    return false;

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
  return true;
}
