




#include "mozilla/ArrayUtils.h"

#include "SVGLength.h"
#include "nsSVGElement.h"
#include "mozilla/dom/SVGSVGElement.h"
#include "nsTextFormatter.h"
#include "SVGContentUtils.h"
#include <limits>
#include <algorithm>

namespace mozilla {


static void GetUnitString(nsAString& unit, uint16_t unitType);
static uint16_t GetUnitTypeForString(const nsAString& unitStr);

void
SVGLength::GetValueAsString(nsAString &aValue) const
{
  char16_t buf[24];
  nsTextFormatter::snprintf(buf, sizeof(buf)/sizeof(char16_t),
                            MOZ_UTF16("%g"),
                            (double)mValue);
  aValue.Assign(buf);

  nsAutoString unitString;
  GetUnitString(unitString, mUnit);
  aValue.Append(unitString);
}

bool
SVGLength::SetValueFromString(const nsAString &aString)
{
  RangedPtr<const char16_t> iter =
    SVGContentUtils::GetStartRangedPtr(aString);
  const RangedPtr<const char16_t> end =
    SVGContentUtils::GetEndRangedPtr(aString);

  float value;

  if (!SVGContentUtils::ParseNumber(iter, end, value)) {
    return false;
  }

  const nsAString& units = Substring(iter.get(), end.get());
  uint16_t unitType = GetUnitTypeForString(units);
  if (!IsValidUnitType(unitType)) {
    return false;
  }
  mValue = value;
  mUnit = uint8_t(unitType);
  return true;
}

inline static bool
IsAbsoluteUnit(uint8_t aUnit)
{
  return aUnit >= nsIDOMSVGLength::SVG_LENGTHTYPE_CM &&
         aUnit <= nsIDOMSVGLength::SVG_LENGTHTYPE_PC;
}












inline static float GetAbsUnitsPerAbsUnit(uint8_t aUnits, uint8_t aPerUnit)
{
  NS_ABORT_IF_FALSE(IsAbsoluteUnit(aUnits), "Not a CSS absolute unit");
  NS_ABORT_IF_FALSE(IsAbsoluteUnit(aPerUnit), "Not a CSS absolute unit");

  float CSSAbsoluteUnitConversionFactors[5][5] = { 
    
    { 1.0f, 0.1f, 2.54f, 0.035277777777777778f, 0.42333333333333333f },
    
    { 10.0f, 1.0f, 25.4f, 0.35277777777777778f, 4.2333333333333333f },
    
    { 0.39370078740157481f, 0.039370078740157481f, 1.0f, 0.013888888888888889f, 0.16666666666666667f },
    
    { 28.346456692913386f, 2.8346456692913386f, 72.0f, 1.0f, 12.0f },
    
    { 2.3622047244094489f, 0.23622047244094489f, 6.0f, 0.083333333333333333f, 1.0f }
  };

  
  return CSSAbsoluteUnitConversionFactors[aUnits - 6][aPerUnit - 6];
}

float
SVGLength::GetValueInSpecifiedUnit(uint8_t aUnit,
                                   const nsSVGElement *aElement,
                                   uint8_t aAxis) const
{
  if (aUnit == mUnit) {
    return mValue;
  }
  if ((aUnit == nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER &&
       mUnit == nsIDOMSVGLength::SVG_LENGTHTYPE_PX) ||
      (aUnit == nsIDOMSVGLength::SVG_LENGTHTYPE_PX &&
       mUnit == nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER)) {
    return mValue;
  }
  if (IsAbsoluteUnit(aUnit) && IsAbsoluteUnit(mUnit)) {
    return mValue * GetAbsUnitsPerAbsUnit(aUnit, mUnit);
  }

  
  
  

  float userUnitsPerCurrentUnit = GetUserUnitsPerUnit(aElement, aAxis);
  float userUnitsPerNewUnit =
    SVGLength(0.0f, aUnit).GetUserUnitsPerUnit(aElement, aAxis);

  NS_ASSERTION(userUnitsPerCurrentUnit >= 0 ||
               !NS_finite(userUnitsPerCurrentUnit),
               "bad userUnitsPerCurrentUnit");
  NS_ASSERTION(userUnitsPerNewUnit >= 0 ||
               !NS_finite(userUnitsPerNewUnit),
               "bad userUnitsPerNewUnit");

  float value = mValue * userUnitsPerCurrentUnit / userUnitsPerNewUnit;

  
  
  if (NS_finite(value)) {
    return value;
  }
  return std::numeric_limits<float>::quiet_NaN();
}

#define INCHES_PER_MM_FLOAT float(0.0393700787)
#define INCHES_PER_CM_FLOAT float(0.393700787)

float
SVGLength::GetUserUnitsPerUnit(const nsSVGElement *aElement, uint8_t aAxis) const
{
  switch (mUnit) {
    case nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER:
    case nsIDOMSVGLength::SVG_LENGTHTYPE_PX:
      return 1.0f;
    case nsIDOMSVGLength::SVG_LENGTHTYPE_MM:
      return INCHES_PER_MM_FLOAT * GetUserUnitsPerInch();
    case nsIDOMSVGLength::SVG_LENGTHTYPE_CM:
      return INCHES_PER_CM_FLOAT * GetUserUnitsPerInch();
    case nsIDOMSVGLength::SVG_LENGTHTYPE_IN:
      return GetUserUnitsPerInch();
    case nsIDOMSVGLength::SVG_LENGTHTYPE_PT:
      return (1.0f/POINTS_PER_INCH_FLOAT) * GetUserUnitsPerInch();
    case nsIDOMSVGLength::SVG_LENGTHTYPE_PC:
      return (12.0f/POINTS_PER_INCH_FLOAT) * GetUserUnitsPerInch();
    case nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE:
      return GetUserUnitsPerPercent(aElement, aAxis);
    case nsIDOMSVGLength::SVG_LENGTHTYPE_EMS:
      return SVGContentUtils::GetFontSize(const_cast<nsSVGElement*>(aElement));
    case nsIDOMSVGLength::SVG_LENGTHTYPE_EXS:
      return SVGContentUtils::GetFontXHeight(const_cast<nsSVGElement*>(aElement));
    default:
      NS_NOTREACHED("Unknown unit type");
      return std::numeric_limits<float>::quiet_NaN();
  }
}

