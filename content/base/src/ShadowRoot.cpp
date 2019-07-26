




#include "mozilla/Preferences.h"
#include "mozilla/dom/ShadowRoot.h"
#include "mozilla/dom/ShadowRootBinding.h"
#include "mozilla/dom/DocumentFragment.h"
#include "ChildIterator.h"
#include "nsContentUtils.h"
#include "nsDOMClassInfoID.h"
#include "nsIDOMHTMLElement.h"
#include "mozilla/dom/Element.h"
#include "mozilla/dom/HTMLContentElement.h"
#include "nsXBLPrototypeBinding.h"

using namespace mozilla;
using namespace mozilla::dom;

static PLDHashOperator
IdentifierMapEntryTraverse(nsIdentifierMapEntry *aEntry, void *aArg)
{
  nsCycleCollectionTraversalCallback *cb =
    static_cast<nsCycleCollectionTraversalCallback*>(aArg);
  aEntry->Traverse(cb);
  return PL_DHASH_NEXT;
}

NS_IMPL_CYCLE_COLLECTION_CLASS(ShadowRoot)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(ShadowRoot,
                                                  DocumentFragment)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mHost)
  tmp->mIdentifierMap.EnumerateEntries(IdentifierMapEntryTraverse, &cb);
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(ShadowRoot,
                                                DocumentFragment)
  if (tmp->mHost) {
    tmp->mHost->RemoveMutationObserver(tmp);
  }
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mHost)
  tmp->mIdentifierMap.Clear();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

DOMCI_DATA(ShadowRoot, ShadowRoot)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(ShadowRoot)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIContent)
  NS_INTERFACE_MAP_ENTRY(nsIMutationObserver)
NS_INTERFACE_MAP_END_INHERITING(DocumentFragment)

NS_IMPL_ADDREF_INHERITED(ShadowRoot, DocumentFragment)
NS_IMPL_RELEASE_INHERITED(ShadowRoot, DocumentFragment)

ShadowRoot::ShadowRoot(nsIContent* aContent,
                       already_AddRefed<nsINodeInfo> aNodeInfo)
  : DocumentFragment(aNodeInfo), mHost(aContent),
    mInsertionPointChanged(false)
{
  SetHost(aContent);
  SetFlags(NODE_IS_IN_SHADOW_TREE);
  
  SetInDocument();
  DOMSlots()->mBindingParent = aContent;
  DOMSlots()->mContainingShadow = this;

  
  
  
  mHost->AddMutationObserver(this);
}

ShadowRoot::~ShadowRoot()
{
  if (mHost) {
    
    mHost->RemoveMutationObserver(this);
  }

  ClearInDocument();
  SetHost(nullptr);
}

JSObject*
ShadowRoot::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return mozilla::dom::ShadowRootBinding::Wrap(aCx, aScope, this);
}

ShadowRoot*
ShadowRoot::FromNode(nsINode* aNode)
{
  if (aNode->HasFlag(NODE_IS_IN_SHADOW_TREE) && !aNode->GetParentNode()) {
    MOZ_ASSERT(aNode->NodeType() == nsIDOMNode::DOCUMENT_FRAGMENT_NODE,
               "ShadowRoot is a document fragment.");
    return static_cast<ShadowRoot*>(aNode);
  }

  return nullptr;
}

Element*
ShadowRoot::GetElementById(const nsAString& aElementId)
{
  nsIdentifierMapEntry *entry = mIdentifierMap.GetEntry(aElementId);
  return entry ? entry->GetIdElement() : nullptr;
}

already_AddRefed<nsContentList>
ShadowRoot::GetElementsByTagName(const nsAString& aTagName)
{
  return NS_GetContentList(this, kNameSpaceID_Unknown, aTagName);
}

already_AddRefed<nsContentList>
ShadowRoot::GetElementsByTagNameNS(const nsAString& aNamespaceURI,
                                   const nsAString& aLocalName)
{
  int32_t nameSpaceId = kNameSpaceID_Wildcard;

  if (!aNamespaceURI.EqualsLiteral("*")) {
    nsresult rv =
      nsContentUtils::NameSpaceManager()->RegisterNameSpace(aNamespaceURI,
                                                            nameSpaceId);
    NS_ENSURE_SUCCESS(rv, nullptr);
  }

  NS_ASSERTION(nameSpaceId != kNameSpaceID_Unknown, "Unexpected namespace ID!");

  return NS_GetContentList(this, nameSpaceId, aLocalName);
}

