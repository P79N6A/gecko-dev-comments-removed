





































#include "ChangeCSSInlineStyleTxn.h"
#include "nsIDOMElement.h"
#include "nsIDOMCSSStyleDeclaration.h"
#include "nsIDOMElementCSSInlineStyle.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "nsCRT.h"
#include "nsIAtom.h"

#define kNullCh (PRUnichar('\0'))



PRBool
ChangeCSSInlineStyleTxn::ValueIncludes(const nsAString &aValueList, const nsAString &aValue, PRBool aCaseSensitive)
{
  nsAutoString  valueList(aValueList);
  PRBool result = PR_FALSE;

  valueList.Append(kNullCh);  

  PRUnichar *value = ToNewUnicode(aValue);
  PRUnichar *start = valueList.BeginWriting();
  PRUnichar *end   = start;

  while (kNullCh != *start) {
    while ((kNullCh != *start) && nsCRT::IsAsciiSpace(*start)) {  
      start++;
    }
    end = start;

    while ((kNullCh != *end) && (PR_FALSE == nsCRT::IsAsciiSpace(*end))) { 
      end++;
    }
    *end = kNullCh; 

    if (start < end) {
      if (aCaseSensitive) {
        if (!nsCRT::strcmp(value, start)) {
          result = PR_TRUE;
          break;
        }
      }
      else {
        if (nsDependentString(value).Equals(nsDependentString(start),
                                            nsCaseInsensitiveStringComparator())) {
          result = PR_TRUE;
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

  PRUnichar *start = classStr.BeginWriting();
  PRUnichar *end   = start;

  while (kNullCh != *start) {
    while ((kNullCh != *start) && nsCRT::IsAsciiSpace(*start)) {  
      start++;
    }
    end = start;

    while ((kNullCh != *end) && (PR_FALSE == nsCRT::IsAsciiSpace(*end))) { 
      end++;
    }
    *end = kNullCh; 

    if (start < end) {
      if (!aRemoveValue.Equals(start)) {
        outString.Append(start);
        outString.Append(PRUnichar(' '));
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
                                            PRBool aRemoveProperty)
{
  NS_ASSERTION(aEditor && aElement, "bad arg");
  if (!aEditor || !aElement) { return NS_ERROR_NULL_POINTER; }

  mEditor = aEditor;
  mElement = do_QueryInterface(aElement);
  mProperty = aProperty;
  NS_ADDREF(mProperty);
  mValue.Assign(aValue);
  mRemoveProperty = aRemoveProperty;
  mUndoAttributeWasSet = PR_FALSE;
  mRedoAttributeWasSet = PR_FALSE;
  mUndoValue.Truncate();
  mRedoValue.Truncate();
  return NS_OK;
}

NS_IMETHODIMP ChangeCSSInlineStyleTxn::DoTransaction(void)
{
  NS_ASSERTION(mEditor && mElement, "bad state");
  if (!mEditor || !mElement) { return NS_ERROR_NOT_INITIALIZED; }

  nsCOMPtr<nsIDOMElementCSSInlineStyle> inlineStyles = do_QueryInterface(mElement);
  if (!inlineStyles) return NS_ERROR_NULL_POINTER;

  nsCOMPtr<nsIDOMCSSStyleDeclaration> cssDecl;
  nsresult result = inlineStyles->GetStyle(getter_AddRefs(cssDecl));
  if (NS_FAILED(result)) return result;
  if (!cssDecl) return NS_ERROR_NULL_POINTER;

  nsAutoString propertyNameString;
  mProperty->ToString(propertyNameString);

  NS_NAMED_LITERAL_STRING(styleAttr, "style");
  result = mElement->HasAttribute(styleAttr, &mUndoAttributeWasSet);
  if (NS_FAILED(result)) return result;

  nsAutoString values;
  result = cssDecl->GetPropertyValue(propertyNameString, values);
  if (NS_FAILED(result)) return result;     
  mUndoValue.Assign(values);

  
  
  PRBool multiple = AcceptsMoreThanOneValue(mProperty);
  
  if (mRemoveProperty) {
    nsAutoString returnString;
    if (multiple) {
      
      

      
      
      RemoveValueFromListOfValues(values, NS_LITERAL_STRING("none"));
      RemoveValueFromListOfValues(values, mValue);
      if (values.IsEmpty()) {
        result = cssDecl->RemoveProperty(propertyNameString, returnString);
        if (NS_FAILED(result)) return result;     
      }
      else {
        nsAutoString priority;
        result = cssDecl->GetPropertyPriority(propertyNameString, priority);
        if (NS_FAILED(result)) return result;     
        result = cssDecl->SetProperty(propertyNameString, values,
                                      priority);
        if (NS_FAILED(result)) return result;     
      }
    }
    else {
      result = cssDecl->RemoveProperty(propertyNameString, returnString);
      if (NS_FAILED(result)) return result;     
    }
  }
  else {
    nsAutoString priority;
    result = cssDecl->GetPropertyPriority(propertyNameString, priority);
    if (NS_FAILED(result)) return result;
    if (multiple) {
      
      

      
      
      AddValueToMultivalueProperty(values, mValue);
    }
    else
      values.Assign(mValue);
    result = cssDecl->SetProperty(propertyNameString, values,
                                  priority);
    if (NS_FAILED(result)) return result;     
  }

  
  PRUint32 length;
  result = cssDecl->GetLength(&length);
  if (NS_FAILED(result)) return result;     
  if (!length) {
    result = mElement->RemoveAttribute(styleAttr);
    if (NS_FAILED(result)) return result;     
  }
  else
    mRedoAttributeWasSet = PR_TRUE;

  return cssDecl->GetPropertyValue(propertyNameString, mRedoValue);
}

nsresult ChangeCSSInlineStyleTxn::SetStyle(PRBool aAttributeWasSet,
                                           nsAString & aValue)
{
  NS_ASSERTION(mEditor && mElement, "bad state");
  if (!mEditor || !mElement) { return NS_ERROR_NOT_INITIALIZED; }

  nsresult result;
  if (aAttributeWasSet) {
    
    nsAutoString propertyNameString;
    mProperty->ToString(propertyNameString);

    nsCOMPtr<nsIDOMElementCSSInlineStyle> inlineStyles = do_QueryInterface(mElement);
    if (!inlineStyles) return NS_ERROR_NULL_POINTER;
    nsCOMPtr<nsIDOMCSSStyleDeclaration> cssDecl;
    result = inlineStyles->GetStyle(getter_AddRefs(cssDecl));
    if (NS_FAILED(result)) return result;
    if (!cssDecl) return NS_ERROR_NULL_POINTER;

    if (aValue.IsEmpty()) {
      
      nsAutoString returnString;
      result = cssDecl->RemoveProperty(propertyNameString, returnString);
    }
    else {
      
      nsAutoString priority;
      result = cssDecl->GetPropertyPriority(propertyNameString, priority);
      if (NS_FAILED(result)) return result;
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


PRBool
ChangeCSSInlineStyleTxn::AcceptsMoreThanOneValue(nsIAtom *aCSSProperty)
{
  nsIAtom * textDecorationAtom = NS_NewAtom("text-decoration");
  PRBool res = (textDecorationAtom == aCSSProperty);
  NS_IF_RELEASE(textDecorationAtom);
  return res;
}


NS_IMETHODIMP
ChangeCSSInlineStyleTxn::AddValueToMultivalueProperty(nsAString & aValues, const nsAString & aNewValue)
{
  if (aValues.IsEmpty()
      || aValues.LowerCaseEqualsLiteral("none")) {
    
    aValues.Assign(aNewValue);
  }
  else if (!ValueIncludes(aValues, aNewValue, PR_FALSE)) {
    
    aValues.Append(PRUnichar(' '));
    aValues.Append(aNewValue);
  }
  return NS_OK;
}
