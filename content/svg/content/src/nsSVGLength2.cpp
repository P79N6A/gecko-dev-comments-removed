





































#include "nsSVGLength2.h"
#include "prdtoa.h"
#include "nsTextFormatter.h"
#include "nsSVGSVGElement.h"
#include "nsIFrame.h"
#include "nsSVGIntegrationUtils.h"
#ifdef MOZ_SMIL
#include "nsSMILValue.h"
#include "nsSMILFloatType.h"
#endif 

class DOMSVGLength : public nsIDOMSVGLength
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(DOMSVGLength)

  DOMSVGLength(nsSVGElement *aSVGElement)
    : mSVGElement(aSVGElement)
    { mVal.Init(); }
    
  NS_IMETHOD GetUnitType(PRUint16* aResult)
    { *aResult = mVal.mSpecifiedUnitType; return NS_OK; }

  NS_IMETHOD GetValue(float* aResult)
    { *aResult = mVal.GetBaseValue(mSVGElement); return NS_OK; }
  NS_IMETHOD SetValue(float aValue)
    { NS_ENSURE_FINITE(aValue, NS_ERROR_ILLEGAL_VALUE);
      mVal.mBaseVal = 
        aValue * mVal.GetUnitScaleFactor(mSVGElement, mVal.mSpecifiedUnitType);
      return NS_OK; }

  NS_IMETHOD GetValueInSpecifiedUnits(float* aResult)
    { *aResult = mVal.mBaseVal; return NS_OK; }
  NS_IMETHOD SetValueInSpecifiedUnits(float aValue)
    { NS_ENSURE_FINITE(aValue, NS_ERROR_ILLEGAL_VALUE);
      mVal.mBaseVal = aValue;
      return NS_OK; }

  NS_IMETHOD SetValueAsString(const nsAString& aValueAsString);
  NS_IMETHOD GetValueAsString(nsAString& aValue)
    { mVal.GetBaseValueString(aValue); return NS_OK; }

  NS_IMETHOD NewValueSpecifiedUnits(PRUint16 unitType,
                                    float valueInSpecifiedUnits);

  NS_IMETHOD ConvertToSpecifiedUnits(PRUint16 unitType);

private:
  nsSVGLength2 mVal;
  nsRefPtr<nsSVGElement> mSVGElement;
};

NS_SVG_VAL_IMPL_CYCLE_COLLECTION(nsSVGLength2::DOMBaseVal, mSVGElement)

NS_SVG_VAL_IMPL_CYCLE_COLLECTION(nsSVGLength2::DOMAnimVal, mSVGElement)

NS_SVG_VAL_IMPL_CYCLE_COLLECTION(nsSVGLength2::DOMAnimatedLength, mSVGElement)

NS_SVG_VAL_IMPL_CYCLE_COLLECTION(DOMSVGLength, mSVGElement)

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsSVGLength2::DOMBaseVal)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsSVGLength2::DOMBaseVal)

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsSVGLength2::DOMAnimVal)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsSVGLength2::DOMAnimVal)

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsSVGLength2::DOMAnimatedLength)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsSVGLength2::DOMAnimatedLength)

NS_IMPL_CYCLE_COLLECTING_ADDREF(DOMSVGLength)
NS_IMPL_CYCLE_COLLECTING_RELEASE(DOMSVGLength)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsSVGLength2::DOMBaseVal)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGLength)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGLength)
NS_INTERFACE_MAP_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsSVGLength2::DOMAnimVal)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGLength)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGLength)
NS_INTERFACE_MAP_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsSVGLength2::DOMAnimatedLength)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGAnimatedLength)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGAnimatedLength)
NS_INTERFACE_MAP_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(DOMSVGLength)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGLength)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGLength)
NS_INTERFACE_MAP_END

