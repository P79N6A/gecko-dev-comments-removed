




#include "mozilla/dom/ShadowRoot.h"

#include "ChildIterator.h"
#include "nsContentUtils.h"
#include "mozilla/dom/HTMLShadowElement.h"
#include "mozilla/dom/HTMLShadowElementBinding.h"

NS_IMPL_NS_NEW_HTML_ELEMENT(Shadow)

using namespace mozilla::dom;

HTMLShadowElement::HTMLShadowElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo), mIsInsertionPoint(false)
{
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
HTMLShadowElement::WrapNode(JSContext *aCx)
{
  return HTMLShadowElementBinding::Wrap(aCx, this);
}

void
HTMLShadowElement::SetProjectedShadow(ShadowRoot* aProjectedShadow)
{
  if (mProjectedShadow) {
    mProjectedShadow->RemoveMutationObserver(this);

    
    
    ExplicitChildIterator childIterator(mProjectedShadow);
    for (nsIContent* content = childIterator.GetNextChild();
         content;
         content = childIterator.GetNextChild()) {
      ShadowRoot::RemoveDestInsertionPoint(this, content->DestInsertionPoints());
    }
  }

  mProjectedShadow = aProjectedShadow;
  if (mProjectedShadow) {
    
    
    ExplicitChildIterator childIterator(mProjectedShadow);
    for (nsIContent* content = childIterator.GetNextChild();
         content;
         content = childIterator.GetNextChild()) {
      content->DestInsertionPoints().AppendElement(this);
    }

    
    
    
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
  nsRefPtr<ShadowRoot> oldContainingShadow = GetContainingShadow();

  nsresult rv = nsGenericHTMLElement::BindToTree(aDocument, aParent,
                                                 aBindingParent,
                                                 aCompileEventHandlers);
  NS_ENSURE_SUCCESS(rv, rv);

  ShadowRoot* containingShadow = GetContainingShadow();
  if (containingShadow && !oldContainingShadow) {
    
    
    
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

  if (mIsInsertionPoint && containingShadow) {
    
    ShadowRoot* projectedShadow = containingShadow->GetOlderShadowRoot();
    if (projectedShadow) {
      projectedShadow->SetIsComposedDocParticipant(IsInComposedDoc());

      for (nsIContent* child = projectedShadow->GetFirstChild(); child;
           child = child->GetNextSibling()) {
        rv = child->BindToTree(nullptr, projectedShadow,
                               projectedShadow->GetBindingParent(),
                               aCompileEventHandlers);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
  }

  return NS_OK;
}

void
HTMLShadowElement::UnbindFromTree(bool aDeep, bool aNullParent)
{
  nsRefPtr<ShadowRoot> oldContainingShadow = GetContainingShadow();

  if (mIsInsertionPoint && oldContainingShadow) {
    
    
    ShadowRoot* projectedShadow = oldContainingShadow->GetOlderShadowRoot();
    if (projectedShadow) {
      for (nsIContent* child = projectedShadow->GetFirstChild(); child;
           child = child->GetNextSibling()) {
        child->UnbindFromTree(true, false);
      }

      projectedShadow->SetIsComposedDocParticipant(false);
    }
  }

  nsGenericHTMLElement::UnbindFromTree(aDeep, aNullParent);

  if (oldContainingShadow && !GetContainingShadow() && mIsInsertionPoint) {
    nsTArray<HTMLShadowElement*>& shadowDescendants =
      oldContainingShadow->ShadowDescendants();
    shadowDescendants.RemoveElement(this);
    oldContainingShadow->SetShadowElement(nullptr);

    
    if (shadowDescendants.Length() > 0 &&
        !IsInFallbackContent(shadowDescendants[0])) {
      oldContainingShadow->SetShadowElement(shadowDescendants[0]);
    }

    oldContainingShadow->SetInsertionPointChanged();

    mIsInsertionPoint = false;
  }
}

void
HTMLShadowElement::DistributeSingleNode(nsIContent* aContent)
{
  if (aContent->DestInsertionPoints().Contains(this)) {
    
    
    return;
  }

  aContent->DestInsertionPoints().AppendElement(this);

  
  
  
  
  ShadowRoot* parentShadowRoot = GetParent()->GetShadowRoot();
  if (parentShadowRoot) {
    parentShadowRoot->DistributeSingleNode(aContent);
    return;
  }

  
  
  ShadowRoot* containingShadow = GetContainingShadow();
  ShadowRoot* youngerShadow = containingShadow->GetYoungerShadowRoot();
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
  ShadowRoot::RemoveDestInsertionPoint(this, aContent->DestInsertionPoints());

  
  
  
  
  ShadowRoot* parentShadowRoot = GetParent()->GetShadowRoot();
  if (parentShadowRoot) {
    parentShadowRoot->RemoveDistributedNode(aContent);
    return;
  }

  
  
  ShadowRoot* containingShadow = GetContainingShadow();
  ShadowRoot* youngerShadow = containingShadow->GetYoungerShadowRoot();
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
  
  
  
  ShadowRoot* containingShadow = GetContainingShadow();
  ShadowRoot* olderShadow = containingShadow->GetOlderShadowRoot();
  if (olderShadow) {
    ExplicitChildIterator childIterator(olderShadow);
    for (nsIContent* content = childIterator.GetNextChild();
         content;
         content = childIterator.GetNextChild()) {
      ShadowRoot::RemoveDestInsertionPoint(this, content->DestInsertionPoints());
      content->DestInsertionPoints().AppendElement(this);
    }
  }

  
  
  
  
  ShadowRoot* parentShadowRoot = GetParent()->GetShadowRoot();
  if (parentShadowRoot) {
    parentShadowRoot->DistributeAllNodes();
    return;
  }

  
  
  ShadowRoot* youngerShadow = containingShadow->GetYoungerShadowRoot();
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

