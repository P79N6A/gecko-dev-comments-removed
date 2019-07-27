




#include "ChangeStyleTxn.h"

#include "mozilla/dom/Element.h"        
#include "nsAString.h"                  
#include "nsCRT.h"                      
#include "nsDebug.h"                    
#include "nsError.h"                    
#include "nsGkAtoms.h"                  
#include "nsIDOMCSSStyleDeclaration.h"  
#include "nsIDOMElementCSSInlineStyle.h" 
#include "nsLiteralString.h"            
#include "nsReadableUtils.h"            
#include "nsString.h"                   
#include "nsUnicharUtils.h"             

using namespace mozilla;
using namespace mozilla::dom;

#define kNullCh (char16_t('\0'))

NS_IMPL_CYCLE_COLLECTION_INHERITED(ChangeStyleTxn, EditTxn, mElement)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(ChangeStyleTxn)
NS_INTERFACE_MAP_END_INHERITING(EditTxn)

NS_IMPL_ADDREF_INHERITED(ChangeStyleTxn, EditTxn)
NS_IMPL_RELEASE_INHERITED(ChangeStyleTxn, EditTxn)

ChangeStyleTxn::~ChangeStyleTxn()
{
}



bool
ChangeStyleTxn::ValueIncludes(const nsAString &aValueList,
                              const nsAString &aValue)
{
  nsAutoString valueList(aValueList);
  bool result = false;

  
  valueList.Append(kNullCh);

  char16_t* value = ToNewUnicode(aValue);
  char16_t* start = valueList.BeginWriting();
  char16_t* end = start;

  while (kNullCh != *start) {
    while (kNullCh != *start && nsCRT::IsAsciiSpace(*start)) {
      
      start++;
    }
    end = start;

    while (kNullCh != *end && !nsCRT::IsAsciiSpace(*end)) {
      
      end++;
    }
    
    *end = kNullCh;

    if (start < end) {
      if (nsDependentString(value).Equals(nsDependentString(start),
            nsCaseInsensitiveStringComparator())) {
        result = true;
        break;
      }
    }
    start = ++end;
  }
  free(value);
  return result;
}



void
ChangeStyleTxn::RemoveValueFromListOfValues(nsAString& aValues,
                                            const nsAString& aRemoveValue)
{
  nsAutoString classStr(aValues);
  nsAutoString outString;
  
  classStr.Append(kNullCh);

  char16_t* start = classStr.BeginWriting();
  char16_t* end = start;

  while (kNullCh != *start) {
    while (kNullCh != *start && nsCRT::IsAsciiSpace(*start)) {
      
      start++;
    }
    end = start;

    while (kNullCh != *end && !nsCRT::IsAsciiSpace(*end)) {
      
      end++;
    }
    
    *end = kNullCh;

    if (start < end && !aRemoveValue.Equals(start)) {
      outString.Append(start);
      outString.Append(char16_t(' '));
    }

    start = ++end;
  }
  aValues.Assign(outString);
}

ChangeStyleTxn::ChangeStyleTxn(Element& aElement, nsIAtom& aProperty,
                               const nsAString& aValue,
                               EChangeType aChangeType)
  : EditTxn()
  , mElement(&aElement)
  , mProperty(&aProperty)
  , mValue(aValue)
  , mRemoveProperty(aChangeType == eRemove)
  , mUndoValue()
  , mRedoValue()
  , mUndoAttributeWasSet(false)
  , mRedoAttributeWasSet(false)
{
}

