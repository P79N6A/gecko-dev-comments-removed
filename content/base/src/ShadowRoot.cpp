




#include "mozilla/Preferences.h"
#include "mozilla/dom/ShadowRoot.h"
#include "mozilla/dom/ShadowRootBinding.h"
#include "mozilla/dom/DocumentFragment.h"
#include "ChildIterator.h"
#include "nsContentUtils.h"
#include "nsDOMClassInfoID.h"
#include "nsIDOMHTMLElement.h"
#include "nsIStyleSheetLinkingElement.h"
#include "mozilla/dom/Element.h"
#include "mozilla/dom/HTMLContentElement.h"
#include "mozilla/dom/HTMLShadowElement.h"
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
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mPoolHost)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mStyleSheetList)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mOlderShadow)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mYoungerShadow)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mAssociatedBinding)
  tmp->mIdentifierMap.EnumerateEntries(IdentifierMapEntryTraverse, &cb);
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(ShadowRoot,
                                                DocumentFragment)
  if (tmp->mPoolHost) {
    tmp->mPoolHost->RemoveMutationObserver(tmp);
  }
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mPoolHost)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mStyleSheetList)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mOlderShadow)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mYoungerShadow)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mAssociatedBinding)
  tmp->mIdentifierMap.Clear();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(ShadowRoot)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIContent)
  NS_INTERFACE_MAP_ENTRY(nsIMutationObserver)
NS_INTERFACE_MAP_END_INHERITING(DocumentFragment)

NS_IMPL_ADDREF_INHERITED(ShadowRoot, DocumentFragment)
NS_IMPL_RELEASE_INHERITED(ShadowRoot, DocumentFragment)

ShadowRoot::ShadowRoot(nsIContent* aContent,
                       already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo,
                       nsXBLPrototypeBinding* aProtoBinding)
  : DocumentFragment(aNodeInfo), mPoolHost(aContent),
    mProtoBinding(aProtoBinding), mShadowElement(nullptr),
    mInsertionPointChanged(false)
{
  SetHost(aContent);

  
  
  
  ClearSubtreeRootPointer();

  SetFlags(NODE_IS_IN_SHADOW_TREE);

  DOMSlots()->mBindingParent = aContent;
  DOMSlots()->mContainingShadow = this;

  
  
  
  mPoolHost->AddMutationObserver(this);
}

ShadowRoot::~ShadowRoot()
{
  if (mPoolHost) {
    
    
    mPoolHost->RemoveMutationObserver(this);
  }

  UnsetFlags(NODE_IS_IN_SHADOW_TREE);

  
  SetSubtreeRootPointer(this);

  SetHost(nullptr);
}

JSObject*
ShadowRoot::WrapObject(JSContext* aCx)
{
  return mozilla::dom::ShadowRootBinding::Wrap(aCx, this);
}

ShadowRoot*
ShadowRoot::FromNode(nsINode* aNode)
{
  if (aNode->IsInShadowTree() && !aNode->GetParentNode()) {
    MOZ_ASSERT(aNode->NodeType() == nsIDOMNode::DOCUMENT_FRAGMENT_NODE,
               "ShadowRoot is a document fragment.");
    return static_cast<ShadowRoot*>(aNode);
  }

  return nullptr;
}

void
ShadowRoot::StyleSheetChanged()
{
  mProtoBinding->FlushSkinSheets();

  nsIPresShell* shell = OwnerDoc()->GetShell();
  if (shell) {
    OwnerDoc()->BeginUpdate(UPDATE_STYLE);
    shell->RecordShadowStyleChange(this);
    OwnerDoc()->EndUpdate(UPDATE_STYLE);
  }
}

