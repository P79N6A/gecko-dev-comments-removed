







































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

















static
nsStyleUnit
GetCommonUnit(nsStyleUnit aFirstUnit,
              nsStyleUnit aSecondUnit)
{
  
  
  if (aFirstUnit != aSecondUnit) {
    
    
    
    
    
    NS_WARNING("start unit & end unit don't match -- need to resolve this "
               "and pick which one we want to use");
    return eStyleUnit_Null;
  }
  return aFirstUnit;
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
    case eStyleUnit_Null:
      NS_WARNING("Unable to find a common unit for given values");
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
    case eStyleUnit_Null:
      NS_WARNING("Unable to find a common unit for given values");
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
    case eStyleUnit_Null:
      NS_WARNING("Unable to find a common unit for given values");
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
nsStyleAnimation::ExtractComputedValue(nsCSSProperty aProperty,
                                       nsStyleContext* aStyleContext,
                                       nsStyleCoord& aComputedValue)
{
  
  
  
  
  switch(aProperty) {
    case eCSSProperty_font_size: {
      const nsStyleFont* styleFont = aStyleContext->GetStyleFont();
      aComputedValue.SetCoordValue(styleFont->mFont.size);
      return PR_TRUE;
    }
    case eCSSProperty_stroke_width: {
      const nsStyleSVG* styleSVG = aStyleContext->GetStyleSVG();
      aComputedValue = styleSVG->mStrokeWidth;
      return PR_TRUE;
    }
    default:
      return PR_FALSE;
  }
}