NS_IMETHODIMP
ChangeStyleTxn::DoTransaction()
{
  nsCOMPtr<nsIDOMElementCSSInlineStyle> inlineStyles =
    do_QueryInterface(mElement);
  NS_ENSURE_TRUE(inlineStyles, NS_ERROR_NULL_POINTER);

  nsCOMPtr<nsIDOMCSSStyleDeclaration> cssDecl;
  nsresult result = inlineStyles->GetStyle(getter_AddRefs(cssDecl));
  NS_ENSURE_SUCCESS(result, result);
  NS_ENSURE_TRUE(cssDecl, NS_ERROR_NULL_POINTER);

  nsAutoString propertyNameString;
  mProperty->ToString(propertyNameString);

  mUndoAttributeWasSet = mElement->HasAttr(kNameSpaceID_None,
                                           nsGkAtoms::style);

  nsAutoString values;
  result = cssDecl->GetPropertyValue(propertyNameString, values);
  NS_ENSURE_SUCCESS(result, result);
  mUndoValue.Assign(values);

  
  bool multiple = AcceptsMoreThanOneValue(*mProperty);

  if (mRemoveProperty) {
    nsAutoString returnString;
    if (multiple) {
      

      
      
      
      RemoveValueFromListOfValues(values, NS_LITERAL_STRING("none"));
      RemoveValueFromListOfValues(values, mValue);
      if (values.IsEmpty()) {
        result = cssDecl->RemoveProperty(propertyNameString, returnString);
        NS_ENSURE_SUCCESS(result, result);
      } else {
        nsAutoString priority;
        result = cssDecl->GetPropertyPriority(propertyNameString, priority);
        NS_ENSURE_SUCCESS(result, result);
        result = cssDecl->SetProperty(propertyNameString, values,
                                      priority);
        NS_ENSURE_SUCCESS(result, result);
      }
    } else {
      result = cssDecl->RemoveProperty(propertyNameString, returnString);
      NS_ENSURE_SUCCESS(result, result);
    }
  } else {
    nsAutoString priority;
    result = cssDecl->GetPropertyPriority(propertyNameString, priority);
    NS_ENSURE_SUCCESS(result, result);
    if (multiple) {
      

      
      
      
      AddValueToMultivalueProperty(values, mValue);
    } else {
      values.Assign(mValue);
    }
    result = cssDecl->SetProperty(propertyNameString, values,
                                  priority);
    NS_ENSURE_SUCCESS(result, result);
  }

  
  uint32_t length;
  result = cssDecl->GetLength(&length);
  NS_ENSURE_SUCCESS(result, result);
  if (!length) {
    result = mElement->UnsetAttr(kNameSpaceID_None, nsGkAtoms::style, true);
    NS_ENSURE_SUCCESS(result, result);
  } else {
    mRedoAttributeWasSet = true;
  }

  return cssDecl->GetPropertyValue(propertyNameString, mRedoValue);
}

nsresult
ChangeStyleTxn::SetStyle(bool aAttributeWasSet, nsAString& aValue)
{
  nsresult result = NS_OK;
  if (aAttributeWasSet) {
    
    nsAutoString propertyNameString;
    mProperty->ToString(propertyNameString);

    nsCOMPtr<nsIDOMElementCSSInlineStyle> inlineStyles =
      do_QueryInterface(mElement);
    NS_ENSURE_TRUE(inlineStyles, NS_ERROR_NULL_POINTER);
    nsCOMPtr<nsIDOMCSSStyleDeclaration> cssDecl;
    result = inlineStyles->GetStyle(getter_AddRefs(cssDecl));
    NS_ENSURE_SUCCESS(result, result);
    NS_ENSURE_TRUE(cssDecl, NS_ERROR_NULL_POINTER);

    if (aValue.IsEmpty()) {
      
      nsAutoString returnString;
      result = cssDecl->RemoveProperty(propertyNameString, returnString);
    } else {
      
      nsAutoString priority;
      result = cssDecl->GetPropertyPriority(propertyNameString, priority);
      NS_ENSURE_SUCCESS(result, result);
      result = cssDecl->SetProperty(propertyNameString, aValue, priority);
    }
  } else {
    result = mElement->UnsetAttr(kNameSpaceID_None, nsGkAtoms::style, true);
  }

  return result;
}

NS_IMETHODIMP
ChangeStyleTxn::UndoTransaction()
{
  return SetStyle(mUndoAttributeWasSet, mUndoValue);
}

NS_IMETHODIMP
ChangeStyleTxn::RedoTransaction()
{
  return SetStyle(mRedoAttributeWasSet, mRedoValue);
}

NS_IMETHODIMP
ChangeStyleTxn::GetTxnDescription(nsAString& aString)
{
  aString.AssignLiteral("ChangeStyleTxn: [mRemoveProperty == ");

  if (mRemoveProperty) {
    aString.AppendLiteral("true] ");
  } else {
    aString.AppendLiteral("false] ");
  }
  aString += nsDependentAtomString(mProperty);
  return NS_OK;
}


bool
ChangeStyleTxn::AcceptsMoreThanOneValue(nsIAtom& aCSSProperty)
{
  return &aCSSProperty == nsGkAtoms::text_decoration;
}


void
ChangeStyleTxn::AddValueToMultivalueProperty(nsAString& aValues,
                                             const nsAString& aNewValue)
{
  if (aValues.IsEmpty() || aValues.LowerCaseEqualsLiteral("none")) {
    aValues.Assign(aNewValue);
  } else if (!ValueIncludes(aValues, aNewValue)) {
    
    aValues.Append(char16_t(' '));
    aValues.Append(aNewValue);
  }
}