void
ShadowRoot::InsertSheet(CSSStyleSheet* aSheet,
                        nsIContent* aLinkingContent)
{
  nsCOMPtr<nsIStyleSheetLinkingElement>
    linkingElement = do_QueryInterface(aLinkingContent);
  MOZ_ASSERT(linkingElement, "The only styles in a ShadowRoot should come "
                             "from <style>.");

  linkingElement->SetStyleSheet(aSheet); 

  
  
  for (size_t i = 0; i <= mProtoBinding->SheetCount(); i++) {
    if (i == mProtoBinding->SheetCount()) {
      mProtoBinding->AppendStyleSheet(aSheet);
      break;
    }

    nsINode* sheetOwnerNode = mProtoBinding->StyleSheetAt(i)->GetOwnerNode();
    if (nsContentUtils::PositionIsBefore(aLinkingContent, sheetOwnerNode)) {
      mProtoBinding->InsertStyleSheetAt(i, aSheet);
      break;
    }
  }

  if (aSheet->IsApplicable()) {
    StyleSheetChanged();
  }
}

void
ShadowRoot::RemoveSheet(CSSStyleSheet* aSheet)
{
  mProtoBinding->RemoveStyleSheet(aSheet);

  if (aSheet->IsApplicable()) {
    StyleSheetChanged();
  }
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
ShadowRoot::SetYoungerShadow(ShadowRoot* aYoungerShadow)
{
  mYoungerShadow = aYoungerShadow;
  mYoungerShadow->mOlderShadow = this;

  ChangePoolHost(mYoungerShadow->GetShadowElement());
}

void
ShadowRoot::RemoveDestInsertionPoint(nsIContent* aInsertionPoint,
                                     nsTArray<nsIContent*>& aDestInsertionPoints)
{
  
  
  
  int32_t index = aDestInsertionPoints.IndexOf(aInsertionPoint);

  
  
  if (index >= 0) {
    aDestInsertionPoints.SetLength(index);
  }
}

void
ShadowRoot::DistributeSingleNode(nsIContent* aContent)
{
  
  HTMLContentElement* insertionPoint = nullptr;
  for (uint32_t i = 0; i < mInsertionPoints.Length(); i++) {
    if (mInsertionPoints[i]->Match(aContent)) {
      if (mInsertionPoints[i]->MatchedNodes().Contains(aContent)) {
        
        return;
      }

      
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
    MOZ_ASSERT(mPoolHost, "Where did the content come from if there is no pool host?");
    ExplicitChildIterator childIterator(mPoolHost);
    for (uint32_t i = 0; i < matchedNodes.Length(); i++) {
      
      
      if (childIterator.Seek(aContent, matchedNodes[i])) {
        
        insertionPoint->InsertMatchedNode(i, aContent);
        isIndexFound = true;
        break;
      }
    }

    if (!isIndexFound) {
      
      
      MOZ_ASSERT(childIterator.Seek(aContent),
                 "Trying to match a node that is not a candidate to be matched");
      insertionPoint->AppendMatchedNode(aContent);
    }

    
    
    
    
    if (insertionPoint->GetParent() == this &&
        mYoungerShadow && mYoungerShadow->GetShadowElement()) {
      mYoungerShadow->GetShadowElement()->DistributeSingleNode(aContent);
    }

    
    
    
    ShadowRoot* parentShadow = insertionPoint->GetParent()->GetShadowRoot();
    if (parentShadow) {
      parentShadow->DistributeSingleNode(aContent);
    }

    
    
    
    if (mShadowElement && mShadowElement == insertionPoint->GetParent()) {
      ShadowRoot* olderShadow = mShadowElement->GetOlderShadowRoot();
      if (olderShadow) {
        olderShadow->DistributeSingleNode(aContent);
      }
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

      mInsertionPoints[i]->RemoveMatchedNode(aContent);

      
      
      
      if (mInsertionPoints[i]->GetParent() == this) {
        if (mYoungerShadow && mYoungerShadow->GetShadowElement()) {
          mYoungerShadow->GetShadowElement()->RemoveDistributedNode(aContent);
        }
      }

      
      
      
      ShadowRoot* parentShadow = mInsertionPoints[i]->GetParent()->GetShadowRoot();
      if (parentShadow) {
        parentShadow->RemoveDistributedNode(aContent);
      }

      
      
      
      if (mShadowElement && mShadowElement == mInsertionPoints[i]->GetParent()) {
        ShadowRoot* olderShadow = mShadowElement->GetOlderShadowRoot();
        if (olderShadow) {
          olderShadow->RemoveDistributedNode(aContent);
        }
      }

      break;
    }
  }
}

void
ShadowRoot::DistributeAllNodes()
{
  
  nsTArray<nsIContent*> nodePool;

  
  
  if (mPoolHost) {
    ExplicitChildIterator childIterator(mPoolHost);
    for (nsIContent* content = childIterator.GetNextChild();
         content;
         content = childIterator.GetNextChild()) {
      nodePool.AppendElement(content);
    }
  }

  nsTArray<ShadowRoot*> shadowsToUpdate;

  for (uint32_t i = 0; i < mInsertionPoints.Length(); i++) {
    mInsertionPoints[i]->ClearMatchedNodes();
    
    for (uint32_t j = 0; j < nodePool.Length(); j++) {
      if (mInsertionPoints[i]->Match(nodePool[j])) {
        mInsertionPoints[i]->AppendMatchedNode(nodePool[j]);
        nodePool.RemoveElementAt(j--);
      }
    }

    
    
    nsIContent* insertionParent = mInsertionPoints[i]->GetParent();
    MOZ_ASSERT(insertionParent, "The only way for an insertion point to be in the"
                                "mInsertionPoints array is to be a descendant of a"
                                "ShadowRoot, in which case, it should have a parent");

    
    
    
    ShadowRoot* parentShadow = insertionParent->GetShadowRoot();
    if (parentShadow && !shadowsToUpdate.Contains(parentShadow)) {
      shadowsToUpdate.AppendElement(parentShadow);
    }
  }

  
  
  
  if (mShadowElement && mOlderShadow) {
    mOlderShadow->DistributeAllNodes();
  }

  
  
  
  if (mYoungerShadow && mYoungerShadow->GetShadowElement()) {
    mYoungerShadow->GetShadowElement()->DistributeAllNodes();
  }

  for (uint32_t i = 0; i < shadowsToUpdate.Length(); i++) {
    shadowsToUpdate[i]->DistributeAllNodes();
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

bool
ShadowRoot::ApplyAuthorStyles()
{
  return mProtoBinding->InheritsStyle();
}

void
ShadowRoot::SetApplyAuthorStyles(bool aApplyAuthorStyles)
{
  mProtoBinding->SetInheritsStyle(aApplyAuthorStyles);

  nsIPresShell* shell = OwnerDoc()->GetShell();
  if (shell) {
    OwnerDoc()->BeginUpdate(UPDATE_STYLE);
    shell->RecordShadowStyleChange(this);
    OwnerDoc()->EndUpdate(UPDATE_STYLE);
  }
}

StyleSheetList*
ShadowRoot::StyleSheets()
{
  if (!mStyleSheetList) {
    mStyleSheetList = new ShadowRootStyleSheetList(this);
  }

  return mStyleSheetList;
}

void
ShadowRoot::SetShadowElement(HTMLShadowElement* aShadowElement)
{
  
  
  
  if (mShadowElement) {
    mShadowElement->SetProjectedShadow(nullptr);
  }

  if (mOlderShadow) {
    
    mOlderShadow->ChangePoolHost(aShadowElement);
  }

  
  
  mShadowElement = aShadowElement;
  if (mShadowElement) {
    mShadowElement->SetProjectedShadow(mOlderShadow);
  }
}

void
ShadowRoot::ChangePoolHost(nsIContent* aNewHost)
{
  if (mPoolHost) {
    mPoolHost->RemoveMutationObserver(this);
  }

  
  
  for (uint32_t i = 0; i < mInsertionPoints.Length(); i++) {
    mInsertionPoints[i]->ClearMatchedNodes();
  }

  mPoolHost = aNewHost;
  if (mPoolHost) {
    mPoolHost->AddMutationObserver(this);
  }
}

bool
ShadowRoot::IsShadowInsertionPoint(nsIContent* aContent)
{
  if (aContent && aContent->IsHTML(nsGkAtoms::shadow)) {
    HTMLShadowElement* shadowElem = static_cast<HTMLShadowElement*>(aContent);
    return shadowElem->IsInsertionPoint();
  }
  return false;
}







bool
ShadowRoot::IsPooledNode(nsIContent* aContent, nsIContent* aContainer,
                         nsIContent* aHost)
{
  if (nsContentUtils::IsContentInsertionPoint(aContent) ||
      IsShadowInsertionPoint(aContent)) {
    
    return false;
  }

  if (aContainer == aHost) {
    
    return true;
  }

  if (aContainer && aContainer->IsHTML(nsGkAtoms::content)) {
    
    HTMLContentElement* content = static_cast<HTMLContentElement*>(aContainer);
    return content->IsInsertionPoint() && content->MatchedNodes().IsEmpty() &&
           aContainer->GetParentNode() == aHost;
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
  if (!IsPooledNode(aElement, aElement->GetParent(), mPoolHost)) {
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
    
    if (nsContentUtils::IsContentInsertionPoint(aContainer)) {
      HTMLContentElement* content = static_cast<HTMLContentElement*>(aContainer);
      if (content->MatchedNodes().IsEmpty()) {
        currentChild->DestInsertionPoints().AppendElement(aContainer);
      }
    }

    if (IsPooledNode(currentChild, aContainer, mPoolHost)) {
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

  
  
  if (IsPooledNode(aChild, aContainer, mPoolHost)) {
    
    if (nsContentUtils::IsContentInsertionPoint(aContainer)) {
      HTMLContentElement* content = static_cast<HTMLContentElement*>(aContainer);
      if (content->MatchedNodes().IsEmpty()) {
        aChild->DestInsertionPoints().AppendElement(aContainer);
      }
    }

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

  
  
  if (nsContentUtils::IsContentInsertionPoint(aContainer)) {
    HTMLContentElement* content = static_cast<HTMLContentElement*>(aContainer);
    if (content->MatchedNodes().IsEmpty()) {
      aChild->DestInsertionPoints().Clear();
    }
  }

  
  
  if (IsPooledNode(aChild, aContainer, mPoolHost)) {
    RemoveDistributedNode(aChild);
  }
}

NS_IMPL_CYCLE_COLLECTION_INHERITED(ShadowRootStyleSheetList, StyleSheetList,
                                   mShadowRoot)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(ShadowRootStyleSheetList)
NS_INTERFACE_MAP_END_INHERITING(StyleSheetList)

NS_IMPL_ADDREF_INHERITED(ShadowRootStyleSheetList, StyleSheetList)
NS_IMPL_RELEASE_INHERITED(ShadowRootStyleSheetList, StyleSheetList)

ShadowRootStyleSheetList::ShadowRootStyleSheetList(ShadowRoot* aShadowRoot)
  : mShadowRoot(aShadowRoot)
{
  MOZ_COUNT_CTOR(ShadowRootStyleSheetList);
}

ShadowRootStyleSheetList::~ShadowRootStyleSheetList()
{
  MOZ_COUNT_DTOR(ShadowRootStyleSheetList);
}

CSSStyleSheet*
ShadowRootStyleSheetList::IndexedGetter(uint32_t aIndex, bool& aFound)
{
  aFound = aIndex < mShadowRoot->mProtoBinding->SheetCount();

  if (!aFound) {
    return nullptr;
  }

  return mShadowRoot->mProtoBinding->StyleSheetAt(aIndex);
}

uint32_t
ShadowRootStyleSheetList::Length()
{
  return mShadowRoot->mProtoBinding->SheetCount();
}

