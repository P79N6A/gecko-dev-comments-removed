






































#include "nsSMILCSSProperty.h"
#include "nsISMILCSSValueType.h"
#include "nsSMILCSSValueType.h"
#include "nsSMILValue.h"
#include "nsCSSDeclaration.h"
#include "nsComputedDOMStyle.h"
#include "nsStyleAnimation.h"
#include "nsIContent.h"
#include "nsPIDOMWindow.h"


static nsISMILCSSValueType*
GetSMILTypeForProperty(nsCSSProperty aPropID)
{
  if (!nsSMILCSSProperty::IsPropertyAnimatable(aPropID)) {
    NS_NOTREACHED("Attempting to animate an un-animatable property");
    return nsnull;
  }
  if (aPropID < eCSSProperty_COUNT_no_shorthands) {
    return &nsSMILCSSValueType::sSingleton;
  }
  return nsnull; 
}

static PRBool
GetCSSComputedValue(nsIContent* aElem,
                    nsCSSProperty aPropID,
                    nsAString& aResult)
{
  NS_ENSURE_TRUE(nsSMILCSSProperty::IsPropertyAnimatable(aPropID),
                 PR_FALSE);

  nsIDocument* doc = aElem->GetCurrentDoc();
  NS_ABORT_IF_FALSE(doc,"any target element that's actively being animated "
                    "must be in a document");
  nsRefPtr<nsComputedDOMStyle>
    computedStyle(doc->GetWindow()->LookupComputedStyleFor(aElem));
  if (computedStyle) {
    
    computedStyle->GetPropertyValue(aPropID, aResult);
    return PR_TRUE;
  }
  return PR_FALSE;
}


nsSMILCSSProperty::nsSMILCSSProperty(nsCSSProperty aPropID,
                                     nsIContent* aElement)
  : mPropID(aPropID), mElement(aElement)
{
  NS_ABORT_IF_FALSE(IsPropertyAnimatable(mPropID),
                    "Creating a nsSMILCSSProperty for a property "
                    "that's not supported for animation");
}

nsSMILValue
nsSMILCSSProperty::GetBaseValue() const
{
  
  
  nsCOMPtr<nsIDOMCSSStyleDeclaration> overrideStyle;
  mElement->GetSMILOverrideStyle(getter_AddRefs(overrideStyle));
  nsCOMPtr<nsICSSDeclaration> overrideDecl = do_QueryInterface(overrideStyle);
  nsAutoString cachedOverrideStyleVal;
  if (overrideDecl) {
    overrideDecl->GetPropertyValue(mPropID, cachedOverrideStyleVal);
    
    if (!cachedOverrideStyleVal.IsEmpty()) {
      overrideDecl->SetPropertyValue(mPropID, EmptyString());
    }
  }

  
  nsAutoString computedStyleVal;
  PRBool didGetComputedVal = GetCSSComputedValue(mElement, mPropID,
                                                 computedStyleVal);

  
  if (overrideDecl && !cachedOverrideStyleVal.IsEmpty()) {
    overrideDecl->SetPropertyValue(mPropID, cachedOverrideStyleVal);
  }

  nsSMILValue baseValue;
  if (didGetComputedVal) {
    
    nsISMILCSSValueType* smilType = GetSMILTypeForProperty(mPropID);
    NS_ABORT_IF_FALSE(smilType, "animating an unsupported type");
    
    smilType->Init(baseValue);
    if (!smilType->ValueFromString(mPropID, mElement,
                                   computedStyleVal, baseValue)) {
      smilType->Destroy(baseValue);
      NS_ABORT_IF_FALSE(baseValue.IsNull(),
                        "Destroy should leave us with null-typed value");
    }
  }
  return baseValue;
}

nsresult
nsSMILCSSProperty::ValueFromString(const nsAString& aStr,
                                   const nsISMILAnimationElement* aSrcElement,
                                   nsSMILValue& aValue) const
{
  NS_ENSURE_TRUE(IsPropertyAnimatable(mPropID), NS_ERROR_FAILURE);
  nsISMILCSSValueType* smilType = GetSMILTypeForProperty(mPropID);
  smilType->Init(aValue);
  PRBool success = smilType->ValueFromString(mPropID, mElement, aStr, aValue);
  if (!success) {
    smilType->Destroy(aValue);
  }
  return success ? NS_OK : NS_ERROR_FAILURE;
}

nsresult
nsSMILCSSProperty::SetAnimValue(const nsSMILValue& aValue)
{
  NS_ENSURE_TRUE(IsPropertyAnimatable(mPropID), NS_ERROR_FAILURE);

  nsresult rv = NS_OK;
  nsAutoString valStr;
  nsISMILCSSValueType* smilType = GetSMILTypeForProperty(mPropID);

  if (smilType->ValueToString(aValue, valStr)) {
    
    nsCOMPtr<nsIDOMCSSStyleDeclaration> overrideStyle;
    mElement->GetSMILOverrideStyle(getter_AddRefs(overrideStyle));
    NS_ABORT_IF_FALSE(overrideStyle, "Need a non-null overrideStyle");

    nsCOMPtr<nsICSSDeclaration> overrideDecl =
      do_QueryInterface(overrideStyle);
    if (overrideDecl) {
      overrideDecl->SetPropertyValue(mPropID, valStr);
    }
  } else {
    NS_WARNING("Failed to convert nsSMILValue for CSS property into a string");
    rv = NS_ERROR_FAILURE;
  }

  return rv;
}

void
nsSMILCSSProperty::ClearAnimValue()
{
  
  nsCOMPtr<nsIDOMCSSStyleDeclaration> overrideStyle;
  mElement->GetSMILOverrideStyle(getter_AddRefs(overrideStyle));
  nsCOMPtr<nsICSSDeclaration> overrideDecl = do_QueryInterface(overrideStyle);
  if (overrideDecl) {
    overrideDecl->SetPropertyValue(mPropID, EmptyString());
  }
}



PRBool
nsSMILCSSProperty::IsPropertyAnimatable(nsCSSProperty aPropID)
{
  
  
  
  
  
  
  
  
  
  

  switch (aPropID) {
    
    case eCSSProperty_font:
    case eCSSProperty_marker:
    case eCSSProperty_overflow:
      
      return PR_FALSE;

    
    case eCSSProperty_clip:
      
      return PR_FALSE;

    
    case eCSSProperty_cursor:
    case eCSSProperty_stroke_dasharray:
      
      return PR_FALSE;

    
    case eCSSProperty_fill:
    case eCSSProperty_stroke:
      return PR_TRUE;

    
    
    
    
    
    
    
    case eCSSProperty_color:
    
    
    
    
    case eCSSProperty_fill_opacity:
    
    
    case eCSSProperty_flood_color:
    case eCSSProperty_flood_opacity:
    
    case eCSSProperty_font_size:
    case eCSSProperty_font_size_adjust:
    
    
    
    
    
    case eCSSProperty_letter_spacing:
    case eCSSProperty_lighting_color:
    
    
    
    
    case eCSSProperty_opacity:
    
    
    case eCSSProperty_stop_color:
    case eCSSProperty_stop_opacity:
    case eCSSProperty_stroke_dashoffset:
    
    
    case eCSSProperty_stroke_miterlimit:
    case eCSSProperty_stroke_opacity:
    case eCSSProperty_stroke_width:
    
    
    
    
    case eCSSProperty_word_spacing:
      return PR_TRUE;

    
    
    
    
    
    
    
    
    case eCSSProperty_direction:
    case eCSSProperty_unicode_bidi:
      return PR_FALSE;

    default:
      return PR_FALSE;
  }
}
