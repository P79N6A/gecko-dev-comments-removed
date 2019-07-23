






































#include "nsDOMCSSAttrDeclaration.h"
#include "nsCSSDeclaration.h"
#include "nsIDocument.h"
#include "nsIDOMMutationEvent.h"
#include "nsICSSStyleRule.h"
#include "nsICSSLoader.h"
#include "nsICSSParser.h"
#include "nsIURI.h"
#include "nsINameSpaceManager.h"
#include "nsStyleConsts.h"
#include "nsContentUtils.h"
#include "nsIContent.h"
#include "nsIPrincipal.h"

nsDOMCSSAttributeDeclaration::nsDOMCSSAttributeDeclaration(nsIContent *aContent)
{
  MOZ_COUNT_CTOR(nsDOMCSSAttributeDeclaration);

  
  
  NS_ASSERTION(aContent && aContent->IsNodeOfType(nsINode::eELEMENT),
               "Inline style for non-element content?");
  mContent = aContent;
}

nsDOMCSSAttributeDeclaration::~nsDOMCSSAttributeDeclaration()
{
  MOZ_COUNT_DTOR(nsDOMCSSAttributeDeclaration);
}

NS_IMPL_ADDREF(nsDOMCSSAttributeDeclaration)
NS_IMPL_RELEASE(nsDOMCSSAttributeDeclaration)

void
nsDOMCSSAttributeDeclaration::DropReference()
{
  mContent = nsnull;
}

nsresult
nsDOMCSSAttributeDeclaration::DeclarationChanged()
{
  NS_ASSERTION(mContent, "Must have content node to set the decl!");
  nsICSSStyleRule* oldRule = mContent->GetInlineStyleRule();
  NS_ASSERTION(oldRule, "content must have rule");

  nsCOMPtr<nsICSSStyleRule> newRule = oldRule->DeclarationChanged(PR_FALSE);
  if (!newRule) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
    
  return mContent->SetInlineStyleRule(newRule, PR_TRUE);
}

nsresult
nsDOMCSSAttributeDeclaration::GetCSSDeclaration(nsCSSDeclaration **aDecl,
                                                PRBool aAllocate)
{
  nsresult result = NS_OK;

  *aDecl = nsnull;
  if (mContent) {
    nsICSSStyleRule* cssRule = mContent->GetInlineStyleRule();
    if (cssRule) {
      *aDecl = cssRule->GetDeclaration();
    }
    else if (aAllocate) {
      nsCSSDeclaration *decl = new nsCSSDeclaration();
      if (!decl)
        return NS_ERROR_OUT_OF_MEMORY;
      if (!decl->InitializeEmpty()) {
        decl->RuleAbort();
        return NS_ERROR_OUT_OF_MEMORY;
      }
      
      nsCOMPtr<nsICSSStyleRule> newRule;
      result = NS_NewCSSStyleRule(getter_AddRefs(newRule), nsnull, decl);
      if (NS_FAILED(result)) {
        decl->RuleAbort();
        return result;
      }
        
      result = mContent->SetInlineStyleRule(newRule, PR_FALSE);
      if (NS_SUCCEEDED(result)) {
        *aDecl = decl;
      }
    }
  }

  return result;
}






nsresult
nsDOMCSSAttributeDeclaration::GetCSSParsingEnvironment(nsIURI** aSheetURI,
                                                       nsIURI** aBaseURI,
                                                       nsIPrincipal** aSheetPrincipal,
                                                       nsICSSLoader** aCSSLoader,
                                                       nsICSSParser** aCSSParser)
{
  NS_ASSERTION(mContent, "Something is severely broken -- there should be an nsIContent here!");
  
  *aSheetURI = nsnull;
  *aBaseURI = nsnull;
  *aSheetPrincipal = nsnull;
  *aCSSLoader = nsnull;
  *aCSSParser = nsnull;

  nsIDocument* doc = mContent->GetOwnerDoc();
  if (!doc) {
    
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsCOMPtr<nsIURI> baseURI = mContent->GetBaseURI();
  nsCOMPtr<nsIURI> sheetURI = doc->GetDocumentURI();

  NS_ADDREF(*aCSSLoader = doc->CSSLoader());
  
  nsresult rv = NS_OK;

  
  
  rv = (*aCSSLoader)->GetParserFor(nsnull, aCSSParser);
  if (NS_FAILED(rv)) {
    return rv;
  }
  
  baseURI.swap(*aBaseURI);
  sheetURI.swap(*aSheetURI);
  NS_ADDREF(*aSheetPrincipal = mContent->NodePrincipal());

  return NS_OK;
}

NS_IMETHODIMP
nsDOMCSSAttributeDeclaration::GetParentRule(nsIDOMCSSRule **aParent)
{
  NS_ENSURE_ARG_POINTER(aParent);

  *aParent = nsnull;
  return NS_OK;
}

