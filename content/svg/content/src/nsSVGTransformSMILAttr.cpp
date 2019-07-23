




































#include "nsSVGTransformSMILAttr.h"
#include "nsSVGTransformSMILType.h"
#include "nsSVGAnimatedTransformList.h"
#include "nsSVGTransformList.h"
#include "nsSVGTransform.h"
#include "nsIDOMSVGTransform.h"
#include "nsIDOMSVGMatrix.h"
#include "nsSVGMatrix.h"
#include "nsSMILValue.h"
#include "nsSMILNullType.h"
#include "nsISMILAnimationElement.h"
#include "nsSVGElement.h"
#include "nsISVGValue.h"
#include "prdtoa.h"

nsISMILType*
nsSVGTransformSMILAttr::GetSMILType() const
{
  return &nsSVGTransformSMILType::sSingleton;
}

nsresult
nsSVGTransformSMILAttr::ValueFromString(const nsAString& aStr,
                                     const nsISMILAnimationElement* aSrcElement,
                                     nsSMILValue& aValue) const
{
  NS_ENSURE_TRUE(aSrcElement, NS_ERROR_FAILURE);
  NS_ASSERTION(aValue.IsNull(),
    "aValue should have been cleared before calling ValueFromString.");

  nsSMILValue val(&nsSVGTransformSMILType::sSingleton);
  if (val.IsNull())
    return NS_ERROR_FAILURE;

  const nsAttrValue* typeAttr = aSrcElement->GetAnimAttr(nsGkAtoms::type);

  const nsIAtom* transformType = typeAttr
                               ? typeAttr->GetAtomValue()
                               : nsGkAtoms::translate;

  nsresult rv = ParseValue(aStr, transformType, val);
  if (NS_FAILED(rv))
    return rv;

  aValue = val;

  return NS_OK;
}

nsSMILValue
nsSVGTransformSMILAttr::GetBaseValue() const
{
  nsSVGTransformSMILType *type = &nsSVGTransformSMILType::sSingleton;
  nsSMILValue val(type);
  if (val.IsNull())
    return val;
 
  nsIDOMSVGTransformList *list = mVal->mBaseVal.get();

  PRUint32 numItems = 0;
  list->GetNumberOfItems(&numItems);
  for (PRUint32 i = 0; i < numItems; i++) {
    nsCOMPtr<nsIDOMSVGTransform> transform;
    nsresult rv = list->GetItem(i, getter_AddRefs(transform));
    if (NS_SUCCEEDED(rv) && transform) {
      rv = AppendSVGTransformToSMILValue(transform.get(), val);
      NS_ENSURE_SUCCESS(rv,nsSMILValue());
    }
  }
  
  return val;
}

void
nsSVGTransformSMILAttr::ClearAnimValue()
{
  mVal->WillModify(nsISVGValue::mod_other);
  mVal->mAnimVal = nsnull;
  mVal->DidModify(nsISVGValue::mod_other);
}

nsresult
nsSVGTransformSMILAttr::SetAnimValue(const nsSMILValue& aValue)
{
  if (aValue.mType != &nsSVGTransformSMILType::sSingleton) {
    NS_WARNING("Unexpected SMIL Type");
    return NS_ERROR_FAILURE;
  }

  nsresult rv = NS_OK;

  
  mVal->WillModify(nsISVGValue::mod_other);
  if (!mVal->mAnimVal) {
    rv = nsSVGTransformList::Create(getter_AddRefs(mVal->mAnimVal));
    NS_ENSURE_SUCCESS(rv,rv);
  }

  
  
  
  rv = UpdateFromSMILValue(mVal->mAnimVal, aValue);
  if (NS_FAILED(rv)) {
    mVal->mAnimVal = nsnull;
  }
  NS_ENSURE_SUCCESS(rv,rv);

  mVal->DidModify(nsISVGValue::mod_other);
  return NS_OK;
}