static nsIAtom** const unitMap[] =
{
  nsnull, 
  nsnull, 
  &nsGkAtoms::percentage,
  &nsGkAtoms::em,
  &nsGkAtoms::ex,
  &nsGkAtoms::px,
  &nsGkAtoms::cm,
  &nsGkAtoms::mm,
  &nsGkAtoms::in,
  &nsGkAtoms::pt,
  &nsGkAtoms::pc
};



static PRBool
IsValidUnitType(PRUint16 unit)
{
  if (unit > nsIDOMSVGLength::SVG_LENGTHTYPE_UNKNOWN &&
      unit <= nsIDOMSVGLength::SVG_LENGTHTYPE_PC)
    return PR_TRUE;

  return PR_FALSE;
}

static void
GetUnitString(nsAString& unit, PRUint16 unitType)
{
  if (IsValidUnitType(unitType)) {
    if (unitMap[unitType]) {
      (*unitMap[unitType])->ToString(unit);
    }
    return;
  }

  NS_NOTREACHED("Unknown unit type");
  return;
}

static PRUint16
GetUnitTypeForString(const char* unitStr)
{
  if (!unitStr || *unitStr == '\0') 
    return nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER;
                   
  nsCOMPtr<nsIAtom> unitAtom = do_GetAtom(unitStr);

  for (PRUint32 i = 0 ; i < NS_ARRAY_LENGTH(unitMap) ; i++) {
    if (unitMap[i] && *unitMap[i] == unitAtom) {
      return i;
    }
  }

  return nsIDOMSVGLength::SVG_LENGTHTYPE_UNKNOWN;
}

static void
GetValueString(nsAString &aValueAsString, float aValue, PRUint16 aUnitType)
{
  PRUnichar buf[24];
  nsTextFormatter::snprintf(buf, sizeof(buf)/sizeof(PRUnichar),
                            NS_LITERAL_STRING("%g").get(),
                            (double)aValue);
  aValueAsString.Assign(buf);

  nsAutoString unitString;
  GetUnitString(unitString, aUnitType);
  aValueAsString.Append(unitString);
}

