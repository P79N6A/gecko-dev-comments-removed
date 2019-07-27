




#include "ChangeCSSInlineStyleTxn.h"
#include "nsAString.h"                  
#include "nsCRT.h"                      
#include "nsDebug.h"                    
#include "nsError.h"                    
#include "nsGkAtoms.h"                  
#include "nsIAtom.h"                    
#include "nsIDOMCSSStyleDeclaration.h"  
#include "nsIDOMElement.h"              
#include "nsIDOMElementCSSInlineStyle.h"
#include "nsISupportsImpl.h"            
#include "nsISupportsUtils.h"           
#include "nsLiteralString.h"            
#include "nsReadableUtils.h"            
#include "nsString.h"                   
#include "nsUnicharUtils.h"
#include "nsXPCOM.h"                    

class nsIEditor;

#define kNullCh (char16_t('\0'))

NS_IMPL_CYCLE_COLLECTION_INHERITED(ChangeCSSInlineStyleTxn, EditTxn,
                                   mElement)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(ChangeCSSInlineStyleTxn)
NS_INTERFACE_MAP_END_INHERITING(EditTxn)



bool
ChangeCSSInlineStyleTxn::ValueIncludes(const nsAString &aValueList, const nsAString &aValue, bool aCaseSensitive)
{
  nsAutoString  valueList(aValueList);
  bool result = false;

  valueList.Append(kNullCh);  

  char16_t *value = ToNewUnicode(aValue);
  char16_t *start = valueList.BeginWriting();
  char16_t *end   = start;

  while (kNullCh != *start) {
    while ((kNullCh != *start) && nsCRT::IsAsciiSpace(*start)) {  
      start++;
    }
    end = start;

    while ((kNullCh != *end) && (false == nsCRT::IsAsciiSpace(*end))) { 
      end++;
    }
    *end = kNullCh; 

    if (start < end) {
      if (aCaseSensitive) {
        if (!nsCRT::strcmp(value, start)) {
          result = true;
          break;
        }
      }
      else {
        if (nsDependentString(value).Equals(nsDependentString(start),
                                            nsCaseInsensitiveStringComparator())) {
          result = true;
          break;
        }
      }
    }
    start = ++end;
  }
  NS_Free(value);
  return result;
}


void
ChangeCSSInlineStyleTxn::RemoveValueFromListOfValues(nsAString & aValues, const nsAString  & aRemoveValue)
{
  nsAutoString  classStr(aValues);  
  nsAutoString  outString;
  classStr.Append(kNullCh);  

  char16_t *start = classStr.BeginWriting();
  char16_t *end   = start;

  while (kNullCh != *start) {
    while ((kNullCh != *start) && nsCRT::IsAsciiSpace(*start)) {  
      start++;
    }
    end = start;

    while ((kNullCh != *end) && (false == nsCRT::IsAsciiSpace(*end))) { 
      end++;
    }
    *end = kNullCh; 

    if (start < end) {
      if (!aRemoveValue.Equals(start)) {
        outString.Append(start);
        outString.Append(char16_t(' '));
      }
    }

    start = ++end;
  }
  aValues.Assign(outString);
}

ChangeCSSInlineStyleTxn::ChangeCSSInlineStyleTxn()
  : EditTxn()
{
}

NS_IMETHODIMP ChangeCSSInlineStyleTxn::Init(nsIEditor      *aEditor,
                                            nsIDOMElement  *aElement,
                                            nsIAtom        *aProperty,
                                            const nsAString& aValue,
                                            bool aRemoveProperty)
{
  NS_ASSERTION(aEditor && aElement, "bad arg");
  if (!aEditor || !aElement) { return NS_ERROR_NULL_POINTER; }

  mEditor = aEditor;
  mElement = do_QueryInterface(aElement);
  mProperty = aProperty;
  NS_ADDREF(mProperty);
  mValue.Assign(aValue);
  mRemoveProperty = aRemoveProperty;
  mUndoAttributeWasSet = false;
  mRedoAttributeWasSet = false;
  mUndoValue.Truncate();
  mRedoValue.Truncate();
  return NS_OK;
}

