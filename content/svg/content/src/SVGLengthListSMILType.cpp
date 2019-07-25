



































#include "SVGLengthListSMILType.h"
#include "nsSMILValue.h"
#include "SVGLengthList.h"
#include <math.h>

namespace mozilla {

 SVGLengthListSMILType SVGLengthListSMILType::sSingleton;




void
SVGLengthListSMILType::Init(nsSMILValue &aValue) const
{
  NS_ABORT_IF_FALSE(aValue.IsNull(), "Unexpected value type");

  SVGLengthListAndInfo* lengthList = new SVGLengthListAndInfo();

  
  lengthList->SetCanZeroPadList(PR_TRUE);

  aValue.mU.mPtr = lengthList;
  aValue.mType = this;
}

void
SVGLengthListSMILType::Destroy(nsSMILValue& aValue) const
{
  NS_PRECONDITION(aValue.mType == this, "Unexpected SMIL value type");
  delete static_cast<SVGLengthListAndInfo*>(aValue.mU.mPtr);
  aValue.mU.mPtr = nsnull;
  aValue.mType = &nsSMILNullType::sSingleton;
}

nsresult
SVGLengthListSMILType::Assign(nsSMILValue& aDest,
                              const nsSMILValue& aSrc) const
{
  NS_PRECONDITION(aDest.mType == aSrc.mType, "Incompatible SMIL types");
  NS_PRECONDITION(aDest.mType == this, "Unexpected SMIL value");

  const SVGLengthListAndInfo* src =
    static_cast<const SVGLengthListAndInfo*>(aSrc.mU.mPtr);
  SVGLengthListAndInfo* dest =
    static_cast<SVGLengthListAndInfo*>(aDest.mU.mPtr);

  return dest->CopyFrom(*src);
}

PRBool
SVGLengthListSMILType::IsEqual(const nsSMILValue& aLeft,
                               const nsSMILValue& aRight) const
{
  NS_PRECONDITION(aLeft.mType == aRight.mType, "Incompatible SMIL types");
  NS_PRECONDITION(aLeft.mType == this, "Unexpected type for SMIL value");

  return *static_cast<const SVGLengthListAndInfo*>(aLeft.mU.mPtr) ==
         *static_cast<const SVGLengthListAndInfo*>(aRight.mU.mPtr);
}

nsresult
SVGLengthListSMILType::Add(nsSMILValue& aDest,
                           const nsSMILValue& aValueToAdd,
                           PRUint32 aCount) const
{
  NS_PRECONDITION(aDest.mType == this, "Unexpected SMIL type");
  NS_PRECONDITION(aValueToAdd.mType == this, "Incompatible SMIL type");

  SVGLengthListAndInfo& dest =
    *static_cast<SVGLengthListAndInfo*>(aDest.mU.mPtr);
  const SVGLengthListAndInfo& valueToAdd =
    *static_cast<const SVGLengthListAndInfo*>(aValueToAdd.mU.mPtr);

  
  

  
  
  
  
  
  
  
  
  
  

  if (dest.Length() < valueToAdd.Length()) {
    if (!dest.CanZeroPadList()) {
      
      return NS_ERROR_FAILURE;
    }

    NS_ASSERTION(valueToAdd.CanZeroPadList() || dest.Length() == 0,
                 "Only \"zero\" nsSMILValues from the SMIL engine should "
                 "return PR_TRUE for CanZeroPadList() when the attribute "
                 "being animated can't be zero padded");

    PRUint32 i = dest.Length();
    if (!dest.SetLength(valueToAdd.Length())) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    for (; i < valueToAdd.Length(); ++i) {
      dest[i].SetValueAndUnit(0.0f, valueToAdd[i].GetUnit());
    }
  }

  for (PRUint32 i = 0; i < valueToAdd.Length(); ++i) {
    float valToAdd;
    if (dest[i].GetUnit() == valueToAdd[i].GetUnit()) {
      valToAdd = valueToAdd[i].GetValueInCurrentUnits();
    } else {
      
      
      valToAdd = valueToAdd[i].GetValueInSpecifiedUnit(dest[i].GetUnit(),
                                                       dest.Element(),
                                                       dest.Axis());
    }
    dest[i].SetValueAndUnit(dest[i].GetValueInCurrentUnits() + valToAdd,
                            dest[i].GetUnit());
  }

  
  dest.SetCanZeroPadList(dest.CanZeroPadList() &&
                         valueToAdd.CanZeroPadList());

  return NS_OK;
}

nsresult
SVGLengthListSMILType::ComputeDistance(const nsSMILValue& aFrom,
                                       const nsSMILValue& aTo,
                                       double& aDistance) const
{
  NS_PRECONDITION(aFrom.mType == this, "Unexpected SMIL type");
  NS_PRECONDITION(aTo.mType == this, "Incompatible SMIL type");

  const SVGLengthListAndInfo& from =
    *static_cast<const SVGLengthListAndInfo*>(aFrom.mU.mPtr);
  const SVGLengthListAndInfo& to =
    *static_cast<const SVGLengthListAndInfo*>(aTo.mU.mPtr);

  
  

  NS_ASSERTION((from.CanZeroPadList() == to.CanZeroPadList()) ||
               (from.CanZeroPadList() && from.IsEmpty()) ||
               (to.CanZeroPadList() && to.IsEmpty()),
               "Only \"zero\" nsSMILValues from the SMIL engine should "
               "return PR_TRUE for CanZeroPadList() when the attribute "
               "being animated can't be zero padded");

  if ((from.Length() < to.Length() && !from.CanZeroPadList()) ||
      (to.Length() < from.Length() && !to.CanZeroPadList())) {
    
    return NS_ERROR_FAILURE;
  }

  
  
  
  
  
  
  
  
  

  double total = 0.0;

  PRUint32 i = 0;
  for (; i < from.Length() && i < to.Length(); ++i) {
    double f = from[i].GetValueInUserUnits(from.Element(), from.Axis());
    double t = to[i].GetValueInUserUnits(to.Element(), to.Axis());
    double delta = t - f;
    total += delta * delta;
  }

  
  

  for (; i < from.Length(); ++i) {
    double f = from[i].GetValueInUserUnits(from.Element(), from.Axis());
    total += f * f;
  }
  for (; i < to.Length(); ++i) {
    double t = to[i].GetValueInUserUnits(to.Element(), to.Axis());
    total += t * t;
  }

  float distance = sqrt(total);
  if (!NS_FloatIsFinite(distance)) {
    return NS_ERROR_FAILURE;
  }
  aDistance = distance;
  return NS_OK;
}

nsresult
SVGLengthListSMILType::Interpolate(const nsSMILValue& aStartVal,
                                   const nsSMILValue& aEndVal,
                                   double aUnitDistance,
                                   nsSMILValue& aResult) const
{
  NS_PRECONDITION(aStartVal.mType == aEndVal.mType,
                  "Trying to interpolate different types");
  NS_PRECONDITION(aStartVal.mType == this,
                  "Unexpected types for interpolation");
  NS_PRECONDITION(aResult.mType == this, "Unexpected result type");

  const SVGLengthListAndInfo& start =
    *static_cast<const SVGLengthListAndInfo*>(aStartVal.mU.mPtr);
  const SVGLengthListAndInfo& end =
    *static_cast<const SVGLengthListAndInfo*>(aEndVal.mU.mPtr);
  SVGLengthListAndInfo& result =
    *static_cast<SVGLengthListAndInfo*>(aResult.mU.mPtr);

  
  

  NS_ASSERTION((start.CanZeroPadList() == end.CanZeroPadList()) ||
               (start.CanZeroPadList() && start.IsEmpty()) ||
               (end.CanZeroPadList() && end.IsEmpty()),
               "Only \"zero\" nsSMILValues from the SMIL engine should "
               "return PR_TRUE for CanZeroPadList() when the attribute "
               "being animated can't be zero padded");

  if ((start.Length() < end.Length() && !start.CanZeroPadList()) ||
      (end.Length() < start.Length() && !end.CanZeroPadList())) {
    
    return NS_ERROR_FAILURE;
  }

  if (!result.SetLength(NS_MAX(start.Length(), end.Length()))) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  PRUint32 i = 0;
  for (; i < start.Length() && i < end.Length(); ++i) {
    float s;
    if (start[i].GetUnit() == end[i].GetUnit()) {
      s = start[i].GetValueInCurrentUnits();
    } else {
      
      
      s = start[i].GetValueInSpecifiedUnit(end[i].GetUnit(), end.Element(), end.Axis());
    }
    float e = end[i].GetValueInCurrentUnits();
    result[i].SetValueAndUnit(s + (e - s) * aUnitDistance, end[i].GetUnit());
  }

  
  

  for (; i < start.Length(); ++i) {
    result[i].SetValueAndUnit(start[i].GetValueInCurrentUnits() -
                              start[i].GetValueInCurrentUnits() * aUnitDistance,
                              start[i].GetUnit());
  }
  for (; i < end.Length(); ++i) {
    result[i].SetValueAndUnit(end[i].GetValueInCurrentUnits() * aUnitDistance,
                              end[i].GetUnit());
  }

  
  result.SetCanZeroPadList(start.CanZeroPadList() &&
                           end.CanZeroPadList());

  return NS_OK;
}

} 
