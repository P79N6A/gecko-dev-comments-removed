








































#include "nsStyleAnimation.h"
#include "nsCOMArray.h"
#include "nsIStyleRule.h"
#include "nsICSSStyleRule.h"
#include "nsString.h"
#include "nsStyleCoord.h"
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
nsStyleUnit
GetCommonUnit(nsStyleUnit aFirstUnit,
              nsStyleUnit aSecondUnit)
{
  
  
  if (aFirstUnit != aSecondUnit) {
    
    
    return eStyleUnit_Null;
  }
  return aFirstUnit;
}




#define MAX_PACKED_COLOR_COMPONENT 255

inline PRUint8 ClampColor(PRUint32 aColor)
{
  if (aColor >= MAX_PACKED_COLOR_COMPONENT)
    return MAX_PACKED_COLOR_COMPONENT;
  return aColor;
}

PRBool
nsStyleAnimation::Add(nsStyleCoord& aDest, const nsStyleCoord& aValueToAdd,
                      PRUint32 aCount)
{
  nsStyleUnit commonUnit = GetCommonUnit(aDest.GetUnit(),
                                         aValueToAdd.GetUnit());
  PRBool success = PR_TRUE;
  switch (commonUnit) {
    case eStyleUnit_Coord: {
      nscoord destCoord = aDest.GetCoordValue();
      nscoord valueToAddCoord = aValueToAdd.GetCoordValue();
      destCoord += aCount * valueToAddCoord;
      aDest.SetCoordValue(destCoord);
      break;
    }
    case eStyleUnit_Percent: {
      float destPct = aDest.GetPercentValue();
      float valueToAddPct = aValueToAdd.GetPercentValue();
      destPct += aCount * valueToAddPct;
      aDest.SetPercentValue(destPct);
      break;
    }
    case eStyleUnit_Factor: {
      float destFactor = aDest.GetFactorValue();
      float valueToAddFactor = aValueToAdd.GetFactorValue();
      destFactor += aCount * valueToAddFactor;
      aDest.SetFactorValue(destFactor);
      break;
    }
    case eStyleUnit_Color: {
      
      
      
      
      
      
      
      
      
      nscolor destColor = aDest.GetColorValue();
      nscolor colorToAdd = aValueToAdd.GetColorValue();
      if (aCount > MAX_PACKED_COLOR_COMPONENT) {
        
        
        
        aCount = MAX_PACKED_COLOR_COMPONENT;
      }
      PRUint8 resultR =
        ClampColor(NS_GET_R(destColor) + aCount * NS_GET_R(colorToAdd));
      PRUint8 resultG =
        ClampColor(NS_GET_G(destColor) + aCount * NS_GET_G(colorToAdd));
      PRUint8 resultB =
        ClampColor(NS_GET_B(destColor) + aCount * NS_GET_B(colorToAdd));
      PRUint8 resultA =
        ClampColor(NS_GET_A(destColor) + aCount * NS_GET_A(colorToAdd));
      aDest.SetColorValue(NS_RGBA(resultR, resultG, resultB, resultA));
      break;
    }
    case eStyleUnit_Null:
      success = PR_FALSE;
      break;
    default:
      NS_NOTREACHED("Can't add nsStyleCoords using the given common unit");
      success = PR_FALSE;
      break;
  }
  return success;
}

