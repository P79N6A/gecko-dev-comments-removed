



































#include "SVGPathSegListSMILType.h"
#include "nsSMILValue.h"
#include "SVGPathData.h"
#include <math.h>

using namespace mozilla;

 SVGPathSegListSMILType SVGPathSegListSMILType::sSingleton;




void
SVGPathSegListSMILType::Init(nsSMILValue &aValue) const
{
  NS_ABORT_IF_FALSE(aValue.IsNull(), "Unexpected value type");
  aValue.mU.mPtr = new SVGPathDataAndOwner();
  aValue.mType = this;
}

void
SVGPathSegListSMILType::Destroy(nsSMILValue& aValue) const
{
  NS_PRECONDITION(aValue.mType == this, "Unexpected SMIL value type");
  delete static_cast<SVGPathDataAndOwner*>(aValue.mU.mPtr);
  aValue.mU.mPtr = nsnull;
  aValue.mType = &nsSMILNullType::sSingleton;
}

nsresult
SVGPathSegListSMILType::Assign(nsSMILValue& aDest,
                               const nsSMILValue& aSrc) const
{
  NS_PRECONDITION(aDest.mType == aSrc.mType, "Incompatible SMIL types");
  NS_PRECONDITION(aDest.mType == this, "Unexpected SMIL value");

  const SVGPathDataAndOwner* src =
    static_cast<const SVGPathDataAndOwner*>(aSrc.mU.mPtr);
  SVGPathDataAndOwner* dest =
    static_cast<SVGPathDataAndOwner*>(aDest.mU.mPtr);

  return dest->CopyFrom(*src);
}

PRBool
SVGPathSegListSMILType::IsEqual(const nsSMILValue& aLeft,
                                const nsSMILValue& aRight) const
{
  NS_PRECONDITION(aLeft.mType == aRight.mType, "Incompatible SMIL types");
  NS_PRECONDITION(aLeft.mType == this, "Unexpected type for SMIL value");

  return *static_cast<const SVGPathDataAndOwner*>(aLeft.mU.mPtr) ==
         *static_cast<const SVGPathDataAndOwner*>(aRight.mU.mPtr);
}

nsresult
SVGPathSegListSMILType::Add(nsSMILValue& aDest,
                            const nsSMILValue& aValueToAdd,
                            PRUint32 aCount) const
{
  NS_PRECONDITION(aDest.mType == this, "Unexpected SMIL type");
  NS_PRECONDITION(aValueToAdd.mType == this, "Incompatible SMIL type");

  SVGPathDataAndOwner& dest =
    *static_cast<SVGPathDataAndOwner*>(aDest.mU.mPtr);
  const SVGPathDataAndOwner& valueToAdd =
    *static_cast<const SVGPathDataAndOwner*>(aValueToAdd.mU.mPtr);

  if (dest.Length() != valueToAdd.Length()) {
    
    if (dest.Length() == 0) {
      return dest.CopyFrom(valueToAdd);
    }
    
    
    
    return NS_ERROR_FAILURE;
  }

  PRUint32 i = 0;
  while (i < dest.Length()) {
    PRUint32 type = SVGPathSegUtils::DecodeType(dest[i]);
    if (type != SVGPathSegUtils::DecodeType(valueToAdd[i])) {
      
      
      
      return NS_ERROR_FAILURE;
    }
    i++;
    if ((type == nsIDOMSVGPathSeg::PATHSEG_ARC_ABS ||
         type == nsIDOMSVGPathSeg::PATHSEG_ARC_REL) &&
        (dest[i+3] != valueToAdd[i+3] || dest[i+4] != valueToAdd[i+4])) {
      
      return NS_ERROR_FAILURE;
    }
    PRUint32 segEnd = i + SVGPathSegUtils::ArgCountForType(type);
    for (; i < segEnd; ++i) {
      dest[i] += valueToAdd[i];
    }
  }

  NS_ABORT_IF_FALSE(i == dest.Length(), "Very, very bad - path data corrupt");

  
  
  return NS_OK;
}

nsresult
SVGPathSegListSMILType::ComputeDistance(const nsSMILValue& aFrom,
                                        const nsSMILValue& aTo,
                                        double& aDistance) const
{
  NS_PRECONDITION(aFrom.mType == this, "Unexpected SMIL type");
  NS_PRECONDITION(aTo.mType == this, "Incompatible SMIL type");

  

  
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
SVGPathSegListSMILType::Interpolate(const nsSMILValue& aStartVal,
                                    const nsSMILValue& aEndVal,
                                    double aUnitDistance,
                                    nsSMILValue& aResult) const
{
  NS_PRECONDITION(aStartVal.mType == aEndVal.mType,
                  "Trying to interpolate different types");
  NS_PRECONDITION(aStartVal.mType == this,
                  "Unexpected types for interpolation");
  NS_PRECONDITION(aResult.mType == this, "Unexpected result type");

  const SVGPathDataAndOwner& start =
    *static_cast<const SVGPathDataAndOwner*>(aStartVal.mU.mPtr);
  const SVGPathDataAndOwner& end =
    *static_cast<const SVGPathDataAndOwner*>(aEndVal.mU.mPtr);
  SVGPathDataAndOwner& result =
    *static_cast<SVGPathDataAndOwner*>(aResult.mU.mPtr);

  if (start.Length() != end.Length() && start.Length() != 0) {
    
    
    
    return NS_ERROR_FAILURE;
  }

  if (!result.SetLength(end.Length())) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  PRUint32 i = 0;

  if (start.Length() == 0) { 
    while (i < end.Length()) {
      PRUint32 type = SVGPathSegUtils::DecodeType(end[i]);
      result[i] = end[i];
      i++;
      PRUint32 segEnd = i + SVGPathSegUtils::ArgCountForType(type);
      if ((type == nsIDOMSVGPathSeg::PATHSEG_ARC_ABS ||
           type == nsIDOMSVGPathSeg::PATHSEG_ARC_REL)) {
        result[i] = end[i] * aUnitDistance;
        result[i+1] = end[i+1] * aUnitDistance;
        result[i+2] = end[i+2] * aUnitDistance;
        
        result[i+3] = end[i+3];
        result[i+4] = end[i+4];
        result[i+5] = end[i+5] * aUnitDistance;
        result[i+6] = end[i+6] * aUnitDistance;
        i = segEnd;
      } else {
        for (; i < segEnd; ++i) {
          result[i] = end[i] * aUnitDistance;
        }
      }
    }
  } else {
    while (i < end.Length()) {
      PRUint32 type = SVGPathSegUtils::DecodeType(end[i]);
      if (type != SVGPathSegUtils::DecodeType(start[i])) {
        
        
        
        return NS_ERROR_FAILURE;
      }
      result[i] = end[i];
      i++;
      if ((type == nsIDOMSVGPathSeg::PATHSEG_ARC_ABS ||
           type == nsIDOMSVGPathSeg::PATHSEG_ARC_REL) &&
          (start[i+3] != end[i+3] || start[i+4] != end[i+4])) {
        
        return NS_ERROR_FAILURE;
      }
      PRUint32 segEnd = i + SVGPathSegUtils::ArgCountForType(type);
      for (; i < segEnd; ++i) {
        result[i] = start[i] + (end[i] - start[i]) * aUnitDistance;
      }
    }
  }

  NS_ABORT_IF_FALSE(i == end.Length(), "Very, very bad - path data corrupt");

  return NS_OK;
}