nsresult
nsSVGTransformSMILAttr::ParseValue(const nsAString& aSpec,
                                   const nsIAtom* aTransformType,
                                   nsSMILValue& aResult) const
{
  nsSVGTransformSMILType* type = &nsSVGTransformSMILType::sSingleton;
  NS_ASSERTION(
      type == static_cast<nsSVGTransformSMILType const *>(aResult.mType),
      "Unexpected type for SMIL value result.");

  
  nsresult rv = type->Init(aResult);
  NS_ENSURE_SUCCESS(rv,rv);

  float params[3] = { 0.f };
  PRInt32 numParsed = ParseParameterList(aSpec, params, 3);
  nsSVGSMILTransform::TransformType transformType;
   
  if (aTransformType == nsGkAtoms::translate) {
    
    if (numParsed != 1 && numParsed != 2)
      return NS_ERROR_FAILURE;
    transformType = nsSVGSMILTransform::TRANSFORM_TRANSLATE;
  } else if (aTransformType == nsGkAtoms::scale) {
    
    if (numParsed != 1 && numParsed != 2)
      return NS_ERROR_FAILURE;
    if (numParsed == 1) {
      params[1] = params[0];
    }
    transformType = nsSVGSMILTransform::TRANSFORM_SCALE;
  } else if (aTransformType == nsGkAtoms::rotate) {
    
    if (numParsed != 1 && numParsed != 3)
      return NS_ERROR_FAILURE;
    transformType = nsSVGSMILTransform::TRANSFORM_ROTATE;
  } else if (aTransformType == nsGkAtoms::skewX) {
    
    if (numParsed != 1)
      return NS_ERROR_FAILURE;
    transformType = nsSVGSMILTransform::TRANSFORM_SKEWX;
  } else if (aTransformType == nsGkAtoms::skewY) {
    
    if (numParsed != 1)
      return NS_ERROR_FAILURE;
    transformType = nsSVGSMILTransform::TRANSFORM_SKEWY;
  } else {
    return NS_ERROR_FAILURE;
  }

  return type->AppendTransform(nsSVGSMILTransform(transformType, params),
                               aResult);
}

inline PRBool
nsSVGTransformSMILAttr::IsSpace(const char c) const
{
  return (c == 0x9 || c == 0xA || c == 0xD || c == 0x20);
}

inline void
nsSVGTransformSMILAttr::SkipWsp(nsACString::const_iterator& aIter,
                               const nsACString::const_iterator& aIterEnd) const
{
  while (aIter != aIterEnd && IsSpace(*aIter))
    ++aIter;
}

PRInt32
nsSVGTransformSMILAttr::ParseParameterList(const nsAString& aSpec,
                                           float* aVars,
                                           PRInt32 aNVars) const
{
  NS_ConvertUTF16toUTF8 spec(aSpec);

  nsACString::const_iterator start, end;
  spec.BeginReading(start);
  spec.EndReading(end);

  SkipWsp(start, end);

  int numArgsFound = 0;

  while (start != end) {
    char const *arg = start.get();
    char *argend;
    float f = float(PR_strtod(arg, &argend));
    if (arg == argend || argend > end.get() || !NS_FloatIsFinite(f))
      return -1;

    if (numArgsFound < aNVars) {
      aVars[numArgsFound] = f;
    }

    start.advance(argend - arg);
    numArgsFound++;

    SkipWsp(start, end);
    if (*start == ',') {
      ++start;
      SkipWsp(start, end);
    }
  }

  return numArgsFound;
}

nsresult
nsSVGTransformSMILAttr::AppendSVGTransformToSMILValue(
  nsIDOMSVGTransform* aTransform, nsSMILValue& aValue) const
{
  nsSVGTransformSMILType* type = &nsSVGTransformSMILType::sSingleton;

  PRUint16 svgTransformType = nsIDOMSVGTransform::SVG_TRANSFORM_MATRIX;
  aTransform->GetType(&svgTransformType);

  nsCOMPtr<nsIDOMSVGMatrix> matrix;
  nsresult rv = aTransform->GetMatrix(getter_AddRefs(matrix));
  if (NS_FAILED(rv) || !matrix)
    return NS_ERROR_FAILURE;

  float params[3] = { 0.f };
  nsSVGSMILTransform::TransformType transformType;

  switch (svgTransformType)
  {
    case nsIDOMSVGTransform::SVG_TRANSFORM_TRANSLATE:
      {
        matrix->GetE(&params[0]);
        matrix->GetF(&params[1]);
        transformType = nsSVGSMILTransform::TRANSFORM_TRANSLATE;
      }
      break;

    case nsIDOMSVGTransform::SVG_TRANSFORM_SCALE:
      {
        matrix->GetA(&params[0]);
        matrix->GetD(&params[1]);
        transformType = nsSVGSMILTransform::TRANSFORM_SCALE;
      }
      break;

    case nsIDOMSVGTransform::SVG_TRANSFORM_ROTATE:
      {
        




        nsSVGTransform* svgTransform = static_cast<nsSVGTransform*>(aTransform);
        svgTransform->GetAngle(&params[0]);
        svgTransform->GetRotationOrigin(params[1], params[2]);
        transformType = nsSVGSMILTransform::TRANSFORM_ROTATE;
      }
      break;

    case nsIDOMSVGTransform::SVG_TRANSFORM_SKEWX:
      {
        aTransform->GetAngle(&params[0]);
        transformType = nsSVGSMILTransform::TRANSFORM_SKEWX;
      }
      break;

    case nsIDOMSVGTransform::SVG_TRANSFORM_SKEWY:
      {
        aTransform->GetAngle(&params[0]);
        transformType = nsSVGSMILTransform::TRANSFORM_SKEWY;
      }
      break;

    case nsIDOMSVGTransform::SVG_TRANSFORM_MATRIX:
      {
        float mx[6];
        matrix->GetA(&mx[0]);
        matrix->GetB(&mx[1]);
        matrix->GetC(&mx[2]);
        matrix->GetD(&mx[3]);
        matrix->GetE(&mx[4]);
        matrix->GetF(&mx[5]);
        rv = type->AppendTransform(nsSVGSMILTransform(mx), aValue);
        transformType = nsSVGSMILTransform::TRANSFORM_MATRIX;
      }
      break;

    case nsIDOMSVGTransform::SVG_TRANSFORM_UNKNOWN:
      
      return NS_OK;

    default:
      NS_WARNING("Trying to convert unrecognised SVG transform type.");
      return NS_ERROR_FAILURE;
  }

  if (transformType != nsSVGSMILTransform::TRANSFORM_MATRIX) {
    rv = 
      type->AppendTransform(nsSVGSMILTransform(transformType, params), aValue);
  }

  return rv;
}

