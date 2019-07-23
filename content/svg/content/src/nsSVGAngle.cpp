



































#include "nsSVGAngle.h"
#include "prdtoa.h"
#include "nsTextFormatter.h"
#include "nsSVGUtils.h"

class DOMSVGAngle : public nsIDOMSVGAngle
{
public:
  NS_DECL_ISUPPORTS

  DOMSVGAngle()
    { mVal.Init(); }
    
  NS_IMETHOD GetUnitType(PRUint16* aResult)
    { *aResult = mVal.mSpecifiedUnitType; return NS_OK; }

  NS_IMETHOD GetValue(float* aResult)
    { *aResult = mVal.GetBaseValue(); return NS_OK; }
  NS_IMETHOD SetValue(float aValue)
    {
      NS_ENSURE_FINITE(aValue, NS_ERROR_ILLEGAL_VALUE);
      mVal.SetBaseValue(aValue, nsnull);
      return NS_OK;
    }

  NS_IMETHOD GetValueInSpecifiedUnits(float* aResult)
    { *aResult = mVal.mBaseVal; return NS_OK; }
  NS_IMETHOD SetValueInSpecifiedUnits(float aValue)
    {
      NS_ENSURE_FINITE(aValue, NS_ERROR_ILLEGAL_VALUE);
      mVal.mBaseVal = aValue;
      return NS_OK;
    }

  NS_IMETHOD SetValueAsString(const nsAString& aValue)
    { return mVal.SetBaseValueString(aValue, nsnull, PR_FALSE); }
  NS_IMETHOD GetValueAsString(nsAString& aValue)
    { mVal.GetBaseValueString(aValue); return NS_OK; }

  NS_IMETHOD NewValueSpecifiedUnits(PRUint16 unitType,
                                    float valueInSpecifiedUnits)
    {
      NS_ENSURE_FINITE(valueInSpecifiedUnits, NS_ERROR_ILLEGAL_VALUE);
      mVal.NewValueSpecifiedUnits(unitType, valueInSpecifiedUnits, nsnull);
      return NS_OK;
    }

  NS_IMETHOD ConvertToSpecifiedUnits(PRUint16 unitType)
    { mVal.ConvertToSpecifiedUnits(unitType, nsnull); return NS_OK; }

private:
  nsSVGAngle mVal;
};

NS_SVG_VAL_IMPL_CYCLE_COLLECTION(nsSVGAngle::DOMBaseVal, mSVGElement)

NS_SVG_VAL_IMPL_CYCLE_COLLECTION(nsSVGAngle::DOMAnimVal, mSVGElement)

NS_SVG_VAL_IMPL_CYCLE_COLLECTION(nsSVGAngle::DOMAnimatedAngle, mSVGElement)

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsSVGAngle::DOMBaseVal)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsSVGAngle::DOMBaseVal)

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsSVGAngle::DOMAnimVal)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsSVGAngle::DOMAnimVal)

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsSVGAngle::DOMAnimatedAngle)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsSVGAngle::DOMAnimatedAngle)

NS_IMPL_ADDREF(DOMSVGAngle)
NS_IMPL_RELEASE(DOMSVGAngle)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsSVGAngle::DOMBaseVal)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGAngle)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGAngle)
NS_INTERFACE_MAP_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsSVGAngle::DOMAnimVal)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGAngle)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGAngle)
NS_INTERFACE_MAP_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsSVGAngle::DOMAnimatedAngle)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGAnimatedAngle)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGAnimatedAngle)
NS_INTERFACE_MAP_END

NS_INTERFACE_MAP_BEGIN(DOMSVGAngle)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGAngle)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGAngle)
NS_INTERFACE_MAP_END

static nsIAtom** const unitMap[] =
{
  nsnull, 
  nsnull, 
  &nsGkAtoms::deg,
  &nsGkAtoms::rad,
  &nsGkAtoms::grad
};



