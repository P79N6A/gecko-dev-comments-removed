




































#include "nsSVGTransformSMILType.h"
#include "nsSMILValue.h"
#include "nsCRT.h"
#include <math.h>

 nsSVGTransformSMILType nsSVGTransformSMILType::sSingleton;

typedef nsTArray<nsSVGSMILTransform> TransformArray;




void
nsSVGTransformSMILType::Init(nsSMILValue &aValue) const
{
  NS_PRECONDITION(aValue.IsNull(), "Unexpected value type");

  TransformArray* transforms = new TransformArray(1);
  aValue.mU.mPtr = transforms;
  aValue.mType = this;
}

void
nsSVGTransformSMILType::Destroy(nsSMILValue& aValue) const
{
  NS_PRECONDITION(aValue.mType == this, "Unexpected SMIL value type");
  TransformArray* params = static_cast<TransformArray*>(aValue.mU.mPtr);
  delete params;
  aValue.mU.mPtr = nsnull;
  aValue.mType = &nsSMILNullType::sSingleton;
}

nsresult
nsSVGTransformSMILType::Assign(nsSMILValue& aDest,
                               const nsSMILValue& aSrc) const
{
  NS_PRECONDITION(aDest.mType == aSrc.mType, "Incompatible SMIL types");
  NS_PRECONDITION(aDest.mType == this, "Unexpected SMIL value");

  const TransformArray* srcTransforms =
    static_cast<const TransformArray*>(aSrc.mU.mPtr);
  TransformArray* dstTransforms = static_cast<TransformArray*>(aDest.mU.mPtr);

  
  PRBool result = dstTransforms->SetCapacity(srcTransforms->Length());
  NS_ENSURE_TRUE(result,NS_ERROR_OUT_OF_MEMORY);

  *dstTransforms = *srcTransforms;

  return NS_OK;
}

PRBool
nsSVGTransformSMILType::IsEqual(const nsSMILValue& aLeft,
                                const nsSMILValue& aRight) const
{
  NS_PRECONDITION(aLeft.mType == aRight.mType, "Incompatible SMIL types");
  NS_PRECONDITION(aLeft.mType == this, "Unexpected SMIL type");

  const TransformArray& leftArr
    (*static_cast<const TransformArray*>(aLeft.mU.mPtr));
  const TransformArray& rightArr
    (*static_cast<const TransformArray*>(aRight.mU.mPtr));

  
  if (leftArr.Length() != rightArr.Length()) {
    return PR_FALSE;
  }

  
  PRUint32 length = leftArr.Length(); 
  for (PRUint32 i = 0; i < length; ++i) {
    if (leftArr[i] != rightArr[i]) {
      return PR_FALSE;
    }
  }
  
  
  return PR_TRUE;
}

nsresult
nsSVGTransformSMILType::Add(nsSMILValue& aDest, const nsSMILValue& aValueToAdd,
                            PRUint32 aCount) const
{
  NS_PRECONDITION(aDest.mType == this, "Unexpected SMIL type");
  NS_PRECONDITION(aDest.mType == aValueToAdd.mType, "Incompatible SMIL types");

  TransformArray& dstTransforms(*static_cast<TransformArray*>(aDest.mU.mPtr));
  const TransformArray& srcTransforms
    (*static_cast<const TransformArray*>(aValueToAdd.mU.mPtr));

  
  
  
  
  
  NS_ASSERTION(srcTransforms.Length() == 1,
    "Invalid source transform list to add");

  
  
  
  
  NS_ASSERTION(dstTransforms.Length() < 2,
    "Invalid dest transform list to add to");

  
  const nsSVGSMILTransform& srcTransform = srcTransforms[0];
  if (dstTransforms.IsEmpty()) {
    nsSVGSMILTransform* result = dstTransforms.AppendElement(
      nsSVGSMILTransform(srcTransform.mTransformType));
    NS_ENSURE_TRUE(result,NS_ERROR_OUT_OF_MEMORY);
  }
  nsSVGSMILTransform& dstTransform = dstTransforms[0];

  
  NS_ASSERTION(srcTransform.mTransformType == dstTransform.mTransformType,
    "Trying to perform simple add of different transform types");

  
  NS_ASSERTION(
    srcTransform.mTransformType != nsSVGSMILTransform::TRANSFORM_MATRIX,
    "Trying to perform simple add with matrix transform");

  
  for (int i = 0; i <= 2; ++i) {
    dstTransform.mParams[i] += srcTransform.mParams[i] * aCount;
  }

  return NS_OK;
}