 float
SVGLength::GetUserUnitsPerPercent(const nsSVGElement *aElement, uint8_t aAxis)
{
  if (aElement) {
    dom::SVGSVGElement *viewportElement = aElement->GetCtx();
    if (viewportElement) {
      return std::max(viewportElement->GetLength(aAxis) / 100.0f, 0.0f);
    }
  }
  return std::numeric_limits<float>::quiet_NaN();
}




static nsIAtom** const unitMap[] =
{
  nullptr, 
  nullptr, 
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

static void
GetUnitString(nsAString& unit, uint16_t unitType)
{
  if (SVGLength::IsValidUnitType(unitType)) {
    if (unitMap[unitType]) {
      (*unitMap[unitType])->ToString(unit);
    }
    return;
  }
  NS_NOTREACHED("Unknown unit type"); 
  return;
}

static uint16_t
GetUnitTypeForString(const nsAString& unitStr)
{
  if (unitStr.IsEmpty())
    return nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER;

  nsIAtom* unitAtom = NS_GetStaticAtom(unitStr);

  if (unitAtom) {
    for (uint32_t i = 1 ; i < ArrayLength(unitMap) ; i++) {
      if (unitMap[i] && *unitMap[i] == unitAtom) {
        return i;
      }
    }
  }
  return nsIDOMSVGLength::SVG_LENGTHTYPE_UNKNOWN;
}

} 
