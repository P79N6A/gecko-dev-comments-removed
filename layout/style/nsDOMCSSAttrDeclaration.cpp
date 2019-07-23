






































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

nsDOMCSSAttributeDeclaration::nsDOMCSSAttributeDeclaration(nsIContent *aContent
#ifdef MOZ_SMIL
                                                           , PRBool aIsSMILOverride
#endif 
                                                           )
  : mContent(aContent)
#ifdef MOZ_SMIL
  , mIsSMILOverride(aIsSMILOverride)
#endif 
{
  MOZ_COUNT_CTOR(nsDOMCSSAttributeDeclaration);

  NS_ASSERTION(aContent && aContent->IsNodeOfType(nsINode::eELEMENT),
               "Inline style for non-element content?");
}

nsDOMCSSAttributeDeclaration::~nsDOMCSSAttributeDeclaration()
{
  MOZ_COUNT_DTOR(nsDOMCSSAttributeDeclaration);
}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsDOMCSSAttributeDeclaration)

NS_IMPL_CYCLE_COLLECTION_ROOT_BEGIN(nsDOMCSSAttributeDeclaration)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_ROOT_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsDOMCSSAttributeDeclaration)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mContent)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(nsDOMCSSAttributeDeclaration)
  NS_IMPL_CYCLE_COLLECTION_TRACE_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsDOMCSSAttributeDeclaration)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mContent)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_INTERFACE_MAP_BEGIN(nsDOMCSSAttributeDeclaration)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRIES_CYCLE_COLLECTION(nsDOMCSSAttributeDeclaration)
NS_IMPL_QUERY_TAIL_INHERITING(nsDOMCSSDeclaration)

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsDOMCSSAttributeDeclaration)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsDOMCSSAttributeDeclaration)

nsresult
nsDOMCSSAttributeDeclaration::DeclarationChanged()
{
  NS_ASSERTION(mContent, "Must have content node to set the decl!");
  nsICSSStyleRule* oldRule =
#ifdef MOZ_SMIL
    mIsSMILOverride ? mContent->GetSMILOverrideStyleRule() :
#endif 
    mContent->GetInlineStyleRule();
  NS_ASSERTION(oldRule, "content must have rule");

  nsCOMPtr<nsICSSStyleRule> newRule = oldRule->DeclarationChanged(PR_FALSE);
  if (!newRule) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
    
  return
#ifdef MOZ_SMIL
    mIsSMILOverride ? mContent->SetSMILOverrideStyleRule(newRule, PR_TRUE) :
#endif 
    mContent->SetInlineStyleRule(newRule, PR_TRUE);
}

nsresult
nsDOMCSSAttributeDeclaration::GetCSSDeclaration(nsCSSDeclaration **aDecl,
                                                PRBool aAllocate)
{
  nsresult result = NS_OK;

  *aDecl = nsnull;
  if (mContent) {
    nsICSSStyleRule* cssRule =
#ifdef MOZ_SMIL
      mIsSMILOverride ? mContent->GetSMILOverrideStyleRule() :
#endif 
      mContent->GetInlineStyleRule();
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
        
      result =
#ifdef MOZ_SMIL
        mIsSMILOverride ?
          mContent->SetSMILOverrideStyleRule(newRule, PR_FALSE) :
#endif 
          mContent->SetInlineStyleRule(newRule, PR_FALSE);
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

