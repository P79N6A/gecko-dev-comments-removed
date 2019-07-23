







































#include "nsDOMCSSDeclaration.h"
#include "nsIDOMCSSRule.h"
#include "nsICSSParser.h"
#include "nsICSSLoader.h"
#include "nsIStyleRule.h"
#include "nsCSSDeclaration.h"
#include "nsCSSProps.h"
#include "nsCOMPtr.h"
#include "nsIURL.h"
#include "nsReadableUtils.h"
#include "nsIPrincipal.h"

#include "nsContentUtils.h"


nsDOMCSSDeclaration::nsDOMCSSDeclaration()
  : mInner(this)
{
}

nsDOMCSSDeclaration::~nsDOMCSSDeclaration()
{
}



NS_INTERFACE_MAP_BEGIN(nsDOMCSSDeclaration)
  NS_INTERFACE_MAP_ENTRY(nsICSSDeclaration)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCSSStyleDeclaration)
  NS_INTERFACE_MAP_ENTRY_AGGREGATED(nsIDOMCSS2Properties, &mInner)
  NS_INTERFACE_MAP_ENTRY_AGGREGATED(nsIDOMNSCSS2Properties, &mInner)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMCSSStyleDeclaration)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(CSSStyleDeclaration)
NS_INTERFACE_MAP_END

NS_IMETHODIMP
nsDOMCSSDeclaration::GetPropertyValue(const nsCSSProperty aPropID,
                                      nsAString& aValue)
{
  NS_PRECONDITION(aPropID != eCSSProperty_UNKNOWN,
                  "Should never pass eCSSProperty_UNKNOWN around");
  
  nsCSSDeclaration *decl;
  nsresult result = GetCSSDeclaration(&decl, PR_FALSE);

  aValue.Truncate();
  if (decl) {
    result = decl->GetValue(aPropID, aValue);
  }

  return result;
}

NS_IMETHODIMP
nsDOMCSSDeclaration::SetPropertyValue(const nsCSSProperty aPropID,
                                      const nsAString& aValue)
{
  if (aValue.IsEmpty()) {
    
    
    return RemoveProperty(aPropID);
  }

  return ParsePropertyValue(aPropID, aValue);
}


NS_IMETHODIMP
nsDOMCSSDeclaration::GetCssText(nsAString& aCssText)
{
  nsCSSDeclaration* decl;
  aCssText.Truncate();
  GetCSSDeclaration(&decl, PR_FALSE);

  if (decl) {
    decl->ToString(aCssText);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDOMCSSDeclaration::SetCssText(const nsAString& aCssText)
{
  return ParseDeclaration(aCssText, PR_FALSE, PR_TRUE);
}

NS_IMETHODIMP
nsDOMCSSDeclaration::GetLength(PRUint32* aLength)
{
  nsCSSDeclaration *decl;
  nsresult result = GetCSSDeclaration(&decl, PR_FALSE);
 
  if (decl) {
    *aLength = decl->Count();
  } else {
    *aLength = 0;
  }

  return result;
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
  nsCSSDeclaration *decl;
  nsresult result = GetCSSDeclaration(&decl, PR_FALSE);

  aReturn.SetLength(0);
  if (decl) {
    result = decl->GetNthProperty(aIndex, aReturn);
  }

  return result;
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
  nsCSSDeclaration *decl;
  nsresult result = GetCSSDeclaration(&decl, PR_FALSE);

  aReturn.Truncate();
  if (decl && decl->GetValueIsImportant(aPropertyName)) {
    aReturn.AssignLiteral("important");    
  }

  return result;
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
    return ParsePropertyValue(propID, aValue);
  }

  
  
  
  
  return ParseDeclaration(aPropertyName + NS_LITERAL_STRING(":") +
                          aValue + NS_LITERAL_STRING("!") + aPriority,
                          PR_TRUE, PR_FALSE);
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
  
  rv = RemoveProperty(propID);
  return rv;
}

nsresult
nsDOMCSSDeclaration::ParsePropertyValue(const nsCSSProperty aPropID,
                                        const nsAString& aPropValue)
{
  nsCSSDeclaration* decl;
  nsresult result = GetCSSDeclaration(&decl, PR_TRUE);
  if (!decl) {
    return result;
  }
  
  nsCOMPtr<nsICSSLoader> cssLoader;
  nsCOMPtr<nsICSSParser> cssParser;
  nsCOMPtr<nsIURI> baseURI, sheetURI;
  nsCOMPtr<nsIPrincipal> sheetPrincipal;
  
  result = GetCSSParsingEnvironment(getter_AddRefs(sheetURI),
                                    getter_AddRefs(baseURI),
                                    getter_AddRefs(sheetPrincipal),
                                    getter_AddRefs(cssLoader),
                                    getter_AddRefs(cssParser));
  if (NS_FAILED(result)) {
    return result;
  }

  PRBool changed;
  result = cssParser->ParseProperty(aPropID, aPropValue, sheetURI, baseURI,
                                    sheetPrincipal, decl, &changed);
  if (NS_SUCCEEDED(result) && changed) {
    result = DeclarationChanged();
  }

  if (cssLoader) {
    cssLoader->RecycleParser(cssParser);
  }

  return result;
}

nsresult
nsDOMCSSDeclaration::ParseDeclaration(const nsAString& aDecl,
                                      PRBool aParseOnlyOneDecl,
                                      PRBool aClearOldDecl)
{
  nsCSSDeclaration* decl;
  nsresult result = GetCSSDeclaration(&decl, PR_TRUE);
  if (!decl) {
    return result;
  }

  nsCOMPtr<nsICSSLoader> cssLoader;
  nsCOMPtr<nsICSSParser> cssParser;
  nsCOMPtr<nsIURI> baseURI, sheetURI;
  nsCOMPtr<nsIPrincipal> sheetPrincipal;

  result = GetCSSParsingEnvironment(getter_AddRefs(sheetURI),
                                    getter_AddRefs(baseURI),
                                    getter_AddRefs(sheetPrincipal),
                                    getter_AddRefs(cssLoader),
                                    getter_AddRefs(cssParser));

  if (NS_FAILED(result)) {
    return result;
  }

  PRBool changed;
  result = cssParser->ParseAndAppendDeclaration(aDecl, sheetURI, baseURI,
                                                sheetPrincipal, decl,
                                                aParseOnlyOneDecl,
                                                &changed,
                                                aClearOldDecl);
  
  if (NS_SUCCEEDED(result) && changed) {
    result = DeclarationChanged();
  }

  if (cssLoader) {
    cssLoader->RecycleParser(cssParser);
  }

  return result;
}

nsresult
nsDOMCSSDeclaration::RemoveProperty(const nsCSSProperty aPropID)
{
  nsCSSDeclaration* decl;
  nsresult rv = GetCSSDeclaration(&decl, PR_FALSE);
  if (!decl) {
    return rv;
  }

  rv = decl->RemoveProperty(aPropID);

  if (NS_SUCCEEDED(rv)) {
    rv = DeclarationChanged();
  } else {
    
    
    
    rv = NS_OK;
  }

  return rv;
}



CSS2PropertiesTearoff::CSS2PropertiesTearoff(nsICSSDeclaration *aOuter)
  : mOuter(aOuter)
{
  NS_ASSERTION(mOuter, "must have outer");
}

CSS2PropertiesTearoff::~CSS2PropertiesTearoff()
{
}

NS_IMETHODIMP_(nsrefcnt)
CSS2PropertiesTearoff::AddRef(void)
{
  return mOuter->AddRef();
}

NS_IMETHODIMP_(nsrefcnt)
CSS2PropertiesTearoff::Release(void)
{
  return mOuter->Release();
}

NS_IMETHODIMP
CSS2PropertiesTearoff::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  return mOuter->QueryInterface(aIID, aInstancePtr);
}