nsresult
nsSVGTransformSMILAttr::UpdateFromSMILValue(
  nsIDOMSVGTransformList* aTransformList, const nsSMILValue& aValue)
{
  PRUint32 svgLength = -1;
  aTransformList->GetNumberOfItems(&svgLength);

  nsSVGTransformSMILType* type = &nsSVGTransformSMILType::sSingleton;
  PRUint32 smilLength = type->GetNumTransforms(aValue);

  nsresult rv = NS_OK;

  for (PRUint32 i = 0; i < smilLength; i++) {
    nsCOMPtr<nsIDOMSVGTransform> transform;
    if (i < svgLength) {
      
      rv = aTransformList->GetItem(i, getter_AddRefs(transform));
      NS_ENSURE_SUCCESS(rv,rv);
    } else {
      
      nsresult rv = NS_NewSVGTransform(getter_AddRefs(transform));
      NS_ENSURE_SUCCESS(rv,rv);

      nsCOMPtr<nsIDOMSVGTransform> result;
      rv = aTransformList->AppendItem(transform, getter_AddRefs(result));
      NS_ENSURE_SUCCESS(rv,rv);
    }
    
    const nsSVGSMILTransform* smilTransform = type->GetTransformAt(i, aValue);
    rv = GetSVGTransformFromSMILValue(*smilTransform, transform);
    NS_ENSURE_SUCCESS(rv,rv);
  }

  
  while (svgLength > smilLength) {
    nsCOMPtr<nsIDOMSVGTransform> removed;
    rv = aTransformList->RemoveItem(--svgLength, getter_AddRefs(removed));
    NS_ENSURE_SUCCESS(rv,rv);
  }

  return NS_OK;
}

nsresult
nsSVGTransformSMILAttr::GetSVGTransformFromSMILValue(
    const nsSVGSMILTransform& aSMILTransform,
    nsIDOMSVGTransform* aSVGTransform) const
{
  nsresult rv = NS_ERROR_FAILURE;

  switch (aSMILTransform.mTransformType)
  {
    case nsSVGSMILTransform::TRANSFORM_TRANSLATE:
      rv = aSVGTransform->SetTranslate(aSMILTransform.mParams[0],
                                       aSMILTransform.mParams[1]);
      break;

    case nsSVGSMILTransform::TRANSFORM_SCALE:
      rv = aSVGTransform->SetScale(aSMILTransform.mParams[0],
                                   aSMILTransform.mParams[1]);
      break;

    case nsSVGSMILTransform::TRANSFORM_ROTATE:
      rv = aSVGTransform->SetRotate(aSMILTransform.mParams[0],
                                    aSMILTransform.mParams[1],
                                    aSMILTransform.mParams[2]);
      break;

    case nsSVGSMILTransform::TRANSFORM_SKEWX:
      rv = aSVGTransform->SetSkewX(aSMILTransform.mParams[0]);
      break;

    case nsSVGSMILTransform::TRANSFORM_SKEWY:
      rv = aSVGTransform->SetSkewY(aSMILTransform.mParams[0]);
      break;

    case nsSVGSMILTransform::TRANSFORM_MATRIX:
      {
        nsCOMPtr<nsIDOMSVGMatrix> svgMatrix;
        rv = NS_NewSVGMatrix(getter_AddRefs(svgMatrix),
                             aSMILTransform.mParams[0],
                             aSMILTransform.mParams[1],
                             aSMILTransform.mParams[2],
                             aSMILTransform.mParams[3],
                             aSMILTransform.mParams[4],
                             aSMILTransform.mParams[5]);
        NS_ENSURE_SUCCESS(rv,rv);
        NS_ENSURE_TRUE(svgMatrix,NS_ERROR_FAILURE);
        rv = aSVGTransform->SetMatrix(svgMatrix);
      }
      break;
  }

  return rv;
}
