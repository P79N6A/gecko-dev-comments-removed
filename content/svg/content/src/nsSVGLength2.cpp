





































#include "nsContentUtils.h"
#include "nsSVGLength2.h"
#include "nsGkAtoms.h"
#include "prdtoa.h"
#include "nsCRT.h"
#include "nsTextFormatter.h"
#include "nsIDOMSVGNumber.h"
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



static PRBool
IsValidUnitType(PRUint16 unit)
{
  if (unit > nsIDOMSVGLength::SVG_LENGTHTYPE_UNKNOWN &&
      unit <= nsIDOMSVGLength::SVG_LENGTHTYPE_PC)
    return PR_TRUE;

  return PR_FALSE;
}

static void
GetValueString(nsAString &aValueAsString, float aValue, PRUint16 aUnitType)

{
  PRUnichar buf[24];
  nsTextFormatter::snprintf(buf, sizeof(buf)/sizeof(PRUnichar),
                            NS_LITERAL_STRING("%g").get(),
                            (double)aValue);
  aValueAsString.Assign(buf);

  nsIAtom* UnitAtom = nsnull;

  switch (aUnitType) {
    case nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER:
      return;
    case nsIDOMSVGLength::SVG_LENGTHTYPE_PX:
      UnitAtom = nsGkAtoms::px;
      break;
    case nsIDOMSVGLength::SVG_LENGTHTYPE_MM:
      UnitAtom = nsGkAtoms::mm;
      break;
    case nsIDOMSVGLength::SVG_LENGTHTYPE_CM:
      UnitAtom = nsGkAtoms::cm;
      break;
    case nsIDOMSVGLength::SVG_LENGTHTYPE_IN:
      UnitAtom = nsGkAtoms::in;
      break;
    case nsIDOMSVGLength::SVG_LENGTHTYPE_PT:
      UnitAtom = nsGkAtoms::pt;
      break;
    case nsIDOMSVGLength::SVG_LENGTHTYPE_PC:
      UnitAtom = nsGkAtoms::pc;
      break;
    case nsIDOMSVGLength::SVG_LENGTHTYPE_EMS:
      UnitAtom = nsGkAtoms::em;
      break;
    case nsIDOMSVGLength::SVG_LENGTHTYPE_EXS:
      UnitAtom = nsGkAtoms::ex;
      break;
    case nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE:
      UnitAtom = nsGkAtoms::percentage;
      break;
    default:
      NS_NOTREACHED("Unknown unit");
      return;
  }

  nsAutoString unitString;
  UnitAtom->ToString(unitString);
  aValueAsString.Append(unitString);
}

float
nsSVGLength2::GetMMPerPixel(nsSVGSVGElement *aCtx)
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
nsSVGLength2::GetAxisLength(nsSVGSVGElement *aCtx)
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