void
ShadowRoot::AddToIdTable(Element* aElement, nsIAtom* aId)
{
  nsIdentifierMapEntry *entry =
    mIdentifierMap.PutEntry(nsDependentAtomString(aId));
  if (entry) {
    entry->AddIdElement(aElement);
  }
}

void
ShadowRoot::RemoveFromIdTable(Element* aElement, nsIAtom* aId)
{
  nsIdentifierMapEntry *entry =
    mIdentifierMap.GetEntry(nsDependentAtomString(aId));
  if (entry) {
    entry->RemoveIdElement(aElement);
    if (entry->IsEmpty()) {
      mIdentifierMap.RawRemoveEntry(entry);
    }
  }
}

already_AddRefed<nsContentList>
ShadowRoot::GetElementsByClassName(const nsAString& aClasses)
{
  return nsContentUtils::GetElementsByClassName(this, aClasses);
}

bool
ShadowRoot::PrefEnabled()
{
  return Preferences::GetBool("dom.webcomponents.enabled", false);
}

class TreeOrderComparator {
public:
  bool Equals(nsINode* aElem1, nsINode* aElem2) const {
    return aElem1 == aElem2;
  }
  bool LessThan(nsINode* aElem1, nsINode* aElem2) const {
    return nsContentUtils::PositionIsBefore(aElem1, aElem2);
  }
};

void
ShadowRoot::AddInsertionPoint(HTMLContentElement* aInsertionPoint)
{
  TreeOrderComparator comparator;
  mInsertionPoints.InsertElementSorted(aInsertionPoint, comparator);
}

void
ShadowRoot::RemoveInsertionPoint(HTMLContentElement* aInsertionPoint)
{
  mInsertionPoints.RemoveElement(aInsertionPoint);
}

void
ShadowRoot::DistributeSingleNode(nsIContent* aContent)
{
  
  HTMLContentElement* insertionPoint = nullptr;
  for (uint32_t i = 0; i < mInsertionPoints.Length(); i++) {
    if (mInsertionPoints[i]->Match(aContent)) {
      
      if (mInsertionPoints[i]->MatchedNodes().IsEmpty() &&
          static_cast<nsINode*>(mInsertionPoints[i])->GetFirstChild()) {
        
        
        
        DistributeAllNodes();
        return;
      }
      insertionPoint = mInsertionPoints[i];
      break;
    }
  }

  
  if (insertionPoint) {
    nsCOMArray<nsIContent>& matchedNodes = insertionPoint->MatchedNodes();
    
    
    bool isIndexFound = false;
    ExplicitChildIterator childIterator(GetHost());
    for (uint32_t i = 0; i < matchedNodes.Length(); i++) {
      
      
      if (childIterator.Seek(aContent, matchedNodes[i])) {
        
        matchedNodes.InsertElementAt(i, aContent);
        isIndexFound = true;
        break;
      }
    }

    if (!isIndexFound) {
      
      
      MOZ_ASSERT(childIterator.Seek(aContent),
                 "Trying to match a node that is not a candidate to be matched");
      matchedNodes.AppendElement(aContent);
    }

    
    
    
    ShadowRoot* parentShadow = insertionPoint->GetParent()->GetShadowRoot();
    if (parentShadow) {
      parentShadow->DistributeSingleNode(aContent);
    }
  }
}

void
ShadowRoot::RemoveDistributedNode(nsIContent* aContent)
{
  
  for (uint32_t i = 0; i < mInsertionPoints.Length(); i++) {
    if (mInsertionPoints[i]->MatchedNodes().Contains(aContent)) {
      
      
      if (mInsertionPoints[i]->MatchedNodes().Length() == 1 &&
          static_cast<nsINode*>(mInsertionPoints[i])->GetFirstChild()) {
        
        
        DistributeAllNodes();
        return;
      }

      mInsertionPoints[i]->MatchedNodes().RemoveElement(aContent);

      
      
      
      ShadowRoot* parentShadow = mInsertionPoints[i]->GetParent()->GetShadowRoot();
      if (parentShadow) {
        parentShadow->RemoveDistributedNode(aContent);
      }

      break;
    }
  }
}