static PRBool
IsValidUnitType(PRUint16 unit)
{
  if (unit > nsIDOMSVGAngle::SVG_ANGLETYPE_UNKNOWN &&
      unit <= nsIDOMSVGAngle::SVG_ANGLETYPE_GRAD)
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
    return nsIDOMSVGAngle::SVG_ANGLETYPE_UNSPECIFIED;
                   
  nsCOMPtr<nsIAtom> unitAtom = do_GetAtom(unitStr);

  for (PRUint32 i = 0 ; i < NS_ARRAY_LENGTH(unitMap) ; i++) {
    if (unitMap[i] && *unitMap[i] == unitAtom) {
      return i;
    }
  }

  return nsIDOMSVGAngle::SVG_ANGLETYPE_UNKNOWN;
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
  if (rest != str && NS_FloatIsFinite(*aValue)) {
    *aUnitType = GetUnitTypeForString(rest);
    if (IsValidUnitType(*aUnitType)) {
      return NS_OK;
    }
  }
  
  return NS_ERROR_FAILURE;
}

float
nsSVGAngle::GetUnitScaleFactor() const
{
  switch (mSpecifiedUnitType) {
  case nsIDOMSVGAngle::SVG_ANGLETYPE_UNSPECIFIED:
  case nsIDOMSVGAngle::SVG_ANGLETYPE_DEG:
    return static_cast<float>(180.0 / M_PI);
  case nsIDOMSVGAngle::SVG_ANGLETYPE_RAD:
    return 1;
  case nsIDOMSVGAngle::SVG_ANGLETYPE_GRAD:
    return static_cast<float>(100.0 / M_PI);
  default:
    NS_NOTREACHED("Unknown unit type");
    return 0;
  }
}

void
nsSVGAngle::SetBaseValueInSpecifiedUnits(float aValue,
                                         nsSVGElement *aSVGElement)
{
  mBaseVal = aValue;
  aSVGElement->DidChangeAngle(mAttrEnum, PR_TRUE);
}

void
nsSVGAngle::ConvertToSpecifiedUnits(PRUint16 unitType,
                                    nsSVGElement *aSVGElement)
{
  if (!IsValidUnitType(unitType))
    return;

  float valueInUserUnits = mBaseVal / GetUnitScaleFactor();
  mSpecifiedUnitType = PRUint8(unitType);
  SetBaseValue(valueInUserUnits, aSVGElement);
}

void
nsSVGAngle::NewValueSpecifiedUnits(PRUint16 unitType,
                                   float valueInSpecifiedUnits,
                                   nsSVGElement *aSVGElement)
{
  if (!IsValidUnitType(unitType))
    return;

  mBaseVal = mAnimVal = valueInSpecifiedUnits;
  mSpecifiedUnitType = PRUint8(unitType);
  if (aSVGElement) {
    aSVGElement->DidChangeAngle(mAttrEnum, PR_TRUE);
  }
}

nsresult
nsSVGAngle::ToDOMBaseVal(nsIDOMSVGAngle **aResult, nsSVGElement *aSVGElement)
{
  *aResult = new DOMBaseVal(this, aSVGElement);
  if (!*aResult)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aResult);
  return NS_OK;
}

nsresult
nsSVGAngle::ToDOMAnimVal(nsIDOMSVGAngle **aResult, nsSVGElement *aSVGElement)
{
  *aResult = new DOMAnimVal(this, aSVGElement);
  if (!*aResult)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aResult);
  return NS_OK;
}



nsresult
nsSVGAngle::SetBaseValueString(const nsAString &aValueAsString,
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
  if (aSVGElement) {
    aSVGElement->DidChangeAngle(mAttrEnum, aDoSetAttr);
  }

  return NS_OK;
}

void
nsSVGAngle::GetBaseValueString(nsAString & aValueAsString)
{
  GetValueString(aValueAsString, mBaseVal, mSpecifiedUnitType);
}

void
nsSVGAngle::GetAnimValueString(nsAString & aValueAsString)
{
  GetValueString(aValueAsString, mAnimVal, mSpecifiedUnitType);
}

void
nsSVGAngle::SetBaseValue(float aValue, nsSVGElement *aSVGElement)
{
  mAnimVal = mBaseVal = aValue * GetUnitScaleFactor();
  if (aSVGElement) {
    aSVGElement->DidChangeAngle(mAttrEnum, PR_TRUE);
  }
}

nsresult
nsSVGAngle::ToDOMAnimatedAngle(nsIDOMSVGAnimatedAngle **aResult,
                               nsSVGElement *aSVGElement)
{
  *aResult = new DOMAnimatedAngle(this, aSVGElement);
  if (!*aResult)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aResult);
  return NS_OK;
}

nsresult
NS_NewDOMSVGAngle(nsIDOMSVGAngle** aResult)
{
  *aResult = new DOMSVGAngle;
  if (!*aResult)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aResult);
  return NS_OK;
}
