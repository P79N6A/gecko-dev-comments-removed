




#include "mozilla/dom/HTMLContentElement.h"
#include "mozilla/dom/HTMLContentElementBinding.h"
#include "mozilla/dom/NodeListBinding.h"
#include "mozilla/dom/ShadowRoot.h"
#include "mozilla/css/StyleRule.h"
#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsIAtom.h"
#include "nsCSSRuleProcessor.h"
#include "nsRuleData.h"
#include "nsRuleProcessorData.h"
#include "nsRuleWalker.h"
#include "nsCSSParser.h"

NS_IMPL_NS_NEW_HTML_ELEMENT(Content)

using namespace mozilla::dom;

HTMLContentElement::HTMLContentElement(already_AddRefed<nsINodeInfo>& aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo), mValidSelector(true), mIsInsertionPoint(false)
{
  SetIsDOMBinding();
}

HTMLContentElement::~HTMLContentElement()
{
}

NS_IMPL_CYCLE_COLLECTION_INHERITED(HTMLContentElement,
                                   nsGenericHTMLElement,
                                   mMatchedNodes)

NS_IMPL_ADDREF_INHERITED(HTMLContentElement, Element)
NS_IMPL_RELEASE_INHERITED(HTMLContentElement, Element)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(HTMLContentElement)
NS_INTERFACE_MAP_END_INHERITING(nsGenericHTMLElement)

NS_IMPL_ELEMENT_CLONE(HTMLContentElement)

JSObject*
HTMLContentElement::WrapNode(JSContext *aCx)
{
  return HTMLContentElementBinding::Wrap(aCx, this);
}

nsresult
HTMLContentElement::BindToTree(nsIDocument* aDocument,
                               nsIContent* aParent,
                               nsIContent* aBindingParent,
                               bool aCompileEventHandlers)
{
  nsresult rv = nsGenericHTMLElement::BindToTree(aDocument, aParent,
                                                 aBindingParent,
                                                 aCompileEventHandlers);
  NS_ENSURE_SUCCESS(rv, rv);

  ShadowRoot* containingShadow = GetContainingShadow();
  if (containingShadow) {
    nsINode* parentNode = nsINode::GetParentNode();
    while (parentNode && parentNode != containingShadow) {
      if (parentNode->IsElement() &&
          parentNode->AsElement()->IsHTML(nsGkAtoms::content)) {
        
        return NS_OK;
      }
      parentNode = parentNode->GetParentNode();
    }

    
    
    mIsInsertionPoint = true;
    containingShadow->AddInsertionPoint(this);
    containingShadow->SetInsertionPointChanged();
  }

  return NS_OK;
}

void
HTMLContentElement::UnbindFromTree(bool aDeep, bool aNullParent)
{
  if (mIsInsertionPoint) {
    ShadowRoot* containingShadow = GetContainingShadow();
    
    
    if (containingShadow) {
      containingShadow->RemoveInsertionPoint(this);

      
      
      
      ClearMatchedNodes();
      containingShadow->SetInsertionPointChanged();
    }

    mIsInsertionPoint = false;
  }

  nsGenericHTMLElement::UnbindFromTree(aDeep, aNullParent);
}

void
HTMLContentElement::ClearMatchedNodes()
{
  for (uint32_t i = 0; i < mMatchedNodes.Length(); i++) {
    mMatchedNodes[i]->SetXBLInsertionParent(nullptr);
  }
  mMatchedNodes.Clear();
}

static bool
IsValidContentSelectors(nsCSSSelector* aSelector)
{
  nsCSSSelector* currentSelector = aSelector;
  while (currentSelector) {
    
    if (currentSelector->IsPseudoElement() ||
        currentSelector->mPseudoClassList ||
        currentSelector->mNegations ||
        currentSelector->mOperator) {
      return false;
    }

    currentSelector = currentSelector->mNext;
  }

  return true;
}

nsresult
HTMLContentElement::SetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                            nsIAtom* aPrefix, const nsAString& aValue,
                            bool aNotify)
{
  nsresult rv = nsGenericHTMLElement::SetAttr(aNameSpaceID, aName, aPrefix,
                                              aValue, aNotify);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aNameSpaceID == kNameSpaceID_None && aName == nsGkAtoms::select) {
    
    
    nsIDocument* doc = OwnerDoc();
    nsCSSParser parser(doc->CSSLoader());

    mValidSelector = true;
    mSelectorList = nullptr;

    nsresult rv = parser.ParseSelectorString(aValue,
                                             doc->GetDocumentURI(),
                                             
                                             0, 
                                             getter_Transfers(mSelectorList));

    
    
    if (NS_SUCCEEDED(rv)) {
      
      nsCSSSelectorList* selectors = mSelectorList;
      while (selectors) {
        if (!IsValidContentSelectors(selectors->mSelectors)) {
          
          mValidSelector = false;
          mSelectorList = nullptr;
          break;
        }
        selectors = selectors->mNext;
      }
    }

    ShadowRoot* containingShadow = GetContainingShadow();
    if (containingShadow) {
      containingShadow->DistributeAllNodes();
    }
  }

  return NS_OK;
}

