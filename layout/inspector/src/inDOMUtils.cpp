



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
#include "mozilla/css/StyleRule.h"
#include "nsICSSStyleRuleDOMWrapper.h"
#include "nsIDOMWindow.h"
#include "nsXBLBinding.h"
#include "nsXBLPrototypeBinding.h"
#include "nsIMutableArray.h"
#include "nsBindingManager.h"
#include "nsComputedDOMStyle.h"
#include "nsEventStateManager.h"
#include "nsIAtom.h"
#include "nsRange.h"
#include "mozilla/dom/Element.h"
#include "nsCSSStyleSheet.h"




inDOMUtils::inDOMUtils()
{
}

inDOMUtils::~inDOMUtils()
{
}

NS_IMPL_ISUPPORTS1(inDOMUtils, inIDOMUtils)




NS_IMETHODIMP
inDOMUtils::IsIgnorableWhitespace(nsIDOMCharacterData *aDataNode,
                                  bool *aReturn)
{
  NS_PRECONDITION(aReturn, "Must have an out parameter");

  NS_ENSURE_ARG_POINTER(aDataNode);

  *aReturn = false;

  nsCOMPtr<nsIContent> content = do_QueryInterface(aDataNode);
  NS_ASSERTION(content, "Does not implement nsIContent!");

  if (!content->TextIsOnlyWhitespace()) {
    return NS_OK;
  }

  
  

  nsCOMPtr<nsIDOMWindow> win = inLayoutUtils::GetWindowFor(aDataNode);
  if (!win) {
    
    NS_ERROR("No window!");
    return NS_OK;
  }

  nsIFrame* frame = content->GetPrimaryFrame();
  if (frame) {
    const nsStyleText* text = frame->GetStyleText();
    *aReturn = !text->WhiteSpaceIsSignificant();
  }
  else {
    
    *aReturn = true;
  }

  return NS_OK;
}

