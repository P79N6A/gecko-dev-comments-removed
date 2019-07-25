



































#include "SVGNumberListSMILType.h"
#include "nsSMILValue.h"
#include "SVGNumberList.h"
#include <math.h>


















namespace mozilla {

 SVGNumberListSMILType SVGNumberListSMILType::sSingleton;




void
SVGNumberListSMILType::Init(nsSMILValue &aValue) const
{
  NS_ABORT_IF_FALSE(aValue.IsNull(), "Unexpected value type");

  SVGNumberListAndInfo* numberList = new SVGNumberListAndInfo();

  aValue.mU.mPtr = numberList;
  aValue.mType = this;
}

void
SVGNumberListSMILType::Destroy(nsSMILValue& aValue) const
{
  NS_PRECONDITION(aValue.mType == this, "Unexpected SMIL value type");
  delete static_cast<SVGNumberListAndInfo*>(aValue.mU.mPtr);
  aValue.mU.mPtr = nsnull;
  aValue.mType = &nsSMILNullType::sSingleton;
}

nsresult
SVGNumberListSMILType::Assign(nsSMILValue& aDest,
                              const nsSMILValue& aSrc) const
{
  NS_PRECONDITION(aDest.mType == aSrc.mType, "Incompatible SMIL types");
  NS_PRECONDITION(aDest.mType == this, "Unexpected SMIL value");

  const SVGNumberListAndInfo* src =
    static_cast<const SVGNumberListAndInfo*>(aSrc.mU.mPtr);
  SVGNumberListAndInfo* dest =
    static_cast<SVGNumberListAndInfo*>(aDest.mU.mPtr);

  return dest->CopyFrom(*src);
}

PRBool
SVGNumberListSMILType::IsEqual(const nsSMILValue& aLeft,
                               const nsSMILValue& aRight) const
{
  NS_PRECONDITION(aLeft.mType == aRight.mType, "Incompatible SMIL types");
  NS_PRECONDITION(aLeft.mType == this, "Unexpected type for SMIL value");

  return *static_cast<const SVGNumberListAndInfo*>(aLeft.mU.mPtr) ==
         *static_cast<const SVGNumberListAndInfo*>(aRight.mU.mPtr);
}

nsresult
SVGNumberListSMILType::Add(nsSMILValue& aDest,
                           const nsSMILValue& aValueToAdd,
                           PRUint32 aCount) const
{
  NS_PRECONDITION(aDest.mType == this, "Unexpected SMIL type");
  NS_PRECONDITION(aValueToAdd.mType == this, "Incompatible SMIL type");

  SVGNumberListAndInfo& dest =
    *static_cast<SVGNumberListAndInfo*>(aDest.mU.mPtr);
  const SVGNumberListAndInfo& valueToAdd =
    *static_cast<const SVGNumberListAndInfo*>(aValueToAdd.mU.mPtr);

  NS_ABORT_IF_FALSE(dest.Element() || valueToAdd.Element(),
                    "Target element propagation failure");

  if (!valueToAdd.Element()) {
    NS_ABORT_IF_FALSE(valueToAdd.Length() == 0,
                      "Not identity value - target element propagation failure");
    return NS_OK;
  }
  if (!dest.Element()) {
    NS_ABORT_IF_FALSE(dest.Length() == 0,
                      "Not identity value - target element propagation failure");
    if (!dest.SetLength(valueToAdd.Length())) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    for (PRUint32 i = 0; i < dest.Length(); ++i) {
      dest[i] = aCount * valueToAdd[i];
    }
    dest.SetInfo(valueToAdd.Element()); 
    return NS_OK;
  }
  NS_ABORT_IF_FALSE(dest.Element() == valueToAdd.Element(),
                    "adding values from different elements...?");
  if (dest.Length() != valueToAdd.Length()) {
    
    
    return NS_ERROR_FAILURE;
  }
  for (PRUint32 i = 0; i < dest.Length(); ++i) {
    dest[i] += aCount * valueToAdd[i];
  }
  dest.SetInfo(valueToAdd.Element()); 
  return NS_OK;
}

nsresult
SVGNumberListSMILType::ComputeDistance(const nsSMILValue& aFrom,
                                       const nsSMILValue& aTo,
                                       double& aDistance) const
{
  NS_PRECONDITION(aFrom.mType == this, "Unexpected SMIL type");
  NS_PRECONDITION(aTo.mType == this, "Incompatible SMIL type");

  const SVGNumberListAndInfo& from =
    *static_cast<const SVGNumberListAndInfo*>(aFrom.mU.mPtr);
  const SVGNumberListAndInfo& to =
    *static_cast<const SVGNumberListAndInfo*>(aTo.mU.mPtr);

  if (from.Length() != to.Length()) {
    
    
    return NS_ERROR_FAILURE;
  }

  
  

  double total = 0.0;

  for (PRUint32 i = 0; i < to.Length(); ++i) {
    double delta = to[i] - from[i];
    total += delta * delta;
  }
  double distance = sqrt(total);
  if (!NS_FloatIsFinite(distance)) {
    return NS_ERROR_FAILURE;
  }
  aDistance = distance;

  return NS_OK;
}

nsresult
SVGNumberListSMILType::Interpolate(const nsSMILValue& aStartVal,
                                   const nsSMILValue& aEndVal,
                                   double aUnitDistance,
                                   nsSMILValue& aResult) const
{
  NS_PRECONDITION(aStartVal.mType == aEndVal.mType,
                  "Trying to interpolate different types");
  NS_PRECONDITION(aStartVal.mType == this,
                  "Unexpected types for interpolation");
  NS_PRECONDITION(aResult.mType == this, "Unexpected result type");

  const SVGNumberListAndInfo& start =
    *static_cast<const SVGNumberListAndInfo*>(aStartVal.mU.mPtr);
  const SVGNumberListAndInfo& end =
    *static_cast<const SVGNumberListAndInfo*>(aEndVal.mU.mPtr);
  SVGNumberListAndInfo& result =
    *static_cast<SVGNumberListAndInfo*>(aResult.mU.mPtr);

  NS_ABORT_IF_FALSE(end.Element(), "Can't propagate target element");
  NS_ABORT_IF_FALSE(start.Element() == end.Element() || !start.Element(),
                    "Different target elements");

  if (start.Element() && 
      start.Length() != end.Length()) {
    
    
    return NS_ERROR_FAILURE;
  }
  if (!result.SetLength(end.Length())) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  result.SetInfo(end.Element()); 

  if (start.Length() != end.Length()) {
    NS_ABORT_IF_FALSE(start.Length() == 0, "Not an identity value");
    for (PRUint32 i = 0; i < end.Length(); ++i) {
      result[i] = aUnitDistance * end[i];
    }
    return NS_OK;
  }
  for (PRUint32 i = 0; i < end.Length(); ++i) {
    result[i] = start[i] + (end[i] - start[i]) * aUnitDistance;
  }
  return NS_OK;
}

} 