void
ShadowRoot::DistributeAllNodes()
{
  
  nsTArray<nsIContent*> nodePool;
  ExplicitChildIterator childIterator(GetHost());
  for (nsIContent* content = childIterator.GetNextChild();
       content;
       content = childIterator.GetNextChild()) {
    nodePool.AppendElement(content);
  }

  for (uint32_t i = 0; i < mInsertionPoints.Length(); i++) {
    mInsertionPoints[i]->ClearMatchedNodes();
    
    for (uint32_t j = 0; j < nodePool.Length(); j++) {
      if (mInsertionPoints[i]->Match(nodePool[j])) {
        mInsertionPoints[i]->MatchedNodes().AppendElement(nodePool[j]);
        nodePool[j]->SetXBLInsertionParent(mInsertionPoints[i]);
        nodePool.RemoveElementAt(j--);
      }
    }
  }

  
  
  for (uint32_t i = 0; i < mInsertionPoints.Length(); i++) {
    nsIContent* insertionParent = mInsertionPoints[i]->GetParent();
    MOZ_ASSERT(insertionParent, "The only way for an insertion point to be in the"
                                "mInsertionPoints array is to be a descendant of a"
                                "ShadowRoot, in which case, it should have a parent");

    
    
    
    ShadowRoot* parentShadow = insertionParent->GetShadowRoot();
    if (parentShadow) {
      parentShadow->DistributeAllNodes();
    }
  }
}

void
ShadowRoot::GetInnerHTML(nsAString& aInnerHTML)
{
  GetMarkup(false, aInnerHTML);
}

void
ShadowRoot::SetInnerHTML(const nsAString& aInnerHTML, ErrorResult& aError)
{
  SetInnerHTMLInternal(aInnerHTML, aError);
}







static bool
IsPooledNode(nsIContent* aContent, nsIContent* aContainer, nsIContent* aHost)
{
  if (nsContentUtils::IsContentInsertionPoint(aContent)) {
    
    return false;
  }

  if (aContainer->IsHTML(nsGkAtoms::content)) {
    
    HTMLContentElement* content = static_cast<HTMLContentElement*>(aContainer);
    return content->IsInsertionPoint() && content->MatchedNodes().IsEmpty() &&
           aContainer->GetParentNode() == aHost;
  }

  if (aContainer == aHost) {
    
    return true;
  }

  return false;
}

void
ShadowRoot::AttributeChanged(nsIDocument* aDocument,
                             Element* aElement,
                             int32_t aNameSpaceID,
                             nsIAtom* aAttribute,
                             int32_t aModType)
{
  
  
  if (!IsPooledNode(aElement, aElement->GetParent(), mHost)) {
    return;
  }

  
  RemoveDistributedNode(aElement);
  DistributeSingleNode(aElement);
}

void
ShadowRoot::ContentAppended(nsIDocument* aDocument,
                            nsIContent* aContainer,
                            nsIContent* aFirstNewContent,
                            int32_t aNewIndexInContainer)
{
  if (mInsertionPointChanged) {
    DistributeAllNodes();
    mInsertionPointChanged = false;
    return;
  }

  
  
  nsIContent* currentChild = aFirstNewContent;
  while (currentChild) {
    if (IsPooledNode(currentChild, aContainer, mHost)) {
      DistributeSingleNode(currentChild);
    }
    currentChild = currentChild->GetNextSibling();
  }
}

void
ShadowRoot::ContentInserted(nsIDocument* aDocument,
                            nsIContent* aContainer,
                            nsIContent* aChild,
                            int32_t aIndexInContainer)
{
  if (mInsertionPointChanged) {
    DistributeAllNodes();
    mInsertionPointChanged = false;
    return;
  }

  
  
  if (IsPooledNode(aChild, aContainer, mHost)) {
    DistributeSingleNode(aChild);
  }
}

void
ShadowRoot::ContentRemoved(nsIDocument* aDocument,
                           nsIContent* aContainer,
                           nsIContent* aChild,
                           int32_t aIndexInContainer,
                           nsIContent* aPreviousSibling)
{
  if (mInsertionPointChanged) {
    DistributeAllNodes();
    mInsertionPointChanged = false;
    return;
  }

  
  
  if (IsPooledNode(aChild, aContainer, mHost)) {
    RemoveDistributedNode(aChild);
  }
}