static nsresult
GetValueFromString(const nsAString &aValueAsString,
                   float *aValue,
                   PRUint16 *aUnitType)
{
  NS_ConvertUTF16toUTF8 value(aValueAsString);
  const char *str = value.get();

  if (NS_IsAsciiWhitespace(*str))
    return NS_ERROR_FAILURE;
  
  char *rest;
  *aValue = float(PR_strtod(str, &rest));
  if (rest != str) {
    *aUnitType = GetUnitTypeForString(rest);
    if (IsValidUnitType(*aUnitType)) {
      return NS_OK;
    }
  }
  
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
DOMSVGLength::SetValueAsString(const nsAString& aValueAsString)
{
  float value;
  PRUint16 unitType;
  
  nsresult rv = GetValueFromString(aValueAsString, &value, &unitType);
  if (NS_FAILED(rv)) {
    return rv;
  }
  
  mVal.mBaseVal = value;
  mVal.mSpecifiedUnitType = PRUint8(unitType);
  return NS_OK;
}

NS_IMETHODIMP
DOMSVGLength::NewValueSpecifiedUnits(PRUint16 unitType,
                                     float valueInSpecifiedUnits)
{
  NS_ENSURE_FINITE(valueInSpecifiedUnits, NS_ERROR_ILLEGAL_VALUE);
  if (!IsValidUnitType(unitType))
    return NS_OK;

  mVal.mBaseVal = valueInSpecifiedUnits;
  mVal.mSpecifiedUnitType = PRUint8(unitType);
  return NS_OK;
}

NS_IMETHODIMP
DOMSVGLength::ConvertToSpecifiedUnits(PRUint16 unitType)
{
  if (!IsValidUnitType(unitType))
    return NS_OK;

  float valueInUserUnits = 
    mVal.mBaseVal / mVal.GetUnitScaleFactor(mSVGElement, mVal.mSpecifiedUnitType);
  mVal.mSpecifiedUnitType = PRUint8(unitType);
  mVal.mBaseVal = 
    valueInUserUnits * mVal.GetUnitScaleFactor(mSVGElement, mVal.mSpecifiedUnitType);
  return NS_OK;
}

float
nsSVGLength2::GetMMPerPixel(nsSVGSVGElement *aCtx) const
{
  if (!aCtx)
    return 1;

  float mmPerPx = aCtx->GetMMPerPx(mCtxType);

  if (mmPerPx == 0.0f) {
    NS_ASSERTION(mmPerPx != 0.0f, "invalid mm/pixels");
    mmPerPx = 1e-4f; 
  }

  return mmPerPx;
}

 float
nsSVGLength2::GetMMPerPixel(nsIFrame *aNonSVGFrame)
{
  nsPresContext* presContext = aNonSVGFrame->PresContext();
  float pixelsPerInch =
    presContext->AppUnitsToFloatCSSPixels(presContext->AppUnitsPerInch());
  return 25.4f/pixelsPerInch;
}

static float
FixAxisLength(float aLength)
{
  if (aLength == 0.0f) {
    NS_WARNING("zero axis length");
    return 1e-20f;
  }
  return aLength;
}

float
nsSVGLength2::GetAxisLength(nsSVGSVGElement *aCtx) const
{
  if (!aCtx)
    return 1;

  return FixAxisLength(aCtx->GetLength(mCtxType));
}

float
nsSVGLength2::GetAxisLength(nsIFrame *aNonSVGFrame) const
{
  gfxRect rect = nsSVGIntegrationUtils::GetSVGRectForNonSVGFrame(aNonSVGFrame);
  float length;
  switch (mCtxType) {
  case nsSVGUtils::X: length = rect.Width(); break;
  case nsSVGUtils::Y: length = rect.Height(); break;
  case nsSVGUtils::XY:
    length = nsSVGUtils::ComputeNormalizedHypotenuse(rect.Width(), rect.Height());
    break;
  default:
    NS_NOTREACHED("Unknown axis type");
    length = 1;
    break;
  }
  return FixAxisLength(length);
}

float
nsSVGLength2::GetUnitScaleFactor(nsSVGElement *aSVGElement,
                                 PRUint8 aUnitType) const
{
  switch (aUnitType) {
  case nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER:
  case nsIDOMSVGLength::SVG_LENGTHTYPE_PX:
    return 1;
  case nsIDOMSVGLength::SVG_LENGTHTYPE_EMS:
    return 1 / GetEmLength(aSVGElement);
  case nsIDOMSVGLength::SVG_LENGTHTYPE_EXS:
    return 1 / GetExLength(aSVGElement);
  }

  return GetUnitScaleFactor(aSVGElement->GetCtx(), aUnitType);
}

float
nsSVGLength2::GetUnitScaleFactor(nsSVGSVGElement *aCtx, PRUint8 aUnitType) const
{
  switch (aUnitType) {
  case nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER:
  case nsIDOMSVGLength::SVG_LENGTHTYPE_PX:
    return 1;
  case nsIDOMSVGLength::SVG_LENGTHTYPE_MM:
    return GetMMPerPixel(aCtx);
  case nsIDOMSVGLength::SVG_LENGTHTYPE_CM:
    return GetMMPerPixel(aCtx) / 10.0f;
  case nsIDOMSVGLength::SVG_LENGTHTYPE_IN:
    return GetMMPerPixel(aCtx) / 25.4f;
  case nsIDOMSVGLength::SVG_LENGTHTYPE_PT:
    return GetMMPerPixel(aCtx) * POINTS_PER_INCH_FLOAT / 25.4f;
  case nsIDOMSVGLength::SVG_LENGTHTYPE_PC:
    return GetMMPerPixel(aCtx) * POINTS_PER_INCH_FLOAT / 24.4f / 12.0f;
  case nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE:
    return 100.0f / GetAxisLength(aCtx);
  case nsIDOMSVGLength::SVG_LENGTHTYPE_EMS:
    return 1 / GetEmLength(aCtx);
  case nsIDOMSVGLength::SVG_LENGTHTYPE_EXS:
    return 1 / GetExLength(aCtx);
  default:
    NS_NOTREACHED("Unknown unit type");
    return 0;
  }
}

float
nsSVGLength2::GetUnitScaleFactor(nsIFrame *aFrame, PRUint8 aUnitType) const
{
  nsIContent* content = aFrame->GetContent();
  if (content->IsNodeOfType(nsINode::eSVG))
    return GetUnitScaleFactor(static_cast<nsSVGElement*>(content), aUnitType);

  switch (aUnitType) {
  case nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER:
  case nsIDOMSVGLength::SVG_LENGTHTYPE_PX:
    return 1;
  case nsIDOMSVGLength::SVG_LENGTHTYPE_MM:
    return GetMMPerPixel(aFrame);
  case nsIDOMSVGLength::SVG_LENGTHTYPE_CM:
    return GetMMPerPixel(aFrame) / 10.0f;
  case nsIDOMSVGLength::SVG_LENGTHTYPE_IN:
    return GetMMPerPixel(aFrame) / 25.4f;
  case nsIDOMSVGLength::SVG_LENGTHTYPE_PT:
    return GetMMPerPixel(aFrame) * POINTS_PER_INCH_FLOAT / 25.4f;
  case nsIDOMSVGLength::SVG_LENGTHTYPE_PC:
    return GetMMPerPixel(aFrame) * POINTS_PER_INCH_FLOAT / 24.4f / 12.0f;
  case nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE:
    return 100.0f / GetAxisLength(aFrame);
  case nsIDOMSVGLength::SVG_LENGTHTYPE_EMS:
    return 1 / GetEmLength(aFrame);
  case nsIDOMSVGLength::SVG_LENGTHTYPE_EXS:
    return 1 / GetExLength(aFrame);
  default:
    NS_NOTREACHED("Unknown unit type");
    return 0;
  }
}

void
nsSVGLength2::SetBaseValueInSpecifiedUnits(float aValue,
                                           nsSVGElement *aSVGElement)
{
  mBaseVal = aValue;
  aSVGElement->DidChangeLength(mAttrEnum, PR_TRUE);

#ifdef MOZ_SMIL
  if (mIsAnimated) {
    aSVGElement->AnimationNeedsResample();
  }
#endif
}

void
nsSVGLength2::ConvertToSpecifiedUnits(PRUint16 unitType,
                                      nsSVGElement *aSVGElement)
{
  if (!IsValidUnitType(unitType))
    return;

  float valueInUserUnits = 
    mBaseVal / GetUnitScaleFactor(aSVGElement, mSpecifiedUnitType);
  mSpecifiedUnitType = PRUint8(unitType);
  SetBaseValue(valueInUserUnits, aSVGElement);
}

void
nsSVGLength2::NewValueSpecifiedUnits(PRUint16 unitType,
                                     float valueInSpecifiedUnits,
                                     nsSVGElement *aSVGElement)
{
  if (!IsValidUnitType(unitType))
    return;

  mBaseVal = mAnimVal = valueInSpecifiedUnits;
  mSpecifiedUnitType = PRUint8(unitType);
  aSVGElement->DidChangeLength(mAttrEnum, PR_TRUE);

#ifdef MOZ_SMIL
  if (mIsAnimated) {
    aSVGElement->AnimationNeedsResample();
  }
#endif
}

nsresult
nsSVGLength2::ToDOMBaseVal(nsIDOMSVGLength **aResult, nsSVGElement *aSVGElement)
{
  *aResult = new DOMBaseVal(this, aSVGElement);
  if (!*aResult)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aResult);
  return NS_OK;
}