NS_IMETHODIMP
inDOMUtils::GetParentForNode(nsIDOMNode* aNode,
                             bool aShowingAnonymousContent,
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
inDOMUtils::GetChildrenForNode(nsIDOMNode* aNode,
                               bool aShowingAnonymousContent,
                               nsIDOMNodeList** aChildren)
{
  NS_ENSURE_ARG_POINTER(aNode);
  NS_PRECONDITION(aChildren, "Must have an out parameter");

  nsCOMPtr<nsIDOMNodeList> kids;

  if (aShowingAnonymousContent) {
    nsCOMPtr<nsIContent> content = do_QueryInterface(aNode);
    if (content) {
      nsRefPtr<nsBindingManager> bindingManager =
        inLayoutUtils::GetBindingManagerFor(aNode);
      if (bindingManager) {
        bindingManager->GetAnonymousNodesFor(content, getter_AddRefs(kids));
        if (!kids) {
          bindingManager->GetContentListFor(content, getter_AddRefs(kids));
        }
      }
    }
  }

  if (!kids) {
    aNode->GetChildNodes(getter_AddRefs(kids));
  }

  kids.forget(aChildren);
  return NS_OK;
}

NS_IMETHODIMP
inDOMUtils::GetCSSStyleRules(nsIDOMElement *aElement,
                             const nsAString& aPseudo,
                             nsISupportsArray **_retval)
{
  NS_ENSURE_ARG_POINTER(aElement);

  *_retval = nsnull;

  nsCOMPtr<nsIAtom> pseudoElt;
  if (!aPseudo.IsEmpty()) {
    pseudoElt = do_GetAtom(aPseudo);
  }

  nsRuleNode* ruleNode = nsnull;
  nsCOMPtr<nsIContent> content = do_QueryInterface(aElement);
  NS_ENSURE_STATE(content);
  nsRefPtr<nsStyleContext> styleContext;
  GetRuleNodeForContent(content, pseudoElt, getter_AddRefs(styleContext), &ruleNode);
  if (!ruleNode) {
    
    
    return NS_OK;
  }

  nsCOMPtr<nsISupportsArray> rules;
  NS_NewISupportsArray(getter_AddRefs(rules));
  if (!rules) return NS_ERROR_OUT_OF_MEMORY;

  nsRefPtr<mozilla::css::StyleRule> cssRule;
  for ( ; !ruleNode->IsRoot(); ruleNode = ruleNode->GetParent()) {
    cssRule = do_QueryObject(ruleNode->GetRule());
    if (cssRule) {
      nsCOMPtr<nsIDOMCSSRule> domRule = cssRule->GetDOMRule();
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
  nsRefPtr<mozilla::css::StyleRule> cssrule;
  nsresult rv = rule->GetCSSStyleRule(getter_AddRefs(cssrule));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(cssrule != nsnull, NS_ERROR_FAILURE);
  *_retval = cssrule->GetLineNumber();
  return NS_OK;
}

NS_IMETHODIMP
inDOMUtils::IsInheritedProperty(const nsAString &aPropertyName, bool *_retval)
{
  nsCSSProperty prop = nsCSSProps::LookupProperty(aPropertyName);
  if (prop == eCSSProperty_UNKNOWN) {
    *_retval = false;
    return NS_OK;
  }

  if (nsCSSProps::IsShorthand(prop)) {
    prop = nsCSSProps::SubpropertyEntryFor(prop)[0];
  }

  nsStyleStructID sid = nsCSSProps::kSIDTable[prop];
  *_retval = !nsCachedStyleData::IsReset(sid);
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

  nsIDocument *ownerDoc = content->OwnerDoc();
  nsXBLBinding *binding = ownerDoc->BindingManager()->GetBinding(content);

  while (binding) {
    urls->AppendElement(binding->PrototypeBinding()->BindingURI(), false);
    binding = binding->GetBaseBinding();
  }

  NS_ADDREF(*_retval = urls);
  return NS_OK;
}

NS_IMETHODIMP
inDOMUtils::SetContentState(nsIDOMElement *aElement, nsEventStates::InternalType aState)
{
  NS_ENSURE_ARG_POINTER(aElement);

  nsRefPtr<nsEventStateManager> esm = inLayoutUtils::GetEventStateManagerFor(aElement);
  if (esm) {
    nsCOMPtr<nsIContent> content;
    content = do_QueryInterface(aElement);

    return esm->SetContentState(content, nsEventStates(aState));
  }

  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
inDOMUtils::GetContentState(nsIDOMElement *aElement, nsEventStates::InternalType* aState)
{
  *aState = 0;
  nsCOMPtr<nsIContent> content = do_QueryInterface(aElement);
  NS_ENSURE_ARG_POINTER(content);

  
  
  *aState = content->AsElement()->State().GetInternalValue();
  return NS_OK;
}

 nsresult
inDOMUtils::GetRuleNodeForContent(nsIContent* aContent,
                                  nsIAtom* aPseudo,
                                  nsStyleContext** aStyleContext,
                                  nsRuleNode** aRuleNode)
{
  *aRuleNode = nsnull;
  *aStyleContext = nsnull;

  if (!aContent->IsElement()) {
    return NS_ERROR_UNEXPECTED;
  }

  nsIDocument* doc = aContent->GetDocument();
  NS_ENSURE_TRUE(doc, NS_ERROR_UNEXPECTED);

  nsIPresShell *presShell = doc->GetShell();
  NS_ENSURE_TRUE(presShell, NS_ERROR_UNEXPECTED);

  nsPresContext *presContext = presShell->GetPresContext();
  NS_ENSURE_TRUE(presContext, NS_ERROR_UNEXPECTED);

  bool safe = presContext->EnsureSafeToHandOutCSSRules();
  NS_ENSURE_TRUE(safe, NS_ERROR_OUT_OF_MEMORY);

  nsRefPtr<nsStyleContext> sContext =
    nsComputedDOMStyle::GetStyleContextForElement(aContent->AsElement(), aPseudo, presShell);
  if (sContext) {
    *aRuleNode = sContext->GetRuleNode();
    sContext.forget(aStyleContext);
  }
  return NS_OK;
}

NS_IMETHODIMP
inDOMUtils::GetUsedFontFaces(nsIDOMRange* aRange,
                             nsIDOMFontFaceList** aFontFaceList)
{
  return static_cast<nsRange*>(aRange)->GetUsedFontFaces(aFontFaceList);
}

static nsEventStates
GetStatesForPseudoClass(const nsAString& aStatePseudo)
{
  
  
  static const nsEventStates sPseudoClassStates[] = {
#define CSS_PSEUDO_CLASS(_name, _value) \
    nsEventStates(),
#define CSS_STATE_PSEUDO_CLASS(_name, _value, _states) \
    _states,
#include "nsCSSPseudoClassList.h"
#undef CSS_STATE_PSEUDO_CLASS
#undef CSS_PSEUDO_CLASS

    
    
    nsEventStates(),
    nsEventStates()
  };
  MOZ_STATIC_ASSERT(NS_ARRAY_LENGTH(sPseudoClassStates) ==
                    nsCSSPseudoClasses::ePseudoClass_NotPseudoClass + 1,
                    "Length of PseudoClassStates array is incorrect");

  nsCOMPtr<nsIAtom> atom = do_GetAtom(aStatePseudo);

  
  
  if (nsCSSPseudoClasses::GetPseudoType(atom) ==
      nsCSSPseudoClasses::ePseudoClass_mozAnyLink) {
    return nsEventStates();
  }
  
  
  return sPseudoClassStates[nsCSSPseudoClasses::GetPseudoType(atom)];
}

NS_IMETHODIMP
inDOMUtils::AddPseudoClassLock(nsIDOMElement *aElement,
                               const nsAString &aPseudoClass)
{
  nsEventStates state = GetStatesForPseudoClass(aPseudoClass);
  if (state.IsEmpty()) {
    return NS_OK;
  }

  nsCOMPtr<mozilla::dom::Element> element = do_QueryInterface(aElement);
  NS_ENSURE_ARG_POINTER(element);

  element->LockStyleStates(state);

  return NS_OK;
}

NS_IMETHODIMP
inDOMUtils::RemovePseudoClassLock(nsIDOMElement *aElement,
                                  const nsAString &aPseudoClass)
{
  nsEventStates state = GetStatesForPseudoClass(aPseudoClass);
  if (state.IsEmpty()) {
    return NS_OK;
  }

  nsCOMPtr<mozilla::dom::Element> element = do_QueryInterface(aElement);
  NS_ENSURE_ARG_POINTER(element);

  element->UnlockStyleStates(state);

  return NS_OK;
}

NS_IMETHODIMP
inDOMUtils::HasPseudoClassLock(nsIDOMElement *aElement,
                               const nsAString &aPseudoClass,
                               bool *_retval)
{
  nsEventStates state = GetStatesForPseudoClass(aPseudoClass);
  if (state.IsEmpty()) {
    *_retval = false;
    return NS_OK;
  }

  nsCOMPtr<mozilla::dom::Element> element = do_QueryInterface(aElement);
  NS_ENSURE_ARG_POINTER(element);

  nsEventStates locks = element->LockedStyleStates();

  *_retval = locks.HasAllStates(state);
  return NS_OK;
}

NS_IMETHODIMP
inDOMUtils::ClearPseudoClassLocks(nsIDOMElement *aElement)
{
  nsCOMPtr<mozilla::dom::Element> element = do_QueryInterface(aElement);
  NS_ENSURE_ARG_POINTER(element);

  element->ClearStyleStateLocks();

  return NS_OK;
}

NS_IMETHODIMP
inDOMUtils::ParseStyleSheet(nsIDOMCSSStyleSheet *aSheet,
                            const nsAString& aInput)
{
  nsRefPtr<nsCSSStyleSheet> sheet = do_QueryObject(aSheet);
  NS_ENSURE_ARG_POINTER(sheet);

  return sheet->ParseSheet(aInput);
}
