







































#include "nsDOMCSSDeclaration.h"
#include "nsIDOMCSSRule.h"
#include "nsCSSParser.h"
#include "mozilla/css/Loader.h"
#include "nsCSSStyleSheet.h"
#include "nsIStyleRule.h"
#include "nsICSSRule.h"
#include "mozilla/css/Declaration.h"
#include "nsCSSProps.h"
#include "nsCOMPtr.h"
#include "nsIURL.h"
#include "nsReadableUtils.h"
#include "nsIPrincipal.h"

#include "nsContentUtils.h"
#include "mozAutoDocUpdate.h"

namespace css = mozilla::css;

nsDOMCSSDeclaration::~nsDOMCSSDeclaration()
{
}

DOMCI_DATA(CSSStyleDeclaration, nsDOMCSSDeclaration)

NS_INTERFACE_TABLE_HEAD(nsDOMCSSDeclaration)
  NS_INTERFACE_TABLE3(nsDOMCSSDeclaration,
                      nsICSSDeclaration,
                      nsIDOMCSSStyleDeclaration,
                      nsIDOMCSS2Properties)
  NS_INTERFACE_TABLE_TO_MAP_SEGUE
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(CSSStyleDeclaration)
NS_INTERFACE_MAP_END

NS_IMETHODIMP
nsDOMCSSDeclaration::GetPropertyValue(const nsCSSProperty aPropID,
                                      nsAString& aValue)
{
  NS_PRECONDITION(aPropID != eCSSProperty_UNKNOWN,
                  "Should never pass eCSSProperty_UNKNOWN around");

  css::Declaration* decl = GetCSSDeclaration(PR_FALSE);

  aValue.Truncate();
  if (decl) {
    decl->GetValue(aPropID, aValue);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsDOMCSSDeclaration::SetPropertyValue(const nsCSSProperty aPropID,
                                      const nsAString& aValue)
{
  if (aValue.IsEmpty()) {
    
    
    return RemoveProperty(aPropID);
  }

  return ParsePropertyValue(aPropID, aValue, PR_FALSE);
}


NS_IMETHODIMP
nsDOMCSSDeclaration::GetCssText(nsAString& aCssText)
{
  css::Declaration* decl = GetCSSDeclaration(PR_FALSE);
  aCssText.Truncate();

  if (decl) {
    decl->ToString(aCssText);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDOMCSSDeclaration::SetCssText(const nsAString& aCssText)
{
  
  
  css::Declaration* olddecl = GetCSSDeclaration(PR_TRUE);
  if (!olddecl) {
    return NS_ERROR_FAILURE;
  }

  nsresult result;
  nsRefPtr<css::Loader> cssLoader;
  nsCOMPtr<nsIURI> baseURI, sheetURI;
  nsCOMPtr<nsIPrincipal> sheetPrincipal;

  result = GetCSSParsingEnvironment(getter_AddRefs(sheetURI),
                                    getter_AddRefs(baseURI),
                                    getter_AddRefs(sheetPrincipal),
                                    getter_AddRefs(cssLoader));

  if (NS_FAILED(result)) {
    return result;
  }

  
  
  
  
  
  mozAutoDocConditionalContentUpdateBatch autoUpdate(DocToUpdate(), PR_TRUE);

  nsAutoPtr<css::Declaration> decl(new css::Declaration());
  decl->InitializeEmpty();
  nsCSSParser cssParser(cssLoader);
  PRBool changed;
  result = cssParser.ParseDeclarations(aCssText, sheetURI, baseURI,
                                       sheetPrincipal, decl, &changed);
  if (NS_FAILED(result) || !changed) {
    return result;
  }

  return SetCSSDeclaration(decl.forget());
}

NS_IMETHODIMP
nsDOMCSSDeclaration::GetLength(PRUint32* aLength)
{
  css::Declaration* decl = GetCSSDeclaration(PR_FALSE);

  if (decl) {
    *aLength = decl->Count();
  } else {
    *aLength = 0;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDOMCSSDeclaration::GetPropertyCSSValue(const nsAString& aPropertyName,
                                         nsIDOMCSSValue** aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);

  
  *aReturn = nsnull;

  return NS_OK;
}

NS_IMETHODIMP
nsDOMCSSDeclaration::Item(PRUint32 aIndex, nsAString& aReturn)
{
  css::Declaration* decl = GetCSSDeclaration(PR_FALSE);

  aReturn.SetLength(0);
  if (decl) {
    decl->GetNthProperty(aIndex, aReturn);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDOMCSSDeclaration::GetPropertyValue(const nsAString& aPropertyName,
                                      nsAString& aReturn)
{
  const nsCSSProperty propID = nsCSSProps::LookupProperty(aPropertyName);
  if (propID == eCSSProperty_UNKNOWN) {
    aReturn.Truncate();
    return NS_OK;
  }

  return GetPropertyValue(propID, aReturn);
}

NS_IMETHODIMP
nsDOMCSSDeclaration::GetPropertyPriority(const nsAString& aPropertyName,
                                         nsAString& aReturn)
{
  css::Declaration* decl = GetCSSDeclaration(PR_FALSE);

  aReturn.Truncate();
  if (decl && decl->GetValueIsImportant(aPropertyName)) {
    aReturn.AssignLiteral("important");
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDOMCSSDeclaration::SetProperty(const nsAString& aPropertyName,
                                 const nsAString& aValue,
                                 const nsAString& aPriority)
{
  
  nsCSSProperty propID = nsCSSProps::LookupProperty(aPropertyName);
  if (propID == eCSSProperty_UNKNOWN) {
    return NS_OK;
  }

  if (aValue.IsEmpty()) {
    
    
    
    return RemoveProperty(propID);
  }

  if (aPriority.IsEmpty()) {
    return ParsePropertyValue(propID, aValue, PR_FALSE);
  }

  if (aPriority.EqualsLiteral("important")) {
    return ParsePropertyValue(propID, aValue, PR_TRUE);
  }

  
  return NS_OK;
}

NS_IMETHODIMP
nsDOMCSSDeclaration::RemoveProperty(const nsAString& aPropertyName,
                                    nsAString& aReturn)
{
  const nsCSSProperty propID = nsCSSProps::LookupProperty(aPropertyName);
  if (propID == eCSSProperty_UNKNOWN) {
    aReturn.Truncate();
    return NS_OK;
  }

  nsresult rv = GetPropertyValue(propID, aReturn);
  NS_ENSURE_SUCCESS(rv, rv);

  return RemoveProperty(propID);
}

 nsresult
nsDOMCSSDeclaration::GetCSSParsingEnvironmentForRule(
                         nsICSSRule* aRule, nsIURI** aSheetURI,
                         nsIURI** aBaseURI, nsIPrincipal** aSheetPrincipal,
                         mozilla::css::Loader** aCSSLoader)
{
  
  *aSheetURI = nsnull;
  *aBaseURI = nsnull;
  *aSheetPrincipal = nsnull;
  *aCSSLoader = nsnull;

  if (aRule) {
    nsIStyleSheet* sheet = aRule->GetStyleSheet();
    if (sheet) {
      NS_IF_ADDREF(*aSheetURI = sheet->GetSheetURI());
      NS_IF_ADDREF(*aBaseURI = sheet->GetBaseURI());

      nsRefPtr<nsCSSStyleSheet> cssSheet(do_QueryObject(sheet));
      if (cssSheet) {
        NS_ADDREF(*aSheetPrincipal = cssSheet->Principal());
      }

      nsIDocument* document = sheet->GetOwningDocument();
      if (document) {
        NS_ADDREF(*aCSSLoader = document->CSSLoader());
      }
    }
  }

  nsresult result = NS_OK;
  if (!*aSheetPrincipal) {
    result = CallCreateInstance("@mozilla.org/nullprincipal;1",
                                aSheetPrincipal);
  }

  return result;
}

nsresult
nsDOMCSSDeclaration::ParsePropertyValue(const nsCSSProperty aPropID,
                                        const nsAString& aPropValue,
                                        PRBool aIsImportant)
{
  css::Declaration* olddecl = GetCSSDeclaration(PR_TRUE);
  if (!olddecl) {
    return NS_ERROR_FAILURE;
  }

  nsresult result;
  nsRefPtr<css::Loader> cssLoader;
  nsCOMPtr<nsIURI> baseURI, sheetURI;
  nsCOMPtr<nsIPrincipal> sheetPrincipal;

  result = GetCSSParsingEnvironment(getter_AddRefs(sheetURI),
                                    getter_AddRefs(baseURI),
                                    getter_AddRefs(sheetPrincipal),
                                    getter_AddRefs(cssLoader));
  if (NS_FAILED(result)) {
    return result;
  }

  
  
  
  
  
  mozAutoDocConditionalContentUpdateBatch autoUpdate(DocToUpdate(), PR_TRUE);
  css::Declaration* decl = olddecl->EnsureMutable();

  nsCSSParser cssParser(cssLoader);
  PRBool changed;
  result = cssParser.ParseProperty(aPropID, aPropValue, sheetURI, baseURI,
                                   sheetPrincipal, decl, &changed,
                                   aIsImportant);
  if (NS_FAILED(result) || !changed) {
    if (decl != olddecl) {
      delete decl;
    }
    return result;
  }

  return SetCSSDeclaration(decl);
}

nsresult
nsDOMCSSDeclaration::RemoveProperty(const nsCSSProperty aPropID)
{
  css::Declaration* decl = GetCSSDeclaration(PR_FALSE);
  if (!decl) {
    return NS_OK; 
  }

  
  
  
  
  
  mozAutoDocConditionalContentUpdateBatch autoUpdate(DocToUpdate(), PR_TRUE);

  decl = decl->EnsureMutable();
  decl->RemoveProperty(aPropID);
  return SetCSSDeclaration(decl);
}



#define CSS_PROP_DOMPROP_PREFIXED(prop_) Moz ## prop_
#define CSS_PROP(name_, id_, method_, flags_, parsevariant_, kwtable_,       \
                 stylestruct_, stylestructoffset_, animtype_)                \
  NS_IMETHODIMP                                                              \
  nsDOMCSSDeclaration::Get##method_(nsAString& aValue)                       \
  {                                                                          \
    return GetPropertyValue(eCSSProperty_##id_, aValue);                     \
  }                                                                          \
                                                                             \
  NS_IMETHODIMP                                                              \
  nsDOMCSSDeclaration::Set##method_(const nsAString& aValue)                 \
  {                                                                          \
    return SetPropertyValue(eCSSProperty_##id_, aValue);                     \
  }

#define CSS_PROP_LIST_EXCLUDE_INTERNAL
#define CSS_PROP_SHORTHAND(name_, id_, method_, flags_) \
  CSS_PROP(name_, id_, method_, flags_, X, X, X, X, X)
#include "nsCSSPropList.h"


CSS_PROP(X, opacity, MozOpacity, X, X, X, X, X, X)
CSS_PROP(X, outline, MozOutline, X, X, X, X, X, X)
CSS_PROP(X, outline_color, MozOutlineColor, X, X, X, X, X, X)
CSS_PROP(X, outline_style, MozOutlineStyle, X, X, X, X, X, X)
CSS_PROP(X, outline_width, MozOutlineWidth, X, X, X, X, X, X)
CSS_PROP(X, outline_offset, MozOutlineOffset, X, X, X, X, X, X)

#undef CSS_PROP_SHORTHAND
#undef CSS_PROP_LIST_EXCLUDE_INTERNAL
#undef CSS_PROP
#undef CSS_PROP_DOMPROP_PREFIXED