nsresult
nsSVGLength2::ToDOMAnimVal(nsIDOMSVGLength **aResult, nsSVGElement *aSVGElement)
{
  *aResult = new DOMAnimVal(this, aSVGElement);
  if (!*aResult)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aResult);
  return NS_OK;
}



nsresult
nsSVGLength2::SetBaseValueString(const nsAString &aValueAsString,
                                 nsSVGElement *aSVGElement,
                                 PRBool aDoSetAttr)
{
  float value;
  PRUint16 unitType;
  
  nsresult rv = GetValueFromString(aValueAsString, &value, &unitType);
  if (NS_FAILED(rv)) {
    return rv;
  }
  
  mBaseVal = mAnimVal = value;
  mSpecifiedUnitType = PRUint8(unitType);
  aSVGElement->DidChangeLength(mAttrEnum, aDoSetAttr);

#ifdef MOZ_SMIL
  if (mIsAnimated) {
    aSVGElement->AnimationNeedsResample();
  }
#endif

  return NS_OK;
}

void
nsSVGLength2::GetBaseValueString(nsAString & aValueAsString)
{
  GetValueString(aValueAsString, mBaseVal, mSpecifiedUnitType);
}

void
nsSVGLength2::GetAnimValueString(nsAString & aValueAsString)
{
  GetValueString(aValueAsString, mAnimVal, mSpecifiedUnitType);
}

