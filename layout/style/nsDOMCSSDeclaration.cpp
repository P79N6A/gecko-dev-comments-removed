






#include "nsDOMCSSDeclaration.h"
#include "nsIDOMCSSRule.h"
#include "nsCSSParser.h"
#include "mozilla/css/Loader.h"
#include "nsCSSStyleSheet.h"
#include "nsIStyleRule.h"
#include "mozilla/css/Rule.h"
#include "mozilla/css/Declaration.h"
#include "nsCSSProps.h"
#include "nsCOMPtr.h"
#include "nsIURL.h"
#include "nsReadableUtils.h"
#include "nsIPrincipal.h"
#include "nsDOMClassInfoID.h"
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

  css::Declaration* decl = GetCSSDeclaration(false);

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

  return ParsePropertyValue(aPropID, aValue, false);
}


NS_IMETHODIMP
nsDOMCSSDeclaration::GetCssText(nsAString& aCssText)
{
  css::Declaration* decl = GetCSSDeclaration(false);
  aCssText.Truncate();

  if (decl) {
    decl->ToString(aCssText);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDOMCSSDeclaration::SetCssText(const nsAString& aCssText)
{
  
  
  css::Declaration* olddecl = GetCSSDeclaration(true);
  if (!olddecl) {
    return NS_ERROR_FAILURE;
  }

  CSSParsingEnvironment env;
  GetCSSParsingEnvironment(env);
  if (!env.mPrincipal) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  
  
  
  
  
  mozAutoDocConditionalContentUpdateBatch autoUpdate(DocToUpdate(), true);

  nsAutoPtr<css::Declaration> decl(new css::Declaration());
  decl->InitializeEmpty();
  nsCSSParser cssParser(env.mCSSLoader);
  bool changed;
  nsresult result = cssParser.ParseDeclarations(aCssText, env.mSheetURI,
                                                env.mBaseURI,
                                                env.mPrincipal, decl, &changed);
  if (NS_FAILED(result) || !changed) {
    return result;
  }

  return SetCSSDeclaration(decl.forget());
}

NS_IMETHODIMP
nsDOMCSSDeclaration::GetLength(uint32_t* aLength)
{
  css::Declaration* decl = GetCSSDeclaration(false);

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

  
  *aReturn = nullptr;

  return NS_OK;
}

NS_IMETHODIMP
nsDOMCSSDeclaration::Item(uint32_t aIndex, nsAString& aReturn)
{
  css::Declaration* decl = GetCSSDeclaration(false);

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
  const nsCSSProperty propID = nsCSSProps::LookupProperty(aPropertyName,
                                                          nsCSSProps::eEnabled);
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
  css::Declaration* decl = GetCSSDeclaration(false);

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
  
  nsCSSProperty propID = nsCSSProps::LookupProperty(aPropertyName,
                                                    nsCSSProps::eEnabled);
  if (propID == eCSSProperty_UNKNOWN) {
    return NS_OK;
  }

  if (aValue.IsEmpty()) {
    
    
    
    return RemoveProperty(propID);
  }

  if (aPriority.IsEmpty()) {
    return ParsePropertyValue(propID, aValue, false);
  }

  if (aPriority.EqualsLiteral("important")) {
    return ParsePropertyValue(propID, aValue, true);
  }

  
  return NS_OK;
}

NS_IMETHODIMP
nsDOMCSSDeclaration::RemoveProperty(const nsAString& aPropertyName,
                                    nsAString& aReturn)
{
  const nsCSSProperty propID = nsCSSProps::LookupProperty(aPropertyName,
                                                          nsCSSProps::eEnabled);
  if (propID == eCSSProperty_UNKNOWN) {
    aReturn.Truncate();
    return NS_OK;
  }

  nsresult rv = GetPropertyValue(propID, aReturn);
  NS_ENSURE_SUCCESS(rv, rv);

  return RemoveProperty(propID);
}

 void
nsDOMCSSDeclaration::GetCSSParsingEnvironmentForRule(css::Rule* aRule,
                                                     CSSParsingEnvironment& aCSSParseEnv)
{
  nsIStyleSheet* sheet = aRule ? aRule->GetStyleSheet() : nullptr;
  nsRefPtr<nsCSSStyleSheet> cssSheet(do_QueryObject(sheet));
  if (!cssSheet) {
    aCSSParseEnv.mPrincipal = nullptr;
    return;
  }

  nsIDocument* document = sheet->GetOwningDocument();
  aCSSParseEnv.mSheetURI = sheet->GetSheetURI();
  aCSSParseEnv.mBaseURI = sheet->GetBaseURI();
  aCSSParseEnv.mPrincipal = cssSheet->Principal();
  aCSSParseEnv.mCSSLoader = document ? document->CSSLoader() : nullptr;
}

nsresult
nsDOMCSSDeclaration::ParsePropertyValue(const nsCSSProperty aPropID,
                                        const nsAString& aPropValue,
                                        bool aIsImportant)
{
  css::Declaration* olddecl = GetCSSDeclaration(true);
  if (!olddecl) {
    return NS_ERROR_FAILURE;
  }

  CSSParsingEnvironment env;
  GetCSSParsingEnvironment(env);
  if (!env.mPrincipal) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  
  
  
  
  
  mozAutoDocConditionalContentUpdateBatch autoUpdate(DocToUpdate(), true);
  css::Declaration* decl = olddecl->EnsureMutable();

  nsCSSParser cssParser(env.mCSSLoader);
  bool changed;
  nsresult result = cssParser.ParseProperty(aPropID, aPropValue, env.mSheetURI,
                                            env.mBaseURI, env.mPrincipal, decl,
                                            &changed, aIsImportant);
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
  css::Declaration* decl = GetCSSDeclaration(false);
  if (!decl) {
    return NS_OK; 
  }

  
  
  
  
  
  mozAutoDocConditionalContentUpdateBatch autoUpdate(DocToUpdate(), true);

  decl = decl->EnsureMutable();
  decl->RemoveProperty(aPropID);
  return SetCSSDeclaration(decl);
}



#define CSS_PROP_DOMPROP_PREFIXED(prop_) Moz ## prop_
#define CSS_PROP(name_, id_, method_, flags_, pref_, parsevariant_,          \
                 kwtable_, stylestruct_, stylestructoffset_, animtype_)      \
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
#define CSS_PROP_SHORTHAND(name_, id_, method_, flags_, pref_)  \
  CSS_PROP(name_, id_, method_, flags_, pref_, X, X, X, X, X)
#include "nsCSSPropList.h"

#define CSS_PROP_ALIAS(aliasname_, propid_, aliasmethod_, pref_)  \
  CSS_PROP(X, propid_, aliasmethod_, X, pref_, X, X, X, X, X)
#include "nsCSSPropAliasList.h"
#undef CSS_PROP_ALIAS

#undef CSS_PROP_SHORTHAND
#undef CSS_PROP_LIST_EXCLUDE_INTERNAL
#undef CSS_PROP
#undef CSS_PROP_DOMPROP_PREFIXED
