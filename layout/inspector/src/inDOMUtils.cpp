





































#include "inDOMUtils.h"
#include "inLayoutUtils.h"

#include "nsIServiceManager.h"
#include "nsString.h"
#include "nsIDOMElement.h"
#include "nsIDocument.h"
#include "nsIPresShell.h"
#include "nsIDOMDocument.h"
#include "nsIDOMCharacterData.h"
#include "nsRuleNode.h"
#include "nsIStyleRule.h"
#include "nsICSSStyleRule.h"
#include "nsICSSStyleRuleDOMWrapper.h"
#include "nsIDOMWindowInternal.h"
#include "nsXBLBinding.h"
#include "nsXBLPrototypeBinding.h"
#include "nsIDOMElement.h"
#include "nsIMutableArray.h"
#include "nsBindingManager.h"
#include "nsComputedDOMStyle.h"



inDOMUtils::inDOMUtils()
{
}

inDOMUtils::~inDOMUtils()
{
}

NS_IMPL_ISUPPORTS1(inDOMUtils, inIDOMUtils)




NS_IMETHODIMP
inDOMUtils::IsIgnorableWhitespace(nsIDOMCharacterData *aDataNode,
                                  PRBool *aReturn)
{
  NS_PRECONDITION(aReturn, "Must have an out parameter");

  NS_ENSURE_ARG_POINTER(aDataNode);

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
    *aReturn = !text->WhiteSpaceIsSignificant();
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
  NS_ENSURE_ARG_POINTER(aNode);

  
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
  NS_ENSURE_ARG_POINTER(aElement);

  *_retval = nsnull;

  nsRuleNode* ruleNode = nsnull;
  nsCOMPtr<nsIContent> content = do_QueryInterface(aElement);
  GetRuleNodeForContent(content, &ruleNode);
  if (!ruleNode) {
    
    
    return NS_OK;
  }

  nsCOMPtr<nsISupportsArray> rules;
  NS_NewISupportsArray(getter_AddRefs(rules));
  if (!rules) return NS_ERROR_OUT_OF_MEMORY;

  nsCOMPtr<nsICSSStyleRule> cssRule;
  nsCOMPtr<nsIDOMCSSRule> domRule;
  for ( ; !ruleNode->IsRoot(); ruleNode = ruleNode->GetParent()) {
    cssRule = do_QueryInterface(ruleNode->GetRule());
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

  NS_ENSURE_ARG_POINTER(aRule);

  nsCOMPtr<nsICSSStyleRuleDOMWrapper> rule = do_QueryInterface(aRule);
  nsCOMPtr<nsICSSStyleRule> cssrule;
  nsresult rv = rule->GetCSSStyleRule(getter_AddRefs(cssrule));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(cssrule != nsnull, NS_ERROR_FAILURE);
  *_retval = cssrule->GetLineNumber();
  return NS_OK;
}

NS_IMETHODIMP 
inDOMUtils::GetBindingURLs(nsIDOMElement *aElement, nsIArray **_retval)
{
  NS_ENSURE_ARG_POINTER(aElement);

  *_retval = nsnull;

  nsCOMPtr<nsIMutableArray> urls = do_CreateInstance(NS_ARRAY_CONTRACTID);
  if (!urls)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIContent> content = do_QueryInterface(aElement);
  NS_ASSERTION(content, "elements must implement nsIContent");

  nsIDocument *ownerDoc = content->GetOwnerDoc();
  if (ownerDoc) {
    nsXBLBinding *binding = ownerDoc->BindingManager()->GetBinding(content);

    while (binding) {
      urls->AppendElement(binding->PrototypeBinding()->BindingURI(), PR_FALSE);
      binding = binding->GetBaseBinding();
    }
  }

  NS_ADDREF(*_retval = urls);
  return NS_OK;
}

NS_IMETHODIMP
inDOMUtils::SetContentState(nsIDOMElement *aElement, PRInt32 aState)
{
  NS_ENSURE_ARG_POINTER(aElement);
  
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

  NS_ENSURE_ARG_POINTER(aElement);

  nsCOMPtr<nsIEventStateManager> esm = inLayoutUtils::GetEventStateManagerFor(aElement);
  if (esm) {
    nsCOMPtr<nsIContent> content;
    content = do_QueryInterface(aElement);
  
    return esm->GetContentState(content, *aState);
  }

  return NS_ERROR_FAILURE;
}

 nsresult
inDOMUtils::GetRuleNodeForContent(nsIContent* aContent, nsRuleNode** aRuleNode)
{
  *aRuleNode = nsnull;

  nsIDocument* doc = aContent->GetDocument();
  NS_ENSURE_TRUE(doc, NS_ERROR_UNEXPECTED);

  nsIPresShell *presShell = doc->GetPrimaryShell();
  NS_ENSURE_TRUE(presShell, NS_ERROR_UNEXPECTED);

  nsRefPtr<nsStyleContext> sContext =
    nsComputedDOMStyle::GetStyleContextForContent(aContent, nsnull, presShell);
  *aRuleNode = sContext->GetRuleNode();
  return NS_OK;
}