void
nsSVGLength2::SetBaseValue(float aValue, nsSVGElement *aSVGElement)
{
  mAnimVal = mBaseVal = 
    aValue * GetUnitScaleFactor(aSVGElement, mSpecifiedUnitType);
  aSVGElement->DidChangeLength(mAttrEnum, PR_TRUE);
#ifdef MOZ_SMIL
  if (mIsAnimated) {
    aSVGElement->AnimationNeedsResample();
  }
#endif
}

void
nsSVGLength2::SetAnimValue(float aValue, nsSVGElement *aSVGElement)
{
  mAnimVal = aValue * GetUnitScaleFactor(aSVGElement, mSpecifiedUnitType);
  mIsAnimated = PR_TRUE;
  aSVGElement->DidAnimateLength(mAttrEnum);
}

nsresult
nsSVGLength2::ToDOMAnimatedLength(nsIDOMSVGAnimatedLength **aResult,
                                  nsSVGElement *aSVGElement)
{
  *aResult = new DOMAnimatedLength(this, aSVGElement);
  if (!*aResult)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aResult);
  return NS_OK;
}

nsresult
NS_NewDOMSVGLength(nsIDOMSVGLength** aResult, nsSVGElement *aSVGElement)
{
  *aResult = new DOMSVGLength(aSVGElement);
  if (!*aResult)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aResult);
  return NS_OK;
}

#ifdef MOZ_SMIL
nsISMILAttr*
nsSVGLength2::ToSMILAttr(nsSVGElement *aSVGElement)
{
  return new SMILLength(this, aSVGElement);
}

nsresult
nsSVGLength2::SMILLength::ValueFromString(const nsAString& aStr,
                                 const nsISMILAnimationElement* ,
                                 nsSMILValue& aValue) const
{
  float value;
  PRUint16 unitType;
  
  nsresult rv = GetValueFromString(aStr, &value, &unitType);
  if (NS_FAILED(rv)) {
    return rv;
  }

  nsSMILValue val(&nsSMILFloatType::sSingleton);
  val.mU.mDouble = value / mVal->GetUnitScaleFactor(mSVGElement, unitType);
  aValue = val;
  
  return NS_OK;
}

nsSMILValue
nsSVGLength2::SMILLength::GetBaseValue() const
{
  nsSMILValue val(&nsSMILFloatType::sSingleton);
  val.mU.mDouble = mVal->GetBaseValue(mSVGElement);
  return val;
}

nsresult
nsSVGLength2::SMILLength::SetAnimValue(const nsSMILValue& aValue)
{
  NS_ASSERTION(aValue.mType == &nsSMILFloatType::sSingleton,
    "Unexpected type to assign animated value");
  if (aValue.mType == &nsSMILFloatType::sSingleton) {
    mVal->SetAnimValue(float(aValue.mU.mDouble), mSVGElement);
  }
  return NS_OK;
}
#endif 