PRBool
nsStyleAnimation::ComputeDistance(const nsStyleCoord& aStartValue,
                                  const nsStyleCoord& aEndValue,
                                  double& aDistance)
{
  nsStyleUnit commonUnit = GetCommonUnit(aStartValue.GetUnit(),
                                         aEndValue.GetUnit());

  PRBool success = PR_TRUE;
  switch (commonUnit) {
    case eStyleUnit_Coord: {
      nscoord startCoord = aStartValue.GetCoordValue();
      nscoord endCoord = aEndValue.GetCoordValue();
      aDistance = fabs(double(endCoord - startCoord));
      break;
    }
    case eStyleUnit_Percent: {
      float startPct = aStartValue.GetPercentValue();
      float endPct = aEndValue.GetPercentValue();
      aDistance = fabs(double(endPct - startPct));
      break;
    }
    case eStyleUnit_Factor: {
      float startFactor = aStartValue.GetFactorValue();
      float endFactor = aEndValue.GetFactorValue();
      aDistance = fabs(double(endFactor - startFactor));
      break;
    }
    case eStyleUnit_Color: {
      
      
      
      
      
      
      
      
      
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
    case eStyleUnit_Null:
      success = PR_FALSE;
      break;
    default:
      NS_NOTREACHED("Can't compute distance using the given common unit");
      success = PR_FALSE;
      break;
  }
  return success;
}

PRBool
nsStyleAnimation::Interpolate(const nsStyleCoord& aStartValue,
                              const nsStyleCoord& aEndValue,
                              double aPortion,
                              nsStyleCoord& aResultValue)
{
  NS_ABORT_IF_FALSE(aPortion >= 0.0 && aPortion <= 1.0,
                    "aPortion out of bounds");
  nsStyleUnit commonUnit = GetCommonUnit(aStartValue.GetUnit(),
                                         aEndValue.GetUnit());
  
  
  
  

  PRBool success = PR_TRUE;
  switch (commonUnit) {
    case eStyleUnit_Coord: {
      nscoord startCoord = aStartValue.GetCoordValue();
      nscoord endCoord = aEndValue.GetCoordValue();
      nscoord resultCoord = startCoord +
        NSToCoordRound(aPortion * (endCoord - startCoord));
      aResultValue.SetCoordValue(resultCoord);
      break;
    }
    case eStyleUnit_Percent: {
      float startPct = aStartValue.GetPercentValue();
      float endPct = aEndValue.GetPercentValue();
      float resultPct = startPct + aPortion * (endPct - startPct);
      aResultValue.SetPercentValue(resultPct);
      break;
    }
    case eStyleUnit_Factor: {
      float startFactor = aStartValue.GetFactorValue();
      float endFactor = aEndValue.GetFactorValue();
      float resultFactor = startFactor + aPortion * (endFactor - startFactor);
      aResultValue.SetFactorValue(resultFactor);
      break;
    }
    case eStyleUnit_Color: {
      double inv = 1.0 - aPortion;
      nscolor startColor = aStartValue.GetColorValue();
      nscolor endColor = aEndValue.GetColorValue();
      
      
      
      

      
      
      double startA = NS_GET_A(startColor) * (1.0 / 255.0);
      double startR = NS_GET_R(startColor) * startA;
      double startG = NS_GET_G(startColor) * startA;
      double startB = NS_GET_B(startColor) * startA;
      double endA = NS_GET_A(endColor) * (1.0 / 255.0);
      double endR = NS_GET_R(endColor) * endA;
      double endG = NS_GET_G(endColor) * endA;
      double endB = NS_GET_B(endColor) * endA;
      double resAf = (startA * inv + endA * aPortion);
      nscolor resultColor;
      if (resAf == 0.0) {
        resultColor = NS_RGBA(0, 0, 0, 0);
      } else {
        double factor = 1.0 / resAf;
        PRUint8 resA = NSToIntRound(resAf * 255.0);
        PRUint8 resR = NSToIntRound((startR * inv + endR * aPortion) * factor);
        PRUint8 resG = NSToIntRound((startG * inv + endG * aPortion) * factor);
        PRUint8 resB = NSToIntRound((startB * inv + endB * aPortion) * factor);
        resultColor = NS_RGBA(resR, resG, resB, resA);
      }
      aResultValue.SetColorValue(resultColor);
      break;
    }
    case eStyleUnit_Null:
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
                               nsStyleCoord& aComputedValue)
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
                                 const nsStyleCoord& aComputedValue,
                                 void* aSpecifiedValue)
{
  NS_ABORT_IF_FALSE(aPresContext, "null pres context");

  switch (aComputedValue.GetUnit()) {
    case eStyleUnit_None:
      NS_ABORT_IF_FALSE(nsCSSProps::kTypeTable[aProperty] ==
                          eCSSType_ValuePair, "type mismatch");
      static_cast<nsCSSValuePair*>(aSpecifiedValue)->
        SetBothValuesTo(nsCSSValue(eCSSUnit_None));
      break;
    case eStyleUnit_Coord: {
      NS_ABORT_IF_FALSE(nsCSSProps::kTypeTable[aProperty] == eCSSType_Value,
                        "type mismatch");
      float pxVal = aPresContext->AppUnitsToFloatCSSPixels(
                                    aComputedValue.GetCoordValue());
      static_cast<nsCSSValue*>(aSpecifiedValue)->
        SetFloatValue(pxVal, eCSSUnit_Pixel);
      break;
    }
    case eStyleUnit_Percent:
      NS_ABORT_IF_FALSE(nsCSSProps::kTypeTable[aProperty] == eCSSType_Value,
                        "type mismatch");
      static_cast<nsCSSValue*>(aSpecifiedValue)->
        SetPercentValue(aComputedValue.GetPercentValue());
      break;
    case eStyleUnit_Factor:
      NS_ABORT_IF_FALSE(nsCSSProps::kTypeTable[aProperty] == eCSSType_Value,
                        "type mismatch");
      static_cast<nsCSSValue*>(aSpecifiedValue)->
        SetFloatValue(aComputedValue.GetFactorValue(), eCSSUnit_Number);
      break;
    case eStyleUnit_Color:
      
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
                                 const nsStyleCoord& aComputedValue,
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

PRBool
nsStyleAnimation::ExtractComputedValue(nsCSSProperty aProperty,
                                       nsStyleContext* aStyleContext,
                                       nsStyleCoord& aComputedValue)
{
  NS_ABORT_IF_FALSE(0 <= aProperty &&
                    aProperty < eCSSProperty_COUNT_no_shorthands,
                    "bad property");
  const void* styleStruct =
    aStyleContext->GetStyleData(nsCSSProps::kSIDTable[aProperty]);
  ptrdiff_t ssOffset = nsCSSProps::kStyleStructOffsetTable[aProperty];
  NS_ABORT_IF_FALSE(0 <= ssOffset, "must be dealing with animatable property");
  nsStyleAnimType animType = nsCSSProps::kAnimTypeTable[aProperty];
  switch (animType) {
    case eStyleAnimType_Coord:
      aComputedValue = *static_cast<const nsStyleCoord*>(
        StyleDataAtOffset(styleStruct, ssOffset));
      return PR_TRUE;
    case eStyleAnimType_Sides_Top:
    case eStyleAnimType_Sides_Right:
    case eStyleAnimType_Sides_Bottom:
    case eStyleAnimType_Sides_Left:
      PR_STATIC_ASSERT(0 == NS_SIDE_TOP);
      PR_STATIC_ASSERT(eStyleAnimType_Sides_Right - eStyleAnimType_Sides_Top
                         == NS_SIDE_RIGHT);
      PR_STATIC_ASSERT(eStyleAnimType_Sides_Bottom - eStyleAnimType_Sides_Top
                         == NS_SIDE_BOTTOM);
      PR_STATIC_ASSERT(eStyleAnimType_Sides_Left - eStyleAnimType_Sides_Top
                         == NS_SIDE_LEFT);
      aComputedValue = static_cast<const nsStyleSides*>(
        StyleDataAtOffset(styleStruct, ssOffset))->
          Get(animType - eStyleAnimType_Sides_Top);
      return PR_TRUE;
    case eStyleAnimType_nscoord:
      aComputedValue.SetCoordValue(*static_cast<const nscoord*>(
        StyleDataAtOffset(styleStruct, ssOffset)));
      return PR_TRUE;
    case eStyleAnimType_float:
      aComputedValue.SetFactorValue(*static_cast<const float*>(
        StyleDataAtOffset(styleStruct, ssOffset)));
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
