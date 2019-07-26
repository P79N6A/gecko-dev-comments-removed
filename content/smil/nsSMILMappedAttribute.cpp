





#include "nsSMILMappedAttribute.h"
#include "nsAttrValue.h"
#include "nsError.h" 
#include "nsSMILValue.h"
#include "nsSMILCSSValueType.h"
#include "nsIDocument.h"
#include "nsIPresShell.h"
#include "nsCSSProps.h"
#include "mozilla/dom/Element.h"


static void
ReleaseStringBufferPropertyValue(void*    aObject,       
                                 nsIAtom* aPropertyName, 
                                 void*    aPropertyValue,
                                 void*    aData          )
{
  nsStringBuffer* buf = static_cast<nsStringBuffer*>(aPropertyValue);
  buf->Release();
}


nsresult
nsSMILMappedAttribute::ValueFromString(const nsAString& aStr,
                                       const mozilla::dom::SVGAnimationElement* aSrcElement,
                                       nsSMILValue& aValue,
                                       bool& aPreventCachingOfSandwich) const
{
  NS_ENSURE_TRUE(IsPropertyAnimatable(mPropID), NS_ERROR_FAILURE);

  nsSMILCSSValueType::ValueFromString(mPropID, mElement, aStr, aValue,
                                      &aPreventCachingOfSandwich);
  return aValue.IsNull() ? NS_ERROR_FAILURE : NS_OK;
}

nsSMILValue
nsSMILMappedAttribute::GetBaseValue() const
{
  nsAutoString baseStringValue;
  nsRefPtr<nsIAtom> attrName = GetAttrNameAtom();
  bool success = mElement->GetAttr(kNameSpaceID_None, attrName,
                                     baseStringValue);
  nsSMILValue baseValue;
  if (success) {
    
    
    
    
    nsSMILCSSValueType::ValueFromString(mPropID, mElement,
                                        baseStringValue, baseValue, nullptr);
  } else {
    
    
    
    void* buf = mElement->UnsetProperty(SMIL_MAPPED_ATTR_ANIMVAL,
                                        attrName, nullptr);
    FlushChangesToTargetAttr();

    
    
    
    
    
    baseValue = nsSMILCSSProperty::GetBaseValue();

    
    if (buf) {
      mElement->SetProperty(SMIL_MAPPED_ATTR_ANIMVAL, attrName, buf,
                            ReleaseStringBufferPropertyValue);
      FlushChangesToTargetAttr();
    }
  }
  return baseValue;
}

nsresult
nsSMILMappedAttribute::SetAnimValue(const nsSMILValue& aValue)
{
  NS_ENSURE_TRUE(IsPropertyAnimatable(mPropID), NS_ERROR_FAILURE);

  
  nsAutoString valStr;
  if (!nsSMILCSSValueType::ValueToString(aValue, valStr)) {
    NS_WARNING("Failed to convert nsSMILValue for mapped attr into a string");
    return NS_ERROR_FAILURE;
  }

  nsRefPtr<nsIAtom> attrName = GetAttrNameAtom();
  nsStringBuffer* oldValStrBuf = static_cast<nsStringBuffer*>
    (mElement->GetProperty(SMIL_MAPPED_ATTR_ANIMVAL, attrName));
  if (oldValStrBuf && valStr.Equals(nsCheapString(oldValStrBuf))) {
    return NS_OK;
  }

  
  nsStringBuffer* valStrBuf =
    nsCSSValue::BufferFromString(nsString(valStr)).get();
  nsresult rv = mElement->SetProperty(SMIL_MAPPED_ATTR_ANIMVAL,
                                      attrName, valStrBuf,
                                      ReleaseStringBufferPropertyValue);
  if (rv == NS_PROPTABLE_PROP_OVERWRITTEN) {
    rv = NS_OK;
  }
  FlushChangesToTargetAttr();

  return rv;
}

void
nsSMILMappedAttribute::ClearAnimValue()
{
  nsRefPtr<nsIAtom> attrName = GetAttrNameAtom();
  mElement->DeleteProperty(SMIL_MAPPED_ATTR_ANIMVAL, attrName);
  FlushChangesToTargetAttr();
}

void
nsSMILMappedAttribute::FlushChangesToTargetAttr() const
{
  
  mElement->DeleteProperty(SMIL_MAPPED_ATTR_ANIMVAL,
                           SMIL_MAPPED_ATTR_STYLERULE_ATOM);
  nsIDocument* doc = mElement->GetCurrentDoc();

  
  if (doc) {
    nsIPresShell* shell = doc->GetShell();
    if (shell) {
      shell->RestyleForAnimation(mElement, eRestyle_Self);
    }
  }
}

already_AddRefed<nsIAtom>
nsSMILMappedAttribute::GetAttrNameAtom() const
{
  return do_GetAtom(nsCSSProps::GetStringValue(mPropID));
}
