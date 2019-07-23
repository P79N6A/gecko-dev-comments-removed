





































#include "nsSVGLength2.h"
#include "prdtoa.h"
#include "nsTextFormatter.h"
#include "nsSVGSVGElement.h"

NS_IMPL_ADDREF(nsSVGLength2::DOMBaseVal)
NS_IMPL_RELEASE(nsSVGLength2::DOMBaseVal)

NS_IMPL_ADDREF(nsSVGLength2::DOMAnimVal)
NS_IMPL_RELEASE(nsSVGLength2::DOMAnimVal)

NS_IMPL_ADDREF(nsSVGLength2::DOMAnimatedLength)
NS_IMPL_RELEASE(nsSVGLength2::DOMAnimatedLength)

NS_INTERFACE_MAP_BEGIN(nsSVGLength2::DOMBaseVal)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGLength)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGLength)
NS_INTERFACE_MAP_END

NS_INTERFACE_MAP_BEGIN(nsSVGLength2::DOMAnimVal)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGLength)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGLength)
NS_INTERFACE_MAP_END

NS_INTERFACE_MAP_BEGIN(nsSVGLength2::DOMAnimatedLength)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGAnimatedLength)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGAnimatedLength)
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
nsSVGLength2::GetAxisLength(nsSVGSVGElement *aCtx) const
{
  if (!aCtx)
    return 1;

  float d = aCtx->GetLength(mCtxType);

  if (d == 0.0f) {
    NS_WARNING("zero axis length");
    d = 1e-20f;
  }

  return d;
}

float
nsSVGLength2::GetUnitScaleFactor(nsSVGElement *aSVGElement) const
{
  switch (mSpecifiedUnitType) {
  case nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER:
  case nsIDOMSVGLength::SVG_LENGTHTYPE_PX:
    return 1;
  case nsIDOMSVGLength::SVG_LENGTHTYPE_EMS:
    return 1 / GetEmLength(aSVGElement);
  case nsIDOMSVGLength::SVG_LENGTHTYPE_EXS:
    return 1 / GetExLength(aSVGElement);
  }

  return GetUnitScaleFactor(aSVGElement->GetCtx());
}

float
nsSVGLength2::GetUnitScaleFactor(nsSVGSVGElement *aCtx) const
{
  switch (mSpecifiedUnitType) {
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

void
nsSVGLength2::SetBaseValueInSpecifiedUnits(float aValue,
                                           nsSVGElement *aSVGElement)
{
  mBaseVal = aValue;
  aSVGElement->DidChangeLength(mAttrEnum, PR_TRUE);
}

void
nsSVGLength2::ConvertToSpecifiedUnits(PRUint16 unitType,
                                      nsSVGElement *aSVGElement)
{
  if (!IsValidUnitType(unitType))
    return;

  float valueInUserUnits = mBaseVal / GetUnitScaleFactor(aSVGElement);
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
  mAnimVal = mBaseVal = aValue * GetUnitScaleFactor(aSVGElement);
  aSVGElement->DidChangeLength(mAttrEnum, PR_TRUE);
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