nsresult
nsSVGLength2::SetBaseValueString(const nsAString &aValueAsString,
                                 nsSVGElement *aSVGElement,
                                 PRBool aDoSetAttr)
{
  nsresult rv = NS_OK;
  
  char *str = ToNewCString(aValueAsString);
  if (!str)
    return NS_ERROR_OUT_OF_MEMORY;

  char* number = str;
  while (*number && isspace(*number))
    ++number;

  if (*number) {
    char *rest;
    double value = PR_strtod(number, &rest);
    if (rest!=number) {
      const char* unitStr = nsCRT::strtok(rest, "\x20\x9\xD\xA", &rest);
      PRUint16 unitType = nsIDOMSVGLength::SVG_LENGTHTYPE_UNKNOWN;
      if (!unitStr || *unitStr=='\0') {
        unitType = nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER;
      }
      else {
        nsCOMPtr<nsIAtom> unitAtom = do_GetAtom(unitStr);
        if (unitAtom == nsGkAtoms::px)
          unitType = nsIDOMSVGLength::SVG_LENGTHTYPE_PX;
        else if (unitAtom == nsGkAtoms::mm)
          unitType = nsIDOMSVGLength::SVG_LENGTHTYPE_MM;
        else if (unitAtom == nsGkAtoms::cm)
          unitType = nsIDOMSVGLength::SVG_LENGTHTYPE_CM;
        else if (unitAtom == nsGkAtoms::in)
          unitType = nsIDOMSVGLength::SVG_LENGTHTYPE_IN;
        else if (unitAtom == nsGkAtoms::pt)
          unitType = nsIDOMSVGLength::SVG_LENGTHTYPE_PT;
        else if (unitAtom == nsGkAtoms::pc)
          unitType = nsIDOMSVGLength::SVG_LENGTHTYPE_PC;
        else if (unitAtom == nsGkAtoms::em)
          unitType = nsIDOMSVGLength::SVG_LENGTHTYPE_EMS;
        else if (unitAtom == nsGkAtoms::ex)
          unitType = nsIDOMSVGLength::SVG_LENGTHTYPE_EXS;
        else if (unitAtom == nsGkAtoms::percentage)
          unitType = nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE;
      }
      if (IsValidUnitType(unitType)){
        mBaseVal = mAnimVal = (float)value;
        mSpecifiedUnitType     = unitType;
        aSVGElement->DidChangeLength(mAttrEnum, aDoSetAttr);
      }
      else { 
        
        rv = NS_ERROR_FAILURE;
        NS_ERROR("invalid length type");
      }
    }
    else { 
      
      rv = NS_ERROR_FAILURE;
    }
  }
  
  nsMemory::Free(str);
    
  return rv;
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

float
nsSVGLength2::ConvertToUserUnits(float aVal, nsSVGElement *aSVGElement)
{
  if (mSpecifiedUnitType == nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER ||
      mSpecifiedUnitType == nsIDOMSVGLength::SVG_LENGTHTYPE_PX)
    return aVal;

  return ConvertToUserUnits(aVal, aSVGElement->GetCtx());
}

float
nsSVGLength2::ConvertToUserUnits(float aVal, nsSVGSVGElement *aCtx)
{
  switch (mSpecifiedUnitType) {
    case nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER:
    case nsIDOMSVGLength::SVG_LENGTHTYPE_PX:
      return aVal;
    case nsIDOMSVGLength::SVG_LENGTHTYPE_MM:
      return aVal / GetMMPerPixel(aCtx);
    case nsIDOMSVGLength::SVG_LENGTHTYPE_CM:
      return aVal * 10.0f / GetMMPerPixel(aCtx);
    case nsIDOMSVGLength::SVG_LENGTHTYPE_IN:
      return aVal * 25.4f / GetMMPerPixel(aCtx);
    case nsIDOMSVGLength::SVG_LENGTHTYPE_PT:
      return aVal * 25.4f / 72.0f / GetMMPerPixel(aCtx);
    case nsIDOMSVGLength::SVG_LENGTHTYPE_PC:
      return aVal * 25.4f * 12.0f / 72.0f / GetMMPerPixel(aCtx);
    case nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE:
      return aVal * GetAxisLength(aCtx) / 100.0f;
    case nsIDOMSVGLength::SVG_LENGTHTYPE_EMS:
    case nsIDOMSVGLength::SVG_LENGTHTYPE_EXS:
      NS_NOTYETIMPLEMENTED("nsIDOMSVGLength::SVG_LENGTHTYPE_EXS");
      return 0;
    default:
      NS_NOTREACHED("Unknown unit type");
      return 0;
  }
}

void
nsSVGLength2::SetBaseValue(float aValue, nsSVGElement *aSVGElement)
{
  nsSVGSVGElement *ctx = nsnull;

  if (mSpecifiedUnitType != nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER &&
      mSpecifiedUnitType != nsIDOMSVGLength::SVG_LENGTHTYPE_PX)
    ctx = aSVGElement->GetCtx();

  switch (mSpecifiedUnitType) {
    case nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER:
    case nsIDOMSVGLength::SVG_LENGTHTYPE_PX:
      mBaseVal = aValue;
      break;
    case nsIDOMSVGLength::SVG_LENGTHTYPE_MM:
      mBaseVal = aValue * GetMMPerPixel(ctx);
      break;
    case nsIDOMSVGLength::SVG_LENGTHTYPE_CM:
      mBaseVal = aValue * GetMMPerPixel(ctx) / 10.0f;
      break;
    case nsIDOMSVGLength::SVG_LENGTHTYPE_IN:
      mBaseVal = aValue * GetMMPerPixel(ctx) / 25.4f;
      break;
    case nsIDOMSVGLength::SVG_LENGTHTYPE_PT:
      mBaseVal = aValue * GetMMPerPixel(ctx) * 72.0f / 25.4f;
      break;
    case nsIDOMSVGLength::SVG_LENGTHTYPE_PC:
      mBaseVal = aValue * GetMMPerPixel(ctx) * 72.0f / 24.4f / 12.0f;
      break;
    case nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE:
      mBaseVal = aValue * 100.0f / GetAxisLength(ctx);
      break;
    case nsIDOMSVGLength::SVG_LENGTHTYPE_EMS:
    case nsIDOMSVGLength::SVG_LENGTHTYPE_EXS:
      NS_NOTYETIMPLEMENTED("nsIDOMSVGLength::SVG_LENGTHTYPE_EXS");
      mBaseVal = 0;
      break;
    default:
      NS_NOTREACHED("Unknown unit type");
      mBaseVal = 0;
      break;
  }

  mAnimVal = mBaseVal;
  aSVGElement->DidChangeLength(mAttrEnum, PR_TRUE);
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

  float valueInUserUnits;
  valueInUserUnits = GetBaseValue(aSVGElement);
  mSpecifiedUnitType = unitType;
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
  mSpecifiedUnitType = unitType;
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
nsSVGLength2::ToDOMAnimatedLength(nsIDOMSVGAnimatedLength **aResult,
                                  nsSVGElement *aSVGElement)
{
  *aResult = new DOMAnimatedLength(this, aSVGElement);
  if (!*aResult)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aResult);
  return NS_OK;
}