#define CSS_PROP(name_, id_, method_, flags_, datastruct_, member_, type_, kwtable_) \
  NS_IMETHODIMP                                                              \
  CSS2PropertiesTearoff::Get##method_(nsAString& aValue)                     \
  {                                                                          \
    return mOuter->GetPropertyValue(eCSSProperty_##id_, aValue);             \
  }                                                                          \
                                                                             \
  NS_IMETHODIMP                                                              \
  CSS2PropertiesTearoff::Set##method_(const nsAString& aValue)               \
  {                                                                          \
    return mOuter->SetPropertyValue(eCSSProperty_##id_, aValue);             \
  }

#define CSS_PROP_NOTIMPLEMENTED(name_, id_, method_, flags_)                         \
  NS_IMETHODIMP                                                              \
  CSS2PropertiesTearoff::Get##method_(nsAString& aValue)                     \
  {                                                                          \
    aValue.Truncate();                                                       \
    return NS_OK;                                                            \
  }                                                                          \
                                                                             \
  NS_IMETHODIMP                                                              \
  CSS2PropertiesTearoff::Set##method_(const nsAString& aValue)               \
  {                                                                          \
    return NS_OK;                                                            \
  }

#define CSS_PROP_LIST_EXCLUDE_INTERNAL
#define CSS_PROP_SHORTHAND(name_, id_, method_, flags_) \
  CSS_PROP(name_, id_, method_, flags_, X, X, X, X)
#include "nsCSSPropList.h"


CSS_PROP(X, opacity, MozOpacity, 0, X, X, X, X)
CSS_PROP(X, outline, MozOutline, 0, X, X, X, X)
CSS_PROP(X, outline_color, MozOutlineColor, 0, X, X, X, X)
CSS_PROP(X, outline_style, MozOutlineStyle, 0, X, X, X, X)
CSS_PROP(X, outline_width, MozOutlineWidth, 0, X, X, X, X)
CSS_PROP(X, outline_offset, MozOutlineOffset, 0, X, X, X, X)

#undef CSS_PROP_SHORTHAND
#undef CSS_PROP_NOTIMPLEMENTED
#undef CSS_PROP_LIST_EXCLUDE_INTERNAL
#undef CSS_PROP
