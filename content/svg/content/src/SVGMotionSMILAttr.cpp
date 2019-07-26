






#include "SVGMotionSMILAttr.h"
#include "SVGMotionSMILType.h"
#include "nsISMILAnimationElement.h"
#include "nsSMILValue.h"
#include "nsDebug.h"
#include "nsSVGElement.h"

namespace mozilla {

nsresult
SVGMotionSMILAttr::ValueFromString(const nsAString& aStr,
                                   const nsISMILAnimationElement* aSrcElement,
                                   nsSMILValue& aValue,
                                   bool& aPreventCachingOfSandwich) const
{
  NS_NOTREACHED("Shouldn't using nsISMILAttr::ValueFromString for parsing "
                "animateMotion's SMIL values.");
  return NS_ERROR_FAILURE;
}

nsSMILValue
SVGMotionSMILAttr::GetBaseValue() const
{
  return nsSMILValue(&SVGMotionSMILType::sSingleton);
}

void
SVGMotionSMILAttr::ClearAnimValue()
{
  mSVGElement->SetAnimateMotionTransform(nullptr);
}

nsresult
SVGMotionSMILAttr::SetAnimValue(const nsSMILValue& aValue)
{
  gfxMatrix matrix = SVGMotionSMILType::CreateMatrix(aValue);
  mSVGElement->SetAnimateMotionTransform(&matrix);
  return NS_OK;
}

const nsIContent*
SVGMotionSMILAttr::GetTargetNode() const
{
  return mSVGElement;
}

} 
