








































#include "nsStyleAnimation.h"
#include "nsCOMArray.h"
#include "nsIStyleRule.h"
#include "nsICSSStyleRule.h"
#include "nsString.h"
#include "nsStyleContext.h"
#include "nsStyleSet.h"
#include "nsComputedDOMStyle.h"
#include "nsICSSParser.h"
#include "nsICSSLoader.h"
#include "nsCSSDataBlock.h"
#include "nsCSSDeclaration.h"
#include "prlog.h"
#include <math.h>

















static
nsStyleAnimation::Unit
GetCommonUnit(nsStyleAnimation::Unit aFirstUnit,
              nsStyleAnimation::Unit aSecondUnit)
{
  
  
  if (aFirstUnit != aSecondUnit) {
    
    
    return nsStyleAnimation::eUnit_Null;
  }
  return aFirstUnit;
}




PRBool
nsStyleAnimation::ComputeDistance(const Value& aStartValue,
                                  const Value& aEndValue,
                                  double& aDistance)
{
  Unit commonUnit = GetCommonUnit(aStartValue.GetUnit(), aEndValue.GetUnit());

  PRBool success = PR_TRUE;
  switch (commonUnit) {
    case eUnit_Coord: {
      nscoord startCoord = aStartValue.GetCoordValue();
      nscoord endCoord = aEndValue.GetCoordValue();
      aDistance = fabs(double(endCoord - startCoord));
      break;
    }
    case eUnit_Percent: {
      float startPct = aStartValue.GetPercentValue();
      float endPct = aEndValue.GetPercentValue();
      aDistance = fabs(double(endPct - startPct));
      break;
    }
    case eUnit_Float: {
      float startFloat = aStartValue.GetFloatValue();
      float endFloat = aEndValue.GetFloatValue();
      aDistance = fabs(double(endFloat - startFloat));
      break;
    }
    case eUnit_Color: {
      
      
      
      
      
      
      
      
      
      nscolor startColor = aStartValue.GetColorValue();
      nscolor endColor = aEndValue.GetColorValue();

      
      
      #define GET_COMPONENT(component_, color_) \
        (NS_GET_##component_(color_) * (1.0 / 255.0))

      double startA = GET_COMPONENT(A, startColor);
      double startR = GET_COMPONENT(R, startColor) * startA;
      double startG = GET_COMPONENT(G, startColor) * startA;
      double startB = GET_COMPONENT(B, startColor) * startA;
      double endA = GET_COMPONENT(A, endColor);
      double endR = GET_COMPONENT(R, endColor) * endA;
      double endG = GET_COMPONENT(G, endColor) * endA;
      double endB = GET_COMPONENT(B, endColor) * endA;

      #undef GET_COMPONENT

      double diffA = startA - endA;
      double diffR = startR - endR;
      double diffG = startG - endG;
      double diffB = startB - endB;
      aDistance = sqrt(diffA * diffA + diffR * diffR +
                       diffG * diffG + diffB * diffB);
      break;
    }
    case eUnit_Null:
    case eUnit_None:
      success = PR_FALSE;
      break;
    default:
      NS_NOTREACHED("Can't compute distance using the given common unit");
      success = PR_FALSE;
      break;
  }
  return success;
}

#define MAX_PACKED_COLOR_COMPONENT 255

inline PRUint8 ClampColor(double aColor)
{
  if (aColor >= MAX_PACKED_COLOR_COMPONENT)
    return MAX_PACKED_COLOR_COMPONENT;
  if (aColor <= 0.0)
    return 0;
  return NSToIntRound(aColor);
}

PRBool
nsStyleAnimation::AddWeighted(double aCoeff1, const Value& aValue1,
                              double aCoeff2, const Value& aValue2,
                              Value& aResultValue)
{
  Unit commonUnit = GetCommonUnit(aValue1.GetUnit(), aValue2.GetUnit());
  
  
  
  

  PRBool success = PR_TRUE;
  switch (commonUnit) {
    case eUnit_Coord: {
      aResultValue.SetCoordValue(NSToCoordRound(
        aCoeff1 * aValue1.GetCoordValue() +
        aCoeff2 * aValue2.GetCoordValue()));
      break;
    }
    case eUnit_Percent: {
      aResultValue.SetPercentValue(
        aCoeff1 * aValue1.GetPercentValue() +
        aCoeff2 * aValue2.GetPercentValue());
      break;
    }
    case eUnit_Float: {
      aResultValue.SetFloatValue(
        aCoeff1 * aValue1.GetFloatValue() +
        aCoeff2 * aValue2.GetFloatValue());
      break;
    }
    case eUnit_Color: {
      nscolor color1 = aValue1.GetColorValue();
      nscolor color2 = aValue2.GetColorValue();
      
      
      
      

      
      
      double A1 = NS_GET_A(color1) * (1.0 / 255.0);
      double R1 = NS_GET_R(color1) * A1;
      double G1 = NS_GET_G(color1) * A1;
      double B1 = NS_GET_B(color1) * A1;
      double A2 = NS_GET_A(color2) * (1.0 / 255.0);
      double R2 = NS_GET_R(color2) * A2;
      double G2 = NS_GET_G(color2) * A2;
      double B2 = NS_GET_B(color2) * A2;
      double Aresf = (A1 * aCoeff1 + A2 * aCoeff2);
      nscolor resultColor;
      if (Aresf <= 0.0) {
        resultColor = NS_RGBA(0, 0, 0, 0);
      } else {
        if (Aresf > 1.0) {
          Aresf = 1.0;
        }
        double factor = 1.0 / Aresf;
        PRUint8 Ares = NSToIntRound(Aresf * 255.0);
        PRUint8 Rres = ClampColor((R1 * aCoeff1 + R2 * aCoeff2) * factor);
        PRUint8 Gres = ClampColor((G1 * aCoeff1 + G2 * aCoeff2) * factor);
        PRUint8 Bres = ClampColor((B1 * aCoeff1 + B2 * aCoeff2) * factor);
        resultColor = NS_RGBA(Rres, Gres, Bres, Ares);
      }
      aResultValue.SetColorValue(resultColor);
      break;
    }
    case eUnit_Null:
    case eUnit_None:
      success = PR_FALSE;
      break;
    default:
      NS_NOTREACHED("Can't interpolate using the given common unit");
      success = PR_FALSE;
      break;
  }
  return success;
}

already_AddRefed<nsICSSStyleRule>
BuildStyleRule(nsCSSProperty aProperty,
               nsIContent* aTargetElement,
               const nsAString& aSpecifiedValue)
{
  
  nsCSSDeclaration* declaration = new nsCSSDeclaration();
  if (!declaration) {
    NS_WARNING("Failed to allocate nsCSSDeclaration");
    return nsnull;
  }

  PRBool changed; 
  nsIDocument* doc = aTargetElement->GetOwnerDoc();
  nsCOMPtr<nsIURI> baseURI = aTargetElement->GetBaseURI();
  nsCOMPtr<nsICSSParser> parser;
  nsCOMPtr<nsICSSStyleRule> styleRule;

  
  
  
  
  if (!declaration->InitializeEmpty() ||
      NS_FAILED(doc->CSSLoader()->GetParserFor(nsnull,
                                               getter_AddRefs(parser))) ||
      NS_FAILED(parser->ParseProperty(aProperty, aSpecifiedValue,
                                      doc->GetDocumentURI(), baseURI,
                                      aTargetElement->NodePrincipal(),
                                      declaration, &changed)) ||
      
      !declaration->SlotForValue(aProperty) ||
      NS_FAILED(NS_NewCSSStyleRule(getter_AddRefs(styleRule), nsnull,
                                   declaration))) {
    NS_WARNING("failure in BuildStyleRule");
    declaration->RuleAbort();  
    return nsnull;
  }

  return styleRule.forget();
}

inline
already_AddRefed<nsStyleContext>
LookupStyleContext(nsIContent* aElement)
{
  nsIDocument* doc = aElement->GetCurrentDoc();
  nsIPresShell* shell = doc->GetPrimaryShell();
  if (!shell) {
    return nsnull;
  }
  return nsComputedDOMStyle::GetStyleContextForContent(aElement, nsnull, shell);
}

PRBool
nsStyleAnimation::ComputeValue(nsCSSProperty aProperty,
                               nsIContent* aTargetElement,
                               const nsAString& aSpecifiedValue,
                               Value& aComputedValue)
{
  NS_ABORT_IF_FALSE(aTargetElement, "null target element");
  NS_ABORT_IF_FALSE(aTargetElement->GetCurrentDoc(),
                    "we should only be able to actively animate nodes that "
                    "are in a document");

  
  nsRefPtr<nsStyleContext> styleContext = LookupStyleContext(aTargetElement);
  if (!styleContext) {
    return PR_FALSE;
  }

  
  nsCOMPtr<nsICSSStyleRule> styleRule =
    BuildStyleRule(aProperty, aTargetElement, aSpecifiedValue);
  if (!styleRule) {
    return PR_FALSE;
  }

  
  nsCOMArray<nsIStyleRule> ruleArray;
  ruleArray.AppendObject(styleRule);
  nsStyleSet* styleSet = styleContext->PresContext()->StyleSet();
  nsRefPtr<nsStyleContext> tmpStyleContext =
    styleSet->ResolveStyleForRules(styleContext->GetParent(),
                                   styleContext->GetPseudoType(),
                                   styleContext->GetRuleNode(), ruleArray);

  
  return ExtractComputedValue(aProperty, tmpStyleContext, aComputedValue);
}

PRBool
nsStyleAnimation::UncomputeValue(nsCSSProperty aProperty,
                                 nsPresContext* aPresContext,
                                 const Value& aComputedValue,
                                 void* aSpecifiedValue)
{
  NS_ABORT_IF_FALSE(aPresContext, "null pres context");

  switch (aComputedValue.GetUnit()) {
    case eUnit_None:
      if (nsCSSProps::kAnimTypeTable[aProperty] == eStyleAnimType_PaintServer) {
        NS_ABORT_IF_FALSE(nsCSSProps::kTypeTable[aProperty] ==
                            eCSSType_ValuePair, "type mismatch");
        static_cast<nsCSSValuePair*>(aSpecifiedValue)->
          SetBothValuesTo(nsCSSValue(eCSSUnit_None));
      } else {
        NS_ABORT_IF_FALSE(nsCSSProps::kTypeTable[aProperty] == eCSSType_Value,
                          "type mismatch");
        static_cast<nsCSSValue*>(aSpecifiedValue)->SetNoneValue();
      }
      break;
    case eUnit_Coord: {
      NS_ABORT_IF_FALSE(nsCSSProps::kTypeTable[aProperty] == eCSSType_Value,
                        "type mismatch");
      float pxVal = aPresContext->AppUnitsToFloatCSSPixels(
                                    aComputedValue.GetCoordValue());
      static_cast<nsCSSValue*>(aSpecifiedValue)->
        SetFloatValue(pxVal, eCSSUnit_Pixel);
      break;
    }
    case eUnit_Percent:
      NS_ABORT_IF_FALSE(nsCSSProps::kTypeTable[aProperty] == eCSSType_Value,
                        "type mismatch");
      static_cast<nsCSSValue*>(aSpecifiedValue)->
        SetPercentValue(aComputedValue.GetPercentValue());
      break;
    case eUnit_Float:
      NS_ABORT_IF_FALSE(nsCSSProps::kTypeTable[aProperty] == eCSSType_Value,
                        "type mismatch");
      static_cast<nsCSSValue*>(aSpecifiedValue)->
        SetFloatValue(aComputedValue.GetFloatValue(), eCSSUnit_Number);
      break;
    case eUnit_Color:
      
      if (nsCSSProps::kAnimTypeTable[aProperty] == eStyleAnimType_PaintServer) {
        NS_ABORT_IF_FALSE(nsCSSProps::kTypeTable[aProperty] ==
                            eCSSType_ValuePair, "type mismatch");
        nsCSSValue val;
        val.SetColorValue(aComputedValue.GetColorValue());
        static_cast<nsCSSValuePair*>(aSpecifiedValue)->
          SetBothValuesTo(val);
      } else {
        NS_ABORT_IF_FALSE(nsCSSProps::kTypeTable[aProperty] == eCSSType_Value,
                          "type mismatch");
        static_cast<nsCSSValue*>(aSpecifiedValue)->
          SetColorValue(aComputedValue.GetColorValue());
      }
      break;
    default:
      return PR_FALSE;
  }
  return PR_TRUE;
}

PRBool
nsStyleAnimation::UncomputeValue(nsCSSProperty aProperty,
                                 nsPresContext* aPresContext,
                                 const Value& aComputedValue,
                                 nsAString& aSpecifiedValue)
{
  NS_ABORT_IF_FALSE(aPresContext, "null pres context");
  aSpecifiedValue.Truncate(); 

  nsCSSValuePair vp;
  nsCSSRect rect;
  void *ptr = nsnull;
  void *storage;
  switch (nsCSSProps::kTypeTable[aProperty]) {
    case eCSSType_Value:
      storage = &vp.mXValue;
      break;
    case eCSSType_Rect:
      storage = &rect;
      break;
    case eCSSType_ValuePair: 
      storage = &vp;
      break;
    case eCSSType_ValueList:
    case eCSSType_ValuePairList:
      storage = &ptr;
      break;
    default:
      NS_ABORT_IF_FALSE(PR_FALSE, "unexpected case");
      storage = nsnull;
      break;
  }

  nsCSSValue value;
  if (!nsStyleAnimation::UncomputeValue(aProperty, aPresContext,
                                        aComputedValue, storage)) {
    return PR_FALSE;
  }
  return nsCSSDeclaration::AppendStorageToString(aProperty, storage,
                                                 aSpecifiedValue);
}

inline const void*
StyleDataAtOffset(const void* aStyleStruct, ptrdiff_t aOffset)
{
  return reinterpret_cast<const char*>(aStyleStruct) + aOffset;
}

inline void*
StyleDataAtOffset(void* aStyleStruct, ptrdiff_t aOffset)
{
  return reinterpret_cast<char*>(aStyleStruct) + aOffset;
}

static void
ExtractBorderColor(nsStyleContext* aStyleContext, const void* aStyleBorder,
                   PRUint8 aSide, nsStyleAnimation::Value& aComputedValue)
{
  nscolor color; 
  PRBool foreground;
  static_cast<const nsStyleBorder*>(aStyleBorder)->
    GetBorderColor(aSide, color, foreground);
  if (foreground) {
    
    color = aStyleContext->GetStyleColor()->mColor;
  }
  aComputedValue.SetColorValue(color);
}

static PRBool
StyleCoordToValue(const nsStyleCoord& aCoord, nsStyleAnimation::Value& aValue)
{
  switch (aCoord.GetUnit()) {
    case eStyleUnit_Null:
      return PR_FALSE;
    case eStyleUnit_Normal:
      aValue.SetNormalValue();
      break;
    case eStyleUnit_Auto:
      aValue.SetAutoValue();
      break;
    case eStyleUnit_None:
      aValue.SetNoneValue();
      break;
    case eStyleUnit_Percent:
      aValue.SetPercentValue(aCoord.GetPercentValue());
      break;
    case eStyleUnit_Factor:
      aValue.SetFloatValue(aCoord.GetFactorValue());
      break;
    case eStyleUnit_Coord:
      aValue.SetCoordValue(aCoord.GetCoordValue());
      break;
    case eStyleUnit_Integer:
    case eStyleUnit_Enumerated:
      return PR_FALSE;
  }
  return PR_TRUE;
}

PRBool
nsStyleAnimation::ExtractComputedValue(nsCSSProperty aProperty,
                                       nsStyleContext* aStyleContext,
                                       Value& aComputedValue)
{
  NS_ABORT_IF_FALSE(0 <= aProperty &&
                    aProperty < eCSSProperty_COUNT_no_shorthands,
                    "bad property");
  const void* styleStruct =
    aStyleContext->GetStyleData(nsCSSProps::kSIDTable[aProperty]);
  ptrdiff_t ssOffset = nsCSSProps::kStyleStructOffsetTable[aProperty];
  nsStyleAnimType animType = nsCSSProps::kAnimTypeTable[aProperty];
  NS_ABORT_IF_FALSE(0 <= ssOffset || animType == eStyleAnimType_Custom,
                    "must be dealing with animatable property");
  switch (animType) {
    case eStyleAnimType_Custom:
      switch (aProperty) {
        
        
        
        
        #define BORDER_WIDTH_CASE(prop_, side_)                               \
        case prop_:                                                           \
          aComputedValue.SetCoordValue(                                       \
            static_cast<const nsStyleBorder*>(styleStruct)->                  \
              GetComputedBorder().side_);                                     \
          break;
        BORDER_WIDTH_CASE(eCSSProperty_border_bottom_width, bottom)
        BORDER_WIDTH_CASE(eCSSProperty_border_left_width_value, left)
        BORDER_WIDTH_CASE(eCSSProperty_border_right_width_value, right)
        BORDER_WIDTH_CASE(eCSSProperty_border_top_width, top)
        #undef BORDER_WIDTH_CASE

        case eCSSProperty_border_bottom_color:
          ExtractBorderColor(aStyleContext, styleStruct, NS_SIDE_BOTTOM,
                             aComputedValue);
          break;
        case eCSSProperty_border_left_color_value:
          ExtractBorderColor(aStyleContext, styleStruct, NS_SIDE_LEFT,
                             aComputedValue);
          break;
        case eCSSProperty_border_right_color_value:
          ExtractBorderColor(aStyleContext, styleStruct, NS_SIDE_RIGHT,
                             aComputedValue);
          break;
        case eCSSProperty_border_top_color:
          ExtractBorderColor(aStyleContext, styleStruct, NS_SIDE_TOP,
                             aComputedValue);
          break;

        case eCSSProperty_outline_color: {
          const nsStyleOutline *styleOutline =
            static_cast<const nsStyleOutline*>(styleStruct);
          nscolor color;
        #ifdef GFX_HAS_INVERT
          styleOutline->GetOutlineColor(color);
        #else
          if (!styleOutline->GetOutlineColor(color))
            color = aStyleContext->GetStyleColor()->mColor;
        #endif
          aComputedValue.SetColorValue(color);
          break;
        }

        default:
          NS_ABORT_IF_FALSE(PR_FALSE, "missing property implementation");
          return PR_FALSE;
      };
      return PR_TRUE;
    case eStyleAnimType_Coord:
      return StyleCoordToValue(*static_cast<const nsStyleCoord*>(
        StyleDataAtOffset(styleStruct, ssOffset)), aComputedValue);
    case eStyleAnimType_Sides_Top:
    case eStyleAnimType_Sides_Right:
    case eStyleAnimType_Sides_Bottom:
    case eStyleAnimType_Sides_Left: {
      PR_STATIC_ASSERT(0 == NS_SIDE_TOP);
      PR_STATIC_ASSERT(eStyleAnimType_Sides_Right - eStyleAnimType_Sides_Top
                         == NS_SIDE_RIGHT);
      PR_STATIC_ASSERT(eStyleAnimType_Sides_Bottom - eStyleAnimType_Sides_Top
                         == NS_SIDE_BOTTOM);
      PR_STATIC_ASSERT(eStyleAnimType_Sides_Left - eStyleAnimType_Sides_Top
                         == NS_SIDE_LEFT);
      const nsStyleCoord &coord = static_cast<const nsStyleSides*>(
        StyleDataAtOffset(styleStruct, ssOffset))->
          Get(animType - eStyleAnimType_Sides_Top);
      return StyleCoordToValue(coord, aComputedValue);
    }
    case eStyleAnimType_nscoord:
      aComputedValue.SetCoordValue(*static_cast<const nscoord*>(
        StyleDataAtOffset(styleStruct, ssOffset)));
      return PR_TRUE;
    case eStyleAnimType_float:
      aComputedValue.SetFloatValue(*static_cast<const float*>(
        StyleDataAtOffset(styleStruct, ssOffset)));
      if (aProperty == eCSSProperty_font_size_adjust &&
          aComputedValue.GetFloatValue() == 0.0f) {
        
        
        
        
        aComputedValue.SetNoneValue();
      }
      return PR_TRUE;
    case eStyleAnimType_Color:
      aComputedValue.SetColorValue(*static_cast<const nscolor*>(
        StyleDataAtOffset(styleStruct, ssOffset)));
      return PR_TRUE;
    case eStyleAnimType_PaintServer: {
      const nsStyleSVGPaint &paint = *static_cast<const nsStyleSVGPaint*>(
        StyleDataAtOffset(styleStruct, ssOffset));
      
      if (paint.mType == eStyleSVGPaintType_Color) {
        aComputedValue.SetColorValue(paint.mPaint.mColor);
        return PR_TRUE;
      }
      if (paint.mType == eStyleSVGPaintType_None) {
        aComputedValue.SetNoneValue();
        return PR_TRUE;
      }
      return PR_FALSE;
    }
    case eStyleAnimType_None:
      NS_NOTREACHED("shouldn't use on non-animatable properties");
  }
  return PR_FALSE;
}

nsStyleAnimation::Value::Value(nscoord aLength, CoordConstructorType)
{
  mUnit = eUnit_Coord;
  mValue.mCoord = aLength;
}

nsStyleAnimation::Value::Value(float aPercent, PercentConstructorType)
{
  mUnit = eUnit_Percent;
  mValue.mFloat = aPercent;
}

nsStyleAnimation::Value::Value(float aFloat, FloatConstructorType)
{
  mUnit = eUnit_Float;
  mValue.mFloat = aFloat;
}

nsStyleAnimation::Value::Value(nscolor aColor, ColorConstructorType)
{
  mUnit = eUnit_Color;
  mValue.mColor = aColor;
}

nsStyleAnimation::Value&
nsStyleAnimation::Value::operator=(const Value& aOther)
{
  FreeValue();

  mUnit = aOther.mUnit;
  switch (mUnit) {
    case eUnit_Null:
    case eUnit_Normal:
    case eUnit_Auto:
    case eUnit_None:
      break;
    case eUnit_Coord:
      mValue.mCoord = aOther.mValue.mCoord;
      break;
    case eUnit_Percent:
    case eUnit_Float:
      mValue.mFloat = aOther.mValue.mFloat;
      break;
    case eUnit_Color:
      mValue.mColor = aOther.mValue.mColor;
      break;
  }

  return *this;
}

void
nsStyleAnimation::Value::SetNormalValue()
{
  FreeValue();
  mUnit = eUnit_Normal;
}

void
nsStyleAnimation::Value::SetAutoValue()
{
  FreeValue();
  mUnit = eUnit_Auto;
}

void
nsStyleAnimation::Value::SetNoneValue()
{
  FreeValue();
  mUnit = eUnit_None;
}

void
nsStyleAnimation::Value::SetCoordValue(nscoord aLength)
{
  FreeValue();
  mUnit = eUnit_Coord;
  mValue.mCoord = aLength;
}

void
nsStyleAnimation::Value::SetPercentValue(float aPercent)
{
  FreeValue();
  mUnit = eUnit_Percent;
  mValue.mFloat = aPercent;
}

void
nsStyleAnimation::Value::SetFloatValue(float aFloat)
{
  FreeValue();
  mUnit = eUnit_Float;
  mValue.mFloat = aFloat;
}

void
nsStyleAnimation::Value::SetColorValue(nscolor aColor)
{
  FreeValue();
  mUnit = eUnit_Color;
  mValue.mColor = aColor;
}

void
nsStyleAnimation::Value::FreeValue()
{
}

PRBool
nsStyleAnimation::Value::operator==(const Value& aOther) const
{
  if (mUnit != aOther.mUnit) {
    return PR_FALSE;
  }

  switch (mUnit) {
    case eUnit_Null:
    case eUnit_Normal:
    case eUnit_Auto:
    case eUnit_None:
      return PR_TRUE;
    case eUnit_Coord:
      return mValue.mCoord == aOther.mValue.mCoord;
    case eUnit_Percent:
    case eUnit_Float:
      return mValue.mFloat == aOther.mValue.mFloat;
    case eUnit_Color:
      return mValue.mColor == aOther.mValue.mColor;
  }

  NS_NOTREACHED("incomplete case");
  return PR_FALSE;
}

