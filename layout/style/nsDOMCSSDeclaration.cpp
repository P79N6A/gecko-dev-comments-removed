






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
#include "mozAutoDocUpdate.h"
#include "nsStyleUtil.h"

using namespace mozilla;

nsDOMCSSDeclaration::~nsDOMCSSDeclaration()
{
}

NS_INTERFACE_TABLE_HEAD(nsDOMCSSDeclaration)
  NS_INTERFACE_TABLE2(nsDOMCSSDeclaration,
                      nsICSSDeclaration,
                      nsIDOMCSSStyleDeclaration)
  NS_INTERFACE_TABLE_TO_MAP_SEGUE
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

  nsresult result;
  if (env.mDocument && !nsStyleUtil::CSPAllowsInlineStyle(
                                               env.mDocument->NodePrincipal(),
                                               env.mDocument->GetDocumentURI(),
                                                         0, aCssText, &result))
    return result;

  
  
  
  
  
  mozAutoDocConditionalContentUpdateBatch autoUpdate(DocToUpdate(), true);

  nsAutoPtr<css::Declaration> decl(new css::Declaration());
  decl->InitializeEmpty();
  nsCSSParser cssParser(env.mCSSLoader);
  bool changed;
  result = cssParser.ParseDeclarations(aCssText, env.mSheetURI,
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

already_AddRefed<dom::CSSValue>
nsDOMCSSDeclaration::GetPropertyCSSValue(const nsAString& aPropertyName, ErrorResult& aRv)
{
  

  return nullptr;
}

void
nsDOMCSSDeclaration::IndexedGetter(uint32_t aIndex, bool& aFound, nsAString& aPropName)
{
  css::Declaration* decl = GetCSSDeclaration(false);
  aFound = decl && decl->GetNthProperty(aIndex, aPropName);
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

  aCSSParseEnv.mDocument = sheet->GetOwningDocument();
  aCSSParseEnv.mSheetURI = sheet->GetSheetURI();
  aCSSParseEnv.mBaseURI = sheet->GetBaseURI();
  aCSSParseEnv.mPrincipal = cssSheet->Principal();
  aCSSParseEnv.mCSSLoader = aCSSParseEnv.mDocument ?
    aCSSParseEnv.mDocument->CSSLoader() : nullptr;
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

  nsresult result;
  if (env.mDocument && !nsStyleUtil::CSPAllowsInlineStyle(
                                                env.mDocument->NodePrincipal(),
                                               env.mDocument->GetDocumentURI(),
                                                       0, aPropValue, &result))
    return result;

  
  
  
  
  
  mozAutoDocConditionalContentUpdateBatch autoUpdate(DocToUpdate(), true);
  css::Declaration* decl = olddecl->EnsureMutable();

  nsCSSParser cssParser(env.mCSSLoader);
  bool changed;
  result = cssParser.ParseProperty(aPropID, aPropValue, env.mSheetURI,
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

  CSSParsingEnvironment env;
  GetCSSParsingEnvironment(env);

  nsresult result;
  if (env.mDocument && !nsStyleUtil::CSPAllowsInlineStyle(
                                                env.mDocument->NodePrincipal(),
                                               env.mDocument->GetDocumentURI(),
                 0, NS_ConvertUTF8toUTF16(nsCSSProps::GetStringValue(aPropID)),
                                                         &result))
    return result;

  
  
  
  
  
  mozAutoDocConditionalContentUpdateBatch autoUpdate(DocToUpdate(), true);

  decl = decl->EnsureMutable();
  decl->RemoveProperty(aPropID);
  return SetCSSDeclaration(decl);
}