nsresult
HTMLContentElement::UnsetAttr(int32_t aNameSpaceID, nsIAtom* aAttribute,
                              bool aNotify)
{
  nsresult rv = nsGenericHTMLElement::UnsetAttr(aNameSpaceID,
                                                aAttribute, aNotify);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aNameSpaceID == kNameSpaceID_None && aAttribute == nsGkAtoms::select) {
    
    
    mValidSelector = true;
    mSelectorList = nullptr;

    ShadowRoot* containingShadow = GetContainingShadow();
    if (containingShadow) {
      containingShadow->DistributeAllNodes();
    }
  }

  return NS_OK;
}

bool
HTMLContentElement::Match(nsIContent* aContent)
{
  if (!mValidSelector) {
    return false;
  }

  if (mSelectorList) {
    nsIDocument* doc = OwnerDoc();
    ShadowRoot* containingShadow = GetContainingShadow();
    nsIContent* host = containingShadow->GetHost();

    TreeMatchContext matchingContext(false, nsRuleWalker::eRelevantLinkUnvisited,
                                     doc, TreeMatchContext::eNeverMatchVisited);
    doc->FlushPendingLinkUpdates();
    matchingContext.SetHasSpecifiedScope();
    matchingContext.AddScopeElement(host->AsElement());

    if (!aContent->IsElement()) {
      return false;
    }

    return nsCSSRuleProcessor::SelectorListMatches(aContent->AsElement(),
                                                   matchingContext,
                                                   mSelectorList);
  }

  return true;
}

already_AddRefed<DistributedContentList>
HTMLContentElement::GetDistributedNodes()
{
  nsRefPtr<DistributedContentList> list = new DistributedContentList(this);
  return list.forget();
}

NS_IMPL_CYCLE_COLLECTION(DistributedContentList, mParent, mDistributedNodes)

NS_INTERFACE_TABLE_HEAD(DistributedContentList)
  NS_INTERFACE_TABLE1(DistributedContentList, nsINodeList)
  NS_INTERFACE_TABLE_TO_MAP_SEGUE_CYCLE_COLLECTION(DistributedContentList)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(DistributedContentList)
NS_IMPL_CYCLE_COLLECTING_RELEASE(DistributedContentList)

DistributedContentList::DistributedContentList(HTMLContentElement* aHostElement)
  : mParent(aHostElement)
{
  MOZ_COUNT_CTOR(DistributedContentList);
  SetIsDOMBinding();

  if (aHostElement->IsInsertionPoint()) {
    if (aHostElement->MatchedNodes().IsEmpty()) {
      
      nsINode* contentNode = aHostElement;
      for (nsIContent* content = contentNode->GetFirstChild();
           content;
           content = content->GetNextSibling()) {
        mDistributedNodes.AppendElement(content);
      }
    } else {
      mDistributedNodes.AppendElements(aHostElement->MatchedNodes());
    }
  }
}

DistributedContentList::~DistributedContentList()
{
  MOZ_COUNT_DTOR(DistributedContentList);
}

nsIContent*
DistributedContentList::Item(uint32_t aIndex)
{
  return mDistributedNodes.SafeElementAt(aIndex);
}

NS_IMETHODIMP
DistributedContentList::Item(uint32_t aIndex, nsIDOMNode** aReturn)
{
  nsIContent* item = Item(aIndex);
  if (!item) {
    return NS_ERROR_FAILURE;
  }

  return CallQueryInterface(item, aReturn);
}

uint32_t
DistributedContentList::Length() const
{
  return mDistributedNodes.Length();
}

NS_IMETHODIMP
DistributedContentList::GetLength(uint32_t* aLength)
{
  *aLength = Length();
  return NS_OK;
}

int32_t
DistributedContentList::IndexOf(nsIContent* aContent)
{
  return mDistributedNodes.IndexOf(aContent);
}

JSObject*
DistributedContentList::WrapObject(JSContext* aCx)
{
  return NodeListBinding::Wrap(aCx, this);
}