nsresult
nsSVGTransformSMILType::SandwichAdd(nsSMILValue& aDest,
                                    const nsSMILValue& aValueToAdd) const
{
  NS_PRECONDITION(aDest.mType == this, "Unexpected SMIL type");
  NS_PRECONDITION(aDest.mType == aValueToAdd.mType, "Incompatible SMIL types");

  
  

  TransformArray& dstTransforms(*static_cast<TransformArray*>(aDest.mU.mPtr));
  const TransformArray& srcTransforms
    (*static_cast<const TransformArray*>(aValueToAdd.mU.mPtr));

  
  NS_ASSERTION(srcTransforms.Length() < 2,
    "Trying to do sandwich add of more than one value");

  
  
  
  
  
  
  
  if (srcTransforms.IsEmpty())
    return NS_OK;

  
  const nsSVGSMILTransform& srcTransform = srcTransforms[0];
  nsSVGSMILTransform* result = dstTransforms.AppendElement(srcTransform);
  NS_ENSURE_TRUE(result,NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}

nsresult
nsSVGTransformSMILType::ComputeDistance(const nsSMILValue& aFrom,
                                        const nsSMILValue& aTo,
                                        double& aDistance) const
{
  NS_PRECONDITION(aFrom.mType == aTo.mType,
      "Can't compute difference between different SMIL types");
  NS_PRECONDITION(aFrom.mType == this, "Unexpected SMIL type");

  const TransformArray* fromTransforms =
    static_cast<const TransformArray*>(aFrom.mU.mPtr);
  const TransformArray* toTransforms =
    static_cast<const TransformArray*>(aTo.mU.mPtr);

  
  
  
  
  
  NS_ASSERTION(fromTransforms->Length() == 1,
    "Wrong number of elements in from value");
  NS_ASSERTION(toTransforms->Length() == 1,
    "Wrong number of elements in to value");

  const nsSVGSMILTransform& fromTransform = (*fromTransforms)[0];
  const nsSVGSMILTransform& toTransform = (*toTransforms)[0];
  NS_ASSERTION(fromTransform.mTransformType == toTransform.mTransformType,
    "Incompatible transform types to calculate distance between");

  switch (fromTransform.mTransformType)
  {
    
    
    
    case nsSVGSMILTransform::TRANSFORM_TRANSLATE:
    case nsSVGSMILTransform::TRANSFORM_SCALE:
      {
        const float& a_tx = fromTransform.mParams[0];
        const float& a_ty = fromTransform.mParams[1];
        const float& b_tx = toTransform.mParams[0];
        const float& b_ty = toTransform.mParams[1];
        aDistance = sqrt(pow(a_tx - b_tx, 2) + (pow(a_ty - b_ty, 2)));
      }
      break;

    case nsSVGSMILTransform::TRANSFORM_ROTATE:
    case nsSVGSMILTransform::TRANSFORM_SKEWX:
    case nsSVGSMILTransform::TRANSFORM_SKEWY:
      {
        const float& a = fromTransform.mParams[0];
        const float& b = toTransform.mParams[0];
        aDistance = fabs(a-b);
      }
      break;

    default:
      NS_ERROR("Got bad transform types for calculating distances");
      aDistance = 1.0;
      return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

nsresult
nsSVGTransformSMILType::Interpolate(const nsSMILValue& aStartVal,
                                    const nsSMILValue& aEndVal,
                                    double aUnitDistance,
                                    nsSMILValue& aResult) const
{
  NS_PRECONDITION(aStartVal.mType == aEndVal.mType,
      "Can't interpolate between different SMIL types");
  NS_PRECONDITION(aStartVal.mType == this,
      "Unexpected type for interpolation");
  NS_PRECONDITION(aResult.mType == this, "Unexpected result type");

  const TransformArray& startTransforms =
    (*static_cast<const TransformArray*>(aStartVal.mU.mPtr));
  const TransformArray& endTransforms
    (*static_cast<const TransformArray*>(aEndVal.mU.mPtr));

  
  
  NS_ASSERTION(endTransforms.Length() == 1,
    "Invalid end-point for interpolating between transform values");

  
  const nsSVGSMILTransform& endTransform = endTransforms[0];
  NS_ASSERTION(
    endTransform.mTransformType != nsSVGSMILTransform::TRANSFORM_MATRIX,
    "End point for interpolation should not be a matrix transform");

  
  
  
  
  
  static float identityParams[3] = { 0.f };
  const float* startParams = nsnull;
  if (startTransforms.Length() == 1) {
    const nsSVGSMILTransform& startTransform = startTransforms[0];
    if (startTransform.mTransformType == endTransform.mTransformType) {
      startParams = startTransform.mParams;
    }
  }
  if (!startParams) {
    startParams = identityParams;
  }

  const float* endParams = endTransform.mParams;

  
  float newParams[3];
  for (int i = 0; i <= 2; ++i) {
    const float& a = startParams[i];
    const float& b = endParams[i];
    newParams[i] = a + (b - a) * aUnitDistance;
  }

  
  nsSVGSMILTransform resultTransform(endTransform.mTransformType, newParams);

  
  TransformArray& dstTransforms =
    (*static_cast<TransformArray*>(aResult.mU.mPtr));
  dstTransforms.Clear();

  
  nsSVGSMILTransform* transform = dstTransforms.AppendElement(resultTransform);
  NS_ENSURE_TRUE(transform,NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}





PRUint32
nsSVGTransformSMILType::GetNumTransforms(const nsSMILValue& aValue)
{
  NS_PRECONDITION(aValue.mType == &sSingleton, "Unexpected SMIL value type");

  const TransformArray& transforms =
    *static_cast<const TransformArray*>(aValue.mU.mPtr);

  return transforms.Length();
}


const nsSVGSMILTransform*
nsSVGTransformSMILType::GetTransformAt(PRUint32 aIndex,
                                       const nsSMILValue& aValue)
{
  NS_PRECONDITION(aValue.mType == &sSingleton, "Unexpected SMIL value type");

  const TransformArray& transforms =
    *static_cast<const TransformArray*>(aValue.mU.mPtr);

  if (aIndex >= transforms.Length()) {
    NS_ERROR("Attempting to access invalid transform");
    return nsnull;
  }

  return &transforms[aIndex];
}


nsresult
nsSVGTransformSMILType::AppendTransform(const nsSVGSMILTransform& aTransform,
                                        nsSMILValue& aValue)
{
  NS_PRECONDITION(aValue.mType == &sSingleton, "Unexpected SMIL value type");

  TransformArray& transforms = *static_cast<TransformArray*>(aValue.mU.mPtr);
  return transforms.AppendElement(aTransform) ?
    NS_OK : NS_ERROR_OUT_OF_MEMORY;
}
