





































#include "nsSMILMappedAttribute.h"
#include "nsPropertyTable.h"
#include "nsContentErrors.h" 
#include "nsSMILValue.h"
#include "nsSMILCSSValueType.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIPresShell.h"
#include "nsCSSProps.h"


static void
ReleaseStringBufferPropertyValue(void*    aObject,       
                                 nsIAtom* aPropertyName, 
                                 void*    aPropertyValue,
                                 void*    aData          )
{
  nsStringBuffer* buf = static_cast<nsStringBuffer*>(aPropertyValue);
  buf->Release();
}

nsSMILValue
nsSMILMappedAttribute::GetBaseValue() const
{
  nsAutoString baseStringValue;
  nsRefPtr<nsIAtom> attrName = GetAttrNameAtom();
  PRBool success = mElement->GetAttr(kNameSpaceID_None, attrName,
                                     baseStringValue);
  nsSMILValue baseValue;
  if (success) {
    nsSMILCSSValueType::ValueFromString(mPropID, mElement,
                                        baseStringValue, baseValue);
  } else {
    
    
    
    void* buf = mElement->UnsetProperty(SMIL_MAPPED_ATTR_ANIMVAL,
                                        attrName, nsnull);
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

  
  nsStringBuffer* valStrBuf = nsCSSValue::BufferFromString(nsString(valStr));
  nsRefPtr<nsIAtom> attrName = GetAttrNameAtom();
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
  nsresult rv = mElement->DeleteProperty(SMIL_MAPPED_ATTR_ANIMVAL, attrName);
  if (NS_FAILED(rv)) {
    
    
    NS_WARNING("couldn't clear animated value (perhaps it wasn't set?)");
  }
  FlushChangesToTargetAttr();
}

void
nsSMILMappedAttribute::FlushChangesToTargetAttr() const
{
  
  mElement->DeleteProperty(SMIL_MAPPED_ATTR_ANIMVAL,
                           SMIL_MAPPED_ATTR_STYLERULE_ATOM);
  nsIDocument* doc = mElement->GetCurrentDoc();

  
  if (doc) {
    nsIPresShell* shell = doc->GetPrimaryShell();
    if (shell) {
      shell->RestyleForAnimation(mElement);
    }
  }
}

already_AddRefed<nsIAtom>
nsSMILMappedAttribute::GetAttrNameAtom() const
{
  return do_GetAtom(nsCSSProps::GetStringValue(mPropID));
}