NS_IMETHODIMP ChangeCSSInlineStyleTxn::DoTransaction(void)
{
  NS_ASSERTION(mEditor && mElement, "bad state");
  if (!mEditor || !mElement) { return NS_ERROR_NOT_INITIALIZED; }

  nsCOMPtr<nsIDOMElementCSSInlineStyle> inlineStyles = do_QueryInterface(mElement);
  NS_ENSURE_TRUE(inlineStyles, NS_ERROR_NULL_POINTER);

  nsCOMPtr<nsIDOMCSSStyleDeclaration> cssDecl;
  nsresult result = inlineStyles->GetStyle(getter_AddRefs(cssDecl));
  NS_ENSURE_SUCCESS(result, result);
  NS_ENSURE_TRUE(cssDecl, NS_ERROR_NULL_POINTER);

  nsAutoString propertyNameString;
  mProperty->ToString(propertyNameString);

  NS_NAMED_LITERAL_STRING(styleAttr, "style");
  result = mElement->HasAttribute(styleAttr, &mUndoAttributeWasSet);
  NS_ENSURE_SUCCESS(result, result);

  nsAutoString values;
  result = cssDecl->GetPropertyValue(propertyNameString, values);
  NS_ENSURE_SUCCESS(result, result);     
  mUndoValue.Assign(values);

  
  
  bool multiple = AcceptsMoreThanOneValue(mProperty);
  
  if (mRemoveProperty) {
    nsAutoString returnString;
    if (multiple) {
      
      

      
      
      RemoveValueFromListOfValues(values, NS_LITERAL_STRING("none"));
      RemoveValueFromListOfValues(values, mValue);
      if (values.IsEmpty()) {
        result = cssDecl->RemoveProperty(propertyNameString, returnString);
        NS_ENSURE_SUCCESS(result, result);     
      }
      else {
        nsAutoString priority;
        result = cssDecl->GetPropertyPriority(propertyNameString, priority);
        NS_ENSURE_SUCCESS(result, result);     
        result = cssDecl->SetProperty(propertyNameString, values,
                                      priority);
        NS_ENSURE_SUCCESS(result, result);     
      }
    }
    else {
      result = cssDecl->RemoveProperty(propertyNameString, returnString);
      NS_ENSURE_SUCCESS(result, result);     
    }
  }
  else {
    nsAutoString priority;
    result = cssDecl->GetPropertyPriority(propertyNameString, priority);
    NS_ENSURE_SUCCESS(result, result);
    if (multiple) {
      
      

      
      
      AddValueToMultivalueProperty(values, mValue);
    }
    else
      values.Assign(mValue);
    result = cssDecl->SetProperty(propertyNameString, values,
                                  priority);
    NS_ENSURE_SUCCESS(result, result);     
  }

  
  uint32_t length;
  result = cssDecl->GetLength(&length);
  NS_ENSURE_SUCCESS(result, result);     
  if (!length) {
    result = mElement->RemoveAttribute(styleAttr);
    NS_ENSURE_SUCCESS(result, result);     
  }
  else
    mRedoAttributeWasSet = true;

  return cssDecl->GetPropertyValue(propertyNameString, mRedoValue);
}

nsresult ChangeCSSInlineStyleTxn::SetStyle(bool aAttributeWasSet,
                                           nsAString & aValue)
{
  NS_ASSERTION(mEditor && mElement, "bad state");
  if (!mEditor || !mElement) { return NS_ERROR_NOT_INITIALIZED; }

  nsresult result = NS_OK;
  if (aAttributeWasSet) {
    
    nsAutoString propertyNameString;
    mProperty->ToString(propertyNameString);

    nsCOMPtr<nsIDOMElementCSSInlineStyle> inlineStyles = do_QueryInterface(mElement);
    NS_ENSURE_TRUE(inlineStyles, NS_ERROR_NULL_POINTER);
    nsCOMPtr<nsIDOMCSSStyleDeclaration> cssDecl;
    result = inlineStyles->GetStyle(getter_AddRefs(cssDecl));
    NS_ENSURE_SUCCESS(result, result);
    NS_ENSURE_TRUE(cssDecl, NS_ERROR_NULL_POINTER);

    if (aValue.IsEmpty()) {
      
      nsAutoString returnString;
      result = cssDecl->RemoveProperty(propertyNameString, returnString);
    }
    else {
      
      nsAutoString priority;
      result = cssDecl->GetPropertyPriority(propertyNameString, priority);
      NS_ENSURE_SUCCESS(result, result);
      result = cssDecl->SetProperty(propertyNameString, aValue, priority);
    }
  }
  else
    result = mElement->RemoveAttribute(NS_LITERAL_STRING("style"));

  return result;
}

NS_IMETHODIMP ChangeCSSInlineStyleTxn::UndoTransaction(void)
{
  return SetStyle(mUndoAttributeWasSet, mUndoValue);
}

NS_IMETHODIMP ChangeCSSInlineStyleTxn::RedoTransaction(void)
{
  return SetStyle(mRedoAttributeWasSet, mRedoValue);
}

NS_IMETHODIMP ChangeCSSInlineStyleTxn::GetTxnDescription(nsAString& aString)
{
  aString.AssignLiteral("ChangeCSSInlineStyleTxn: [mRemoveProperty == ");

  if (!mRemoveProperty)
    aString.AppendLiteral("false] ");
  else
    aString.AppendLiteral("true] ");
  nsAutoString tempString;
  mProperty->ToString(tempString);
  aString += tempString;
  return NS_OK;
}


bool
ChangeCSSInlineStyleTxn::AcceptsMoreThanOneValue(nsIAtom *aCSSProperty)
{
  return aCSSProperty == nsGkAtoms::text_decoration;
}


NS_IMETHODIMP
ChangeCSSInlineStyleTxn::AddValueToMultivalueProperty(nsAString & aValues, const nsAString & aNewValue)
{
  if (aValues.IsEmpty()
      || aValues.LowerCaseEqualsLiteral("none")) {
    
    aValues.Assign(aNewValue);
  }
  else if (!ValueIncludes(aValues, aNewValue, false)) {
    
    aValues.Append(char16_t(' '));
    aValues.Append(aNewValue);
  }
  return NS_OK;
}
