





































#include "inDOMUtils.h"
#include "inLayoutUtils.h"

#include "nsIServiceManager.h"
#include "nsString.h"
#include "nsIDOMElement.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsIDOMCharacterData.h"
#include "nsRuleNode.h"
#include "nsIStyleRule.h"
#include "nsICSSStyleRule.h"
#include "nsICSSStyleRuleDOMWrapper.h"
#include "nsIDOMWindowInternal.h"

static NS_DEFINE_CID(kInspectorCSSUtilsCID, NS_INSPECTORCSSUTILS_CID);



inDOMUtils::inDOMUtils()
{
  mCSSUtils = do_GetService(kInspectorCSSUtilsCID);
}

inDOMUtils::~inDOMUtils()
{
}

NS_IMPL_ISUPPORTS1(inDOMUtils, inIDOMUtils)




NS_IMETHODIMP
inDOMUtils::IsIgnorableWhitespace(nsIDOMCharacterData *aDataNode,
                                  PRBool *aReturn)
{
  NS_PRECONDITION(aDataNode, "Must have a character data node");
  NS_PRECONDITION(aReturn, "Must have an out parameter");

  *aReturn = PR_FALSE;

  nsCOMPtr<nsIContent> content = do_QueryInterface(aDataNode);
  NS_ASSERTION(content, "Does not implement nsIContent!");

  if (!content->TextIsOnlyWhitespace()) {
    return NS_OK;
  }

  
  

  nsCOMPtr<nsIDOMWindowInternal> win = inLayoutUtils::GetWindowFor(aDataNode);
  if (!win) {
    
    NS_ERROR("No window!");
    return NS_OK;
  }

  nsCOMPtr<nsIPresShell> presShell = inLayoutUtils::GetPresShellFor(win);
  if (!presShell) {
    
    return NS_OK;
  }

  nsIFrame* frame = presShell->GetPrimaryFrameFor(content);
  if (frame) {
    const nsStyleText* text = frame->GetStyleText();
    *aReturn = text->mWhiteSpace != NS_STYLE_WHITESPACE_PRE &&
               text->mWhiteSpace != NS_STYLE_WHITESPACE_MOZ_PRE_WRAP;
  }
  else {
    
    *aReturn = PR_TRUE;
  }

  return NS_OK;
}

NS_IMETHODIMP
inDOMUtils::GetParentForNode(nsIDOMNode* aNode,
                             PRBool aShowingAnonymousContent,
                             nsIDOMNode** aParent)
{
    
  nsCOMPtr<nsIDOMDocument> doc(do_QueryInterface(aNode));
  nsCOMPtr<nsIDOMNode> parent;

  if (doc) {
    parent = inLayoutUtils::GetContainerFor(doc);
  } else if (aShowingAnonymousContent) {
    nsCOMPtr<nsIContent> content = do_QueryInterface(aNode);
    if (content) {
      nsIContent* bparent = nsnull;
      nsRefPtr<nsBindingManager> bindingManager = inLayoutUtils::GetBindingManagerFor(aNode);
      if (bindingManager) {
        bparent = bindingManager->GetInsertionParent(content);
      }
    
      parent = do_QueryInterface(bparent);
    }
  }
  
  if (!parent) {
    
    aNode->GetParentNode(getter_AddRefs(parent));
  }

  NS_IF_ADDREF(*aParent = parent);
  return NS_OK;
}

NS_IMETHODIMP
inDOMUtils::GetCSSStyleRules(nsIDOMElement *aElement,
                             nsISupportsArray **_retval)
{
  if (!aElement) return NS_ERROR_NULL_POINTER;

  *_retval = nsnull;

  nsRuleNode* ruleNode = nsnull;
  nsCOMPtr<nsIContent> content = do_QueryInterface(aElement);
  mCSSUtils->GetRuleNodeForContent(content, &ruleNode);
  if (!ruleNode) {
    
    
    return NS_OK;
  }

  nsCOMPtr<nsISupportsArray> rules;
  NS_NewISupportsArray(getter_AddRefs(rules));
  if (!rules) return NS_ERROR_OUT_OF_MEMORY;

  nsCOMPtr<nsIStyleRule> srule;
  nsCOMPtr<nsICSSStyleRule> cssRule;
  nsCOMPtr<nsIDOMCSSRule> domRule;
  for (PRBool isRoot;
       mCSSUtils->IsRuleNodeRoot(ruleNode, &isRoot), !isRoot;
       mCSSUtils->GetRuleNodeParent(ruleNode, &ruleNode))
  {
    mCSSUtils->GetRuleNodeRule(ruleNode, getter_AddRefs(srule));
    cssRule = do_QueryInterface(srule);
    if (cssRule) {
      cssRule->GetDOMRule(getter_AddRefs(domRule));
      if (domRule)
        rules->InsertElementAt(domRule, 0);
    }
  }

  *_retval = rules;
  NS_ADDREF(*_retval);

  return NS_OK;
}

NS_IMETHODIMP
inDOMUtils::GetRuleLine(nsIDOMCSSStyleRule *aRule, PRUint32 *_retval)
{
  *_retval = 0;
  if (!aRule)
    return NS_OK;
  nsCOMPtr<nsICSSStyleRuleDOMWrapper> rule = do_QueryInterface(aRule);
  nsCOMPtr<nsICSSStyleRule> cssrule;
  rule->GetCSSStyleRule(getter_AddRefs(cssrule));
  if (cssrule)
    *_retval = cssrule->GetLineNumber();
  return NS_OK;
}

NS_IMETHODIMP 
inDOMUtils::GetBindingURLs(nsIDOMElement *aElement, nsIArray **_retval)
{
  return mCSSUtils->GetBindingURLs(aElement, _retval);
}

NS_IMETHODIMP
inDOMUtils::SetContentState(nsIDOMElement *aElement, PRInt32 aState)
{
  if (!aElement)
    return NS_ERROR_NULL_POINTER;
  
  nsCOMPtr<nsIEventStateManager> esm = inLayoutUtils::GetEventStateManagerFor(aElement);
  if (esm) {
    nsCOMPtr<nsIContent> content;
    content = do_QueryInterface(aElement);
  
    return esm->SetContentState(content, aState);
  }
  
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
inDOMUtils::GetContentState(nsIDOMElement *aElement, PRInt32* aState)
{
  *aState = 0;

  if (!aElement)
    return NS_ERROR_NULL_POINTER;

  nsCOMPtr<nsIEventStateManager> esm = inLayoutUtils::GetEventStateManagerFor(aElement);
  if (esm) {
    nsCOMPtr<nsIContent> content;
    content = do_QueryInterface(aElement);
  
    return esm->GetContentState(content, *aState);
  }

  return NS_ERROR_FAILURE;
}

