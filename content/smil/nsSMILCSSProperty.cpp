






































#include "nsSMILCSSProperty.h"
#include "nsSMILCSSValueType.h"
#include "nsSMILValue.h"
#include "nsComputedDOMStyle.h"
#include "nsStyleAnimation.h"
#include "mozilla/dom/Element.h"
#include "nsIDOMElement.h"

using namespace mozilla::dom;


static PRBool
GetCSSComputedValue(nsIContent* aElem,
                    nsCSSProperty aPropID,
                    nsAString& aResult)
{
  NS_ABORT_IF_FALSE(!nsCSSProps::IsShorthand(aPropID),
                    "Can't look up computed value of shorthand property");
  NS_ABORT_IF_FALSE(nsSMILCSSProperty::IsPropertyAnimatable(aPropID),
                    "Shouldn't get here for non-animatable properties");

  nsIDocument* doc = aElem->GetCurrentDoc();
  if (!doc) {
    
    
    
    return PR_FALSE;
  }

  nsIPresShell* shell = doc->GetShell();
  if (!shell) {
    NS_WARNING("Unable to look up computed style -- no pres shell");
    return PR_FALSE;
  }

  nsRefPtr<nsComputedDOMStyle> computedStyle;
  nsCOMPtr<nsIDOMElement> domElement(do_QueryInterface(aElem));
  nsresult rv = NS_NewComputedDOMStyle(domElement, EmptyString(), shell,
                                       getter_AddRefs(computedStyle));

  if (NS_SUCCEEDED(rv)) {
    computedStyle->GetPropertyValue(aPropID, aResult);
    return PR_TRUE;
  }
  return PR_FALSE;
}


nsSMILCSSProperty::nsSMILCSSProperty(nsCSSProperty aPropID,
                                     Element* aElement)
  : mPropID(aPropID), mElement(aElement)
{
  NS_ABORT_IF_FALSE(IsPropertyAnimatable(mPropID),
                    "Creating a nsSMILCSSProperty for a property "
                    "that's not supported for animation");
}

nsSMILValue
nsSMILCSSProperty::GetBaseValue() const
{
  
  
  
  nsSMILValue baseValue;

  
  
  if (nsCSSProps::IsShorthand(mPropID) || mPropID == eCSSProperty_display) {
    
    
    
    
    
    
    
    
    
    
    nsSMILValue tmpVal(&nsSMILCSSValueType::sSingleton);
    baseValue.Swap(tmpVal);
    return baseValue;
  }

  
  
  
  nsCOMPtr<nsICSSDeclaration> overrideDecl =
    do_QueryInterface(mElement->GetSMILOverrideStyle());
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

  
  if (didGetComputedVal) {
    nsSMILCSSValueType::ValueFromString(mPropID, mElement,
                                        computedStyleVal, baseValue);
  }
  return baseValue;
}

nsresult
nsSMILCSSProperty::ValueFromString(const nsAString& aStr,
                                   const nsISMILAnimationElement* aSrcElement,
                                   nsSMILValue& aValue,
                                   PRBool& aPreventCachingOfSandwich) const
{
  NS_ENSURE_TRUE(IsPropertyAnimatable(mPropID), NS_ERROR_FAILURE);

  nsSMILCSSValueType::ValueFromString(mPropID, mElement, aStr, aValue);
  if (aValue.IsNull()) {
    return NS_ERROR_FAILURE;
  }

  
  
  
  
  
  
  
  
  
  aPreventCachingOfSandwich = PR_TRUE;
  return NS_OK;
}

nsresult
nsSMILCSSProperty::SetAnimValue(const nsSMILValue& aValue)
{
  NS_ENSURE_TRUE(IsPropertyAnimatable(mPropID), NS_ERROR_FAILURE);

  
  nsAutoString valStr;
  if (!nsSMILCSSValueType::ValueToString(aValue, valStr)) {
    NS_WARNING("Failed to convert nsSMILValue for CSS property into a string");
    return NS_ERROR_FAILURE;
  }

  
  nsCOMPtr<nsICSSDeclaration> overrideDecl =
    do_QueryInterface(mElement->GetSMILOverrideStyle());
  if (overrideDecl) {
    overrideDecl->SetPropertyValue(mPropID, valStr);
  }
  return NS_OK;
}

void
nsSMILCSSProperty::ClearAnimValue()
{
  
  nsCOMPtr<nsICSSDeclaration> overrideDecl =
    do_QueryInterface(mElement->GetSMILOverrideStyle());
  if (overrideDecl) {
    overrideDecl->SetPropertyValue(mPropID, EmptyString());
  }
}



PRBool
nsSMILCSSProperty::IsPropertyAnimatable(nsCSSProperty aPropID)
{
  
  
  
  
  
  
  
  
  
  

  switch (aPropID) {
    case eCSSProperty_clip:
    case eCSSProperty_clip_rule:
    case eCSSProperty_clip_path:
    case eCSSProperty_color:
    case eCSSProperty_color_interpolation:
    case eCSSProperty_color_interpolation_filters:
    case eCSSProperty_cursor:
    case eCSSProperty_display:
    case eCSSProperty_dominant_baseline:
    case eCSSProperty_fill:
    case eCSSProperty_fill_opacity:
    case eCSSProperty_fill_rule:
    case eCSSProperty_filter:
    case eCSSProperty_flood_color:
    case eCSSProperty_flood_opacity:
    case eCSSProperty_font:
    case eCSSProperty_font_family:
    case eCSSProperty_font_size:
    case eCSSProperty_font_size_adjust:
    case eCSSProperty_font_stretch:
    case eCSSProperty_font_style:
    case eCSSProperty_font_variant:
    case eCSSProperty_font_weight:
    case eCSSProperty_image_rendering:
    case eCSSProperty_letter_spacing:
    case eCSSProperty_lighting_color:
    case eCSSProperty_marker:
    case eCSSProperty_marker_end:
    case eCSSProperty_marker_mid:
    case eCSSProperty_marker_start:
    case eCSSProperty_mask:
    case eCSSProperty_opacity:
    case eCSSProperty_overflow:
    case eCSSProperty_pointer_events:
    case eCSSProperty_shape_rendering:
    case eCSSProperty_stop_color:
    case eCSSProperty_stop_opacity:
    case eCSSProperty_stroke:
    case eCSSProperty_stroke_dasharray:
    case eCSSProperty_stroke_dashoffset:
    case eCSSProperty_stroke_linecap:
    case eCSSProperty_stroke_linejoin:
    case eCSSProperty_stroke_miterlimit:
    case eCSSProperty_stroke_opacity:
    case eCSSProperty_stroke_width:
    case eCSSProperty_text_anchor:
    case eCSSProperty_text_blink:
    case eCSSProperty_text_decoration:
    case eCSSProperty_text_decoration_line:
    case eCSSProperty_text_rendering:
    case eCSSProperty_visibility:
    case eCSSProperty_word_spacing:
      return PR_TRUE;

    
    
    
    
    
    
    
    
    case eCSSProperty_direction:
    case eCSSProperty_unicode_bidi:
      return PR_FALSE;

    default:
      return PR_FALSE;
  }
}
