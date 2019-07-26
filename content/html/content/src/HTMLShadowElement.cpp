




#include "mozilla/dom/ShadowRoot.h"

#include "nsContentUtils.h"
#include "mozilla/dom/HTMLShadowElement.h"
#include "mozilla/dom/HTMLShadowElementBinding.h"

NS_IMPL_NS_NEW_HTML_ELEMENT(Shadow)

using namespace mozilla::dom;

HTMLShadowElement::HTMLShadowElement(already_AddRefed<nsINodeInfo>& aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo), mIsInsertionPoint(false)
{
  SetIsDOMBinding();
}

HTMLShadowElement::~HTMLShadowElement()
{
  if (mProjectedShadow) {
    mProjectedShadow->RemoveMutationObserver(this);
  }
}

NS_IMPL_CYCLE_COLLECTION_CLASS(HTMLShadowElement)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(HTMLShadowElement,
                                                  nsGenericHTMLElement)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mProjectedShadow)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(HTMLShadowElement,
                                                nsGenericHTMLElement)
  if (tmp->mProjectedShadow) {
    tmp->mProjectedShadow->RemoveMutationObserver(tmp);
    tmp->mProjectedShadow = nullptr;
  }
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_ADDREF_INHERITED(HTMLShadowElement, Element)
NS_IMPL_RELEASE_INHERITED(HTMLShadowElement, Element)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(HTMLShadowElement)
NS_INTERFACE_MAP_END_INHERITING(nsGenericHTMLElement)

NS_IMPL_ELEMENT_CLONE(HTMLShadowElement)

JSObject*
HTMLShadowElement::WrapNode(JSContext *aCx, JS::Handle<JSObject*> aScope)
{
  return HTMLShadowElementBinding::Wrap(aCx, aScope, this);
}

void
HTMLShadowElement::SetProjectedShadow(ShadowRoot* aProjectedShadow)
{
  if (mProjectedShadow) {
    mProjectedShadow->RemoveMutationObserver(this);
  }

  mProjectedShadow = aProjectedShadow;
  if (mProjectedShadow) {
    
    
    
    mProjectedShadow->AddMutationObserver(this);
  }
}

static bool
IsInFallbackContent(nsIContent* aContent)
{
  nsINode* parentNode = aContent->GetParentNode();
  while (parentNode) {
    if (parentNode->IsElement() &&
        parentNode->AsElement()->IsHTML(nsGkAtoms::content)) {
      return true;
    }
    parentNode = parentNode->GetParentNode();
  }

  return false;
}

nsresult
HTMLShadowElement::BindToTree(nsIDocument* aDocument,
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
    
    
    
    TreeOrderComparator comparator;
    containingShadow->ShadowDescendants().InsertElementSorted(this, comparator);

    if (containingShadow->ShadowDescendants()[0] != this) {
      
      return NS_OK;
    }

    if (IsInFallbackContent(this)) {
      
      
      containingShadow->SetShadowElement(nullptr);
    } else {
      mIsInsertionPoint = true;
      containingShadow->SetShadowElement(this);
    }

    containingShadow->SetInsertionPointChanged();
  }

  return NS_OK;
}

void
HTMLShadowElement::UnbindFromTree(bool aDeep, bool aNullParent)
{
  if (mIsInsertionPoint) {
    ShadowRoot* containingShadow = GetContainingShadow();
    
    
    if (containingShadow) {
      nsTArray<HTMLShadowElement*>& shadowDescendants =
        containingShadow->ShadowDescendants();
      shadowDescendants.RemoveElement(this);
      containingShadow->SetShadowElement(nullptr);

      
      if (shadowDescendants.Length() > 0 &&
          !IsInFallbackContent(shadowDescendants[0])) {
        containingShadow->SetShadowElement(shadowDescendants[0]);
      }

      containingShadow->SetInsertionPointChanged();
    }

    mIsInsertionPoint = false;
  }

  nsGenericHTMLElement::UnbindFromTree(aDeep, aNullParent);
}

void
HTMLShadowElement::DistributeSingleNode(nsIContent* aContent)
{
  
  
  
  
  ShadowRoot* parentShadowRoot = GetParent()->GetShadowRoot();
  if (parentShadowRoot) {
    parentShadowRoot->DistributeSingleNode(aContent);
    return;
  }

  
  
  ShadowRoot* containingShadow = GetContainingShadow();
  ShadowRoot* youngerShadow = containingShadow->GetYoungerShadow();
  if (youngerShadow && GetParent() == containingShadow) {
    HTMLShadowElement* youngerShadowElement = youngerShadow->GetShadowElement();
    if (youngerShadowElement) {
      youngerShadowElement->DistributeSingleNode(aContent);
    }
  }
}

void
HTMLShadowElement::RemoveDistributedNode(nsIContent* aContent)
{
  
  
  
  
  ShadowRoot* parentShadowRoot = GetParent()->GetShadowRoot();
  if (parentShadowRoot) {
    parentShadowRoot->RemoveDistributedNode(aContent);
    return;
  }

  
  
  ShadowRoot* containingShadow = GetContainingShadow();
  ShadowRoot* youngerShadow = containingShadow->GetYoungerShadow();
  if (youngerShadow && GetParent() == containingShadow) {
    HTMLShadowElement* youngerShadowElement = youngerShadow->GetShadowElement();
    if (youngerShadowElement) {
      youngerShadowElement->RemoveDistributedNode(aContent);
    }
  }
}

void
HTMLShadowElement::DistributeAllNodes()
{
  
  
  
  
  ShadowRoot* parentShadowRoot = GetParent()->GetShadowRoot();
  if (parentShadowRoot) {
    parentShadowRoot->DistributeAllNodes();
    return;
  }

  
  
  ShadowRoot* containingShadow = GetContainingShadow();
  ShadowRoot* youngerShadow = containingShadow->GetYoungerShadow();
  if (youngerShadow && GetParent() == containingShadow) {
    HTMLShadowElement* youngerShadowElement = youngerShadow->GetShadowElement();
    if (youngerShadowElement) {
      youngerShadowElement->DistributeAllNodes();
    }
  }
}

void
HTMLShadowElement::ContentAppended(nsIDocument* aDocument,
                                   nsIContent* aContainer,
                                   nsIContent* aFirstNewContent,
                                   int32_t aNewIndexInContainer)
{
  
  
  
  nsIContent* currentChild = aFirstNewContent;
  while (currentChild) {
    if (ShadowRoot::IsPooledNode(currentChild, aContainer, mProjectedShadow)) {
      DistributeSingleNode(currentChild);
    }
    currentChild = currentChild->GetNextSibling();
  }
}

void
HTMLShadowElement::ContentInserted(nsIDocument* aDocument,
                                   nsIContent* aContainer,
                                   nsIContent* aChild,
                                   int32_t aIndexInContainer)
{
  
  
  
  if (!ShadowRoot::IsPooledNode(aChild, aContainer, mProjectedShadow)) {
    return;
  }

  DistributeSingleNode(aChild);
}

void
HTMLShadowElement::ContentRemoved(nsIDocument* aDocument,
                                  nsIContent* aContainer,
                                  nsIContent* aChild,
                                  int32_t aIndexInContainer,
                                  nsIContent* aPreviousSibling)
{
  
  
  
  if (!ShadowRoot::IsPooledNode(aChild, aContainer, mProjectedShadow)) {
    return;
  }

  RemoveDistributedNode(aChild);
}

