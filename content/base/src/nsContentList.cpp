











#include "nsContentList.h"
#include "nsIContent.h"
#include "nsIDOMNode.h"
#include "nsIDocument.h"
#include "mozilla/dom/Element.h"
#include "nsWrapperCacheInlines.h"
#include "nsContentUtils.h"
#include "nsCCUncollectableMarker.h"
#include "nsGkAtoms.h"
#include "mozilla/dom/HTMLCollectionBinding.h"
#include "mozilla/dom/NodeListBinding.h"
#include "mozilla/dom/BindingUtils.h"
#include "mozilla/Likely.h"
#include "nsGenericHTMLElement.h"
#include <algorithm>


#include "nsIDOMHTMLFormElement.h"

#include "pldhash.h"

#ifdef DEBUG_CONTENT_LIST
#include "nsIContentIterator.h"
#define ASSERT_IN_SYNC AssertInSync()
#else
#define ASSERT_IN_SYNC PR_BEGIN_MACRO PR_END_MACRO
#endif

using namespace mozilla::dom;

nsBaseContentList::~nsBaseContentList()
{
}

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsBaseContentList)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mElements)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsBaseContentList)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
  if (nsCCUncollectableMarker::sGeneration && tmp->IsBlack() &&
      MOZ_LIKELY(!cb.WantAllTraces())) {
    return NS_SUCCESS_INTERRUPTED_TRAVERSE;
  }
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mElements)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END
NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(nsBaseContentList)
  NS_IMPL_CYCLE_COLLECTION_TRACE_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_BEGIN(nsBaseContentList)
  if (nsCCUncollectableMarker::sGeneration && tmp->IsBlack()) {
    for (uint32_t i = 0; i < tmp->mElements.Length(); ++i) {
      nsIContent* c = tmp->mElements[i];
      if (c->IsPurple()) {
        c->RemovePurple();
      }
      Element::MarkNodeChildren(c);
    }
    return true;
  }
NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_END

NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_IN_CC_BEGIN(nsBaseContentList)
  return nsCCUncollectableMarker::sGeneration && tmp->IsBlack();
NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_IN_CC_END

NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_THIS_BEGIN(nsBaseContentList)
  return nsCCUncollectableMarker::sGeneration && tmp->IsBlack();
NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_THIS_END

#define NS_CONTENT_LIST_INTERFACES(_class)                                    \
    NS_INTERFACE_TABLE_ENTRY(_class, nsINodeList)                             \
    NS_INTERFACE_TABLE_ENTRY(_class, nsIDOMNodeList)


NS_INTERFACE_TABLE_HEAD(nsBaseContentList)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_TABLE2(nsBaseContentList, nsINodeList, nsIDOMNodeList)
  NS_INTERFACE_TABLE_TO_MAP_SEGUE_CYCLE_COLLECTION(nsBaseContentList)
NS_INTERFACE_MAP_END


NS_IMPL_CYCLE_COLLECTING_ADDREF(nsBaseContentList)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsBaseContentList)


NS_IMETHODIMP
nsBaseContentList::GetLength(uint32_t* aLength)
{
  *aLength = mElements.Length();

  return NS_OK;
}

NS_IMETHODIMP
nsBaseContentList::Item(uint32_t aIndex, nsIDOMNode** aReturn)
{
  nsISupports *tmp = Item(aIndex);

  if (!tmp) {
    *aReturn = nullptr;

    return NS_OK;
  }

  return CallQueryInterface(tmp, aReturn);
}

nsIContent*
nsBaseContentList::Item(uint32_t aIndex)
{
  return mElements.SafeElementAt(aIndex);
}


int32_t
nsBaseContentList::IndexOf(nsIContent *aContent, bool aDoFlush)
{
  return mElements.IndexOf(aContent);
}

int32_t
nsBaseContentList::IndexOf(nsIContent* aContent)
{
  return IndexOf(aContent, true);
}

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsSimpleContentList,
                                                  nsBaseContentList)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mRoot)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(nsSimpleContentList,
                                                nsBaseContentList)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mRoot)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(nsSimpleContentList)
NS_INTERFACE_MAP_END_INHERITING(nsBaseContentList)


NS_IMPL_ADDREF_INHERITED(nsSimpleContentList, nsBaseContentList)
NS_IMPL_RELEASE_INHERITED(nsSimpleContentList, nsBaseContentList)

JSObject*
nsSimpleContentList::WrapObject(JSContext *cx, JSObject *scope,
                                bool *triedToWrap)
{
  return NodeListBinding::Wrap(cx, scope, this, triedToWrap);
}



nsFormContentList::nsFormContentList(nsIContent *aForm,
                                     nsBaseContentList& aContentList)
  : nsSimpleContentList(aForm)
{

  

  uint32_t i, length = 0;
  aContentList.GetLength(&length);

  for (i = 0; i < length; i++) {
    nsIContent *c = aContentList.Item(i);
    if (c && nsContentUtils::BelongsInForm(aForm, c)) {
      AppendElement(c);
    }
  }
}


static PLDHashTable gContentListHashTable;

struct ContentListHashEntry : public PLDHashEntryHdr
{
  nsContentList* mContentList;
};

static PLDHashNumber
ContentListHashtableHashKey(PLDHashTable *table, const void *key)
{
  const nsContentListKey* list = static_cast<const nsContentListKey *>(key);
  return list->GetHash();
}

static bool
ContentListHashtableMatchEntry(PLDHashTable *table,
                               const PLDHashEntryHdr *entry,
                               const void *key)
{
  const ContentListHashEntry *e =
    static_cast<const ContentListHashEntry *>(entry);
  const nsContentList* list = e->mContentList;
  const nsContentListKey* ourKey = static_cast<const nsContentListKey *>(key);

  return list->MatchesKey(*ourKey);
}

already_AddRefed<nsContentList>
NS_GetContentList(nsINode* aRootNode, 
                  int32_t  aMatchNameSpaceId,
                  const nsAString& aTagname)
                  
{
  NS_ASSERTION(aRootNode, "content list has to have a root");

  nsContentList* list = nullptr;

  static PLDHashTableOps hash_table_ops =
  {
    PL_DHashAllocTable,
    PL_DHashFreeTable,
    ContentListHashtableHashKey,
    ContentListHashtableMatchEntry,
    PL_DHashMoveEntryStub,
    PL_DHashClearEntryStub,
    PL_DHashFinalizeStub
  };

  
  if (!gContentListHashTable.ops) {
    bool success = PL_DHashTableInit(&gContentListHashTable,
                                       &hash_table_ops, nullptr,
                                       sizeof(ContentListHashEntry),
                                       16);

    if (!success) {
      gContentListHashTable.ops = nullptr;
    }
  }
  
  ContentListHashEntry *entry = nullptr;
  
  if (gContentListHashTable.ops) {
    nsContentListKey hashKey(aRootNode, aMatchNameSpaceId, aTagname);
    
    
    
    entry = static_cast<ContentListHashEntry *>
                       (PL_DHashTableOperate(&gContentListHashTable,
                                             &hashKey,
                                             PL_DHASH_ADD));
    if (entry)
      list = entry->mContentList;
  }

  if (!list) {
    
    
    nsCOMPtr<nsIAtom> xmlAtom = do_GetAtom(aTagname);
    nsCOMPtr<nsIAtom> htmlAtom;
    if (aMatchNameSpaceId == kNameSpaceID_Unknown) {
      nsAutoString lowercaseName;
      nsContentUtils::ASCIIToLower(aTagname, lowercaseName);
      htmlAtom = do_GetAtom(lowercaseName);
    } else {
      htmlAtom = xmlAtom;
    }
    list = new nsContentList(aRootNode, aMatchNameSpaceId,
                             htmlAtom, xmlAtom);
    if (entry) {
      entry->mContentList = list;
    }
  }

  NS_ADDREF(list);

  return list;
}

#ifdef DEBUG
const nsCacheableFuncStringContentList::ContentListType
  nsCacheableFuncStringNodeList::sType = nsCacheableFuncStringContentList::eNodeList;
const nsCacheableFuncStringContentList::ContentListType
  nsCacheableFuncStringHTMLCollection::sType = nsCacheableFuncStringContentList::eHTMLCollection;
#endif

JSObject*
nsCacheableFuncStringNodeList::WrapObject(JSContext *cx, JSObject *scope,
                                          bool *triedToWrap)
{
  return NodeListBinding::Wrap(cx, scope, this, triedToWrap);
}


JSObject*
nsCacheableFuncStringHTMLCollection::WrapObject(JSContext *cx, JSObject *scope,
                                                bool *triedToWrap)
{
  return HTMLCollectionBinding::Wrap(cx, scope, this, triedToWrap);
}


static PLDHashTable gFuncStringContentListHashTable;

struct FuncStringContentListHashEntry : public PLDHashEntryHdr
{
  nsCacheableFuncStringContentList* mContentList;
};

static PLDHashNumber
FuncStringContentListHashtableHashKey(PLDHashTable *table, const void *key)
{
  const nsFuncStringCacheKey* funcStringKey =
    static_cast<const nsFuncStringCacheKey *>(key);
  return funcStringKey->GetHash();
}

static bool
FuncStringContentListHashtableMatchEntry(PLDHashTable *table,
                               const PLDHashEntryHdr *entry,
                               const void *key)
{
  const FuncStringContentListHashEntry *e =
    static_cast<const FuncStringContentListHashEntry *>(entry);
  const nsFuncStringCacheKey* ourKey =
    static_cast<const nsFuncStringCacheKey *>(key);

  return e->mContentList->Equals(ourKey);
}

template<class ListType>
already_AddRefed<nsContentList>
GetFuncStringContentList(nsINode* aRootNode,
                         nsContentListMatchFunc aFunc,
                         nsContentListDestroyFunc aDestroyFunc,
                         nsFuncStringContentListDataAllocator aDataAllocator,
                         const nsAString& aString)
{
  NS_ASSERTION(aRootNode, "content list has to have a root");

  nsCacheableFuncStringContentList* list = nullptr;

  static PLDHashTableOps hash_table_ops =
  {
    PL_DHashAllocTable,
    PL_DHashFreeTable,
    FuncStringContentListHashtableHashKey,
    FuncStringContentListHashtableMatchEntry,
    PL_DHashMoveEntryStub,
    PL_DHashClearEntryStub,
    PL_DHashFinalizeStub
  };

  
  if (!gFuncStringContentListHashTable.ops) {
    bool success = PL_DHashTableInit(&gFuncStringContentListHashTable,
                                       &hash_table_ops, nullptr,
                                       sizeof(FuncStringContentListHashEntry),
                                       16);

    if (!success) {
      gFuncStringContentListHashTable.ops = nullptr;
    }
  }

  FuncStringContentListHashEntry *entry = nullptr;
  
  if (gFuncStringContentListHashTable.ops) {
    nsFuncStringCacheKey hashKey(aRootNode, aFunc, aString);

    
    
    entry = static_cast<FuncStringContentListHashEntry *>
                       (PL_DHashTableOperate(&gFuncStringContentListHashTable,
                                             &hashKey,
                                             PL_DHASH_ADD));
    if (entry) {
      list = entry->mContentList;
#ifdef DEBUG
      MOZ_ASSERT_IF(list, list->mType == ListType::sType);
#endif
    }
  }

  if (!list) {
    
    
    list = new ListType(aRootNode, aFunc, aDestroyFunc, aDataAllocator,
                        aString);
    if (entry) {
      entry->mContentList = list;
    }
  }

  NS_ADDREF(list);

  

  return list;
}

already_AddRefed<nsContentList>
NS_GetFuncStringNodeList(nsINode* aRootNode,
                         nsContentListMatchFunc aFunc,
                         nsContentListDestroyFunc aDestroyFunc,
                         nsFuncStringContentListDataAllocator aDataAllocator,
                         const nsAString& aString)
{
  return GetFuncStringContentList<nsCacheableFuncStringNodeList>(aRootNode,
                                                                 aFunc,
                                                                 aDestroyFunc,
                                                                 aDataAllocator,
                                                                 aString);
}

already_AddRefed<nsContentList>
NS_GetFuncStringHTMLCollection(nsINode* aRootNode,
                               nsContentListMatchFunc aFunc,
                               nsContentListDestroyFunc aDestroyFunc,
                               nsFuncStringContentListDataAllocator aDataAllocator,
                               const nsAString& aString)
{
  return GetFuncStringContentList<nsCacheableFuncStringHTMLCollection>(aRootNode,
                                                                       aFunc,
                                                                       aDestroyFunc,
                                                                       aDataAllocator,
                                                                       aString);
}



nsContentList::nsContentList(nsINode* aRootNode,
                             int32_t aMatchNameSpaceId,
                             nsIAtom* aHTMLMatchAtom,
                             nsIAtom* aXMLMatchAtom,
                             bool aDeep)
  : nsBaseContentList(),
    mRootNode(aRootNode),
    mMatchNameSpaceId(aMatchNameSpaceId),
    mHTMLMatchAtom(aHTMLMatchAtom),
    mXMLMatchAtom(aXMLMatchAtom),
    mFunc(nullptr),
    mDestroyFunc(nullptr),
    mData(nullptr),
    mState(LIST_DIRTY),
    mDeep(aDeep),
    mFuncMayDependOnAttr(false)
{
  NS_ASSERTION(mRootNode, "Must have root");
  if (nsGkAtoms::_asterix == mHTMLMatchAtom) {
    NS_ASSERTION(mXMLMatchAtom == nsGkAtoms::_asterix, "HTML atom and XML atom are not both asterix?");
    mMatchAll = true;
  }
  else {
    mMatchAll = false;
  }
  mRootNode->AddMutationObserver(this);

  
  
  
  
  
  nsIDocument* doc = mRootNode->GetCurrentDoc();
  mFlushesNeeded = doc && !doc->IsHTML();
}

nsContentList::nsContentList(nsINode* aRootNode,
                             nsContentListMatchFunc aFunc,
                             nsContentListDestroyFunc aDestroyFunc,
                             void* aData,
                             bool aDeep,
                             nsIAtom* aMatchAtom,
                             int32_t aMatchNameSpaceId,
                             bool aFuncMayDependOnAttr)
  : nsBaseContentList(),
    mRootNode(aRootNode),
    mMatchNameSpaceId(aMatchNameSpaceId),
    mHTMLMatchAtom(aMatchAtom),
    mXMLMatchAtom(aMatchAtom),
    mFunc(aFunc),
    mDestroyFunc(aDestroyFunc),
    mData(aData),
    mState(LIST_DIRTY),
    mMatchAll(false),
    mDeep(aDeep),
    mFuncMayDependOnAttr(aFuncMayDependOnAttr)
{
  NS_ASSERTION(mRootNode, "Must have root");
  mRootNode->AddMutationObserver(this);

  
  
  
  
  
  nsIDocument* doc = mRootNode->GetCurrentDoc();
  mFlushesNeeded = doc && !doc->IsHTML();
}

nsContentList::~nsContentList()
{
  RemoveFromHashtable();
  if (mRootNode) {
    mRootNode->RemoveMutationObserver(this);
  }

  if (mDestroyFunc) {
    
    (*mDestroyFunc)(mData);
  }
}

JSObject*
nsContentList::WrapObject(JSContext *cx, JSObject *scope, bool *triedToWrap)
{
  return HTMLCollectionBinding::Wrap(cx, scope, this, triedToWrap);
}

NS_IMPL_ISUPPORTS_INHERITED3(nsContentList, nsBaseContentList,
                             nsIHTMLCollection, nsIDOMHTMLCollection,
                             nsIMutationObserver)

uint32_t
nsContentList::Length(bool aDoFlush)
{
  BringSelfUpToDate(aDoFlush);
    
  return mElements.Length();
}

nsIContent *
nsContentList::Item(uint32_t aIndex, bool aDoFlush)
{
  if (mRootNode && aDoFlush && mFlushesNeeded) {
    
    nsIDocument* doc = mRootNode->GetCurrentDoc();
    if (doc) {
      
      doc->FlushPendingNotifications(Flush_ContentAndNotify);
    }
  }

  if (mState != LIST_UP_TO_DATE)
    PopulateSelf(std::min(aIndex, UINT32_MAX - 1) + 1);

  ASSERT_IN_SYNC;
  NS_ASSERTION(!mRootNode || mState != LIST_DIRTY,
               "PopulateSelf left the list in a dirty (useless) state!");

  return mElements.SafeElementAt(aIndex);
}

nsIContent *
nsContentList::NamedItem(const nsAString& aName, bool aDoFlush)
{
  BringSelfUpToDate(aDoFlush);
    
  uint32_t i, count = mElements.Length();

  
  nsCOMPtr<nsIAtom> name = do_GetAtom(aName);
  NS_ENSURE_TRUE(name, nullptr);

  for (i = 0; i < count; i++) {
    nsIContent *content = mElements[i];
    
    if (content &&
        (content->AttrValueIs(kNameSpaceID_None, nsGkAtoms::name,
                              name, eCaseMatters) ||
         content->AttrValueIs(kNameSpaceID_None, nsGkAtoms::id,
                              name, eCaseMatters))) {
      return content;
    }
  }

  return nullptr;
}

void
nsContentList::GetSupportedNames(nsTArray<nsString>& aNames)
{
  BringSelfUpToDate(true);

  nsAutoTArray<nsIAtom*, 8> atoms;
  for (uint32_t i = 0; i < mElements.Length(); ++i) {
    nsIContent *content = mElements.ElementAt(i);
    nsGenericHTMLElement* el = nsGenericHTMLElement::FromContent(content);
    if (el) {
      
      
      
      
      const nsAttrValue* val = el->GetParsedAttr(nsGkAtoms::name);
      if (val && val->Type() == nsAttrValue::eAtom) {
        nsIAtom* name = val->GetAtomValue();
        if (!atoms.Contains(name)) {
          atoms.AppendElement(name);
        }
      }
    }
    if (content->HasID()) {
      nsIAtom* id = content->GetID();
      if (!atoms.Contains(id)) {
        atoms.AppendElement(id);
      }
    }
  }

  aNames.SetCapacity(atoms.Length());
  for (uint32_t i = 0; i < atoms.Length(); ++i) {
    aNames.AppendElement(nsDependentAtomString(atoms[i]));
  }
}

int32_t
nsContentList::IndexOf(nsIContent *aContent, bool aDoFlush)
{
  BringSelfUpToDate(aDoFlush);
    
  return mElements.IndexOf(aContent);
}

int32_t
nsContentList::IndexOf(nsIContent* aContent)
{
  return IndexOf(aContent, true);
}

void
nsContentList::NodeWillBeDestroyed(const nsINode* aNode)
{
  

  RemoveFromCaches();
  mRootNode = nullptr;

  
  
  SetDirty();
}

NS_IMETHODIMP
nsContentList::GetLength(uint32_t* aLength)
{
  *aLength = Length(true);

  return NS_OK;
}

NS_IMETHODIMP
nsContentList::Item(uint32_t aIndex, nsIDOMNode** aReturn)
{
  nsINode* node = Item(aIndex);

  if (node) {
    return CallQueryInterface(node, aReturn);
  }

  *aReturn = nullptr;

  return NS_OK;
}

NS_IMETHODIMP
nsContentList::NamedItem(const nsAString& aName, nsIDOMNode** aReturn)
{
  nsIContent *content = NamedItem(aName, true);

  if (content) {
    return CallQueryInterface(content, aReturn);
  }

  *aReturn = nullptr;

  return NS_OK;
}

Element*
nsContentList::GetElementAt(uint32_t aIndex)
{
  return static_cast<Element*>(Item(aIndex, true));
}

nsIContent*
nsContentList::Item(uint32_t aIndex)
{
  return GetElementAt(aIndex);
}

JSObject*
nsContentList::NamedItem(JSContext* cx, const nsAString& name,
                         mozilla::ErrorResult& error)
{
  nsIContent *item = NamedItem(name, true);
  if (!item) {
    return nullptr;
  }
  JSObject* wrapper = GetWrapper();
  JSAutoCompartment ac(cx, wrapper);
  JS::Value v;
  if (!mozilla::dom::WrapObject(cx, wrapper, item, item, nullptr, &v)) {
    error.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }
  return &v.toObject();
}

void
nsContentList::AttributeChanged(nsIDocument *aDocument, Element* aElement,
                                int32_t aNameSpaceID, nsIAtom* aAttribute,
                                int32_t aModType)
{
  NS_PRECONDITION(aElement, "Must have a content node to work with");
  
  if (!mFunc || !mFuncMayDependOnAttr || mState == LIST_DIRTY ||
      !MayContainRelevantNodes(aElement->GetParentNode()) ||
      !nsContentUtils::IsInSameAnonymousTree(mRootNode, aElement)) {
    
    
    return;
  }
  
  if (Match(aElement)) {
    if (mElements.IndexOf(aElement) == mElements.NoIndex) {
      
      
      
      SetDirty();
    }
  } else {
    
    
    
    
    mElements.RemoveElement(aElement);
  }
}

void
nsContentList::ContentAppended(nsIDocument* aDocument, nsIContent* aContainer,
                               nsIContent* aFirstNewContent,
                               int32_t aNewIndexInContainer)
{
  NS_PRECONDITION(aContainer, "Can't get at the new content if no container!");
  
  





  if (mState == LIST_DIRTY ||
      !nsContentUtils::IsInSameAnonymousTree(mRootNode, aContainer) ||
      !MayContainRelevantNodes(aContainer))
    return;

  







  
  int32_t count = aContainer->GetChildCount();

  if (count > 0) {
    uint32_t ourCount = mElements.Length();
    bool appendToList = false;
    if (ourCount == 0) {
      appendToList = true;
    } else {
      nsIContent* ourLastContent = mElements[ourCount - 1];
      



      if (nsContentUtils::PositionIsBefore(ourLastContent, aFirstNewContent)) {
        appendToList = true;
      }
    }
    

    if (!appendToList) {
      
      
      for (nsIContent* cur = aFirstNewContent; cur; cur = cur->GetNextSibling()) {
        if (MatchSelf(cur)) {
          
          
          SetDirty();
          break;
        }
      }

      ASSERT_IN_SYNC;
      return;
    }

    





    if (mState == LIST_LAZY) 
      return;

    



    if (mDeep) {
      for (nsIContent* cur = aFirstNewContent;
           cur;
           cur = cur->GetNextNode(aContainer)) {
        if (cur->IsElement() && Match(cur->AsElement())) {
          mElements.AppendElement(cur);
        }
      }
    } else {
      for (nsIContent* cur = aFirstNewContent; cur; cur = cur->GetNextSibling()) {
        if (cur->IsElement() && Match(cur->AsElement())) {
          mElements.AppendElement(cur);
        }
      }
    }

    ASSERT_IN_SYNC;
  }
}

void
nsContentList::ContentInserted(nsIDocument *aDocument,
                               nsIContent* aContainer,
                               nsIContent* aChild,
                               int32_t aIndexInContainer)
{
  
  
  
  if (mState != LIST_DIRTY &&
      MayContainRelevantNodes(NODE_FROM(aContainer, aDocument)) &&
      nsContentUtils::IsInSameAnonymousTree(mRootNode, aChild) &&
      MatchSelf(aChild)) {
    SetDirty();
  }

  ASSERT_IN_SYNC;
}
 
void
nsContentList::ContentRemoved(nsIDocument *aDocument,
                              nsIContent* aContainer,
                              nsIContent* aChild,
                              int32_t aIndexInContainer,
                              nsIContent* aPreviousSibling)
{
  
  
  
  if (mState != LIST_DIRTY &&
      MayContainRelevantNodes(NODE_FROM(aContainer, aDocument)) &&
      nsContentUtils::IsInSameAnonymousTree(mRootNode, aChild) &&
      MatchSelf(aChild)) {
    SetDirty();
  }

  ASSERT_IN_SYNC;
}

bool
nsContentList::Match(Element *aElement)
{
  if (mFunc) {
    return (*mFunc)(aElement, mMatchNameSpaceId, mXMLMatchAtom, mData);
  }

  if (!mXMLMatchAtom)
    return false;

  nsINodeInfo *ni = aElement->NodeInfo();
 
  bool unknown = mMatchNameSpaceId == kNameSpaceID_Unknown;
  bool wildcard = mMatchNameSpaceId == kNameSpaceID_Wildcard;
  bool toReturn = mMatchAll;
  if (!unknown && !wildcard)
    toReturn &= ni->NamespaceEquals(mMatchNameSpaceId);

  if (toReturn)
    return toReturn;

  bool matchHTML = aElement->GetNameSpaceID() == kNameSpaceID_XHTML &&
    aElement->OwnerDoc()->IsHTML();
 
  if (unknown) {
    return matchHTML ? ni->QualifiedNameEquals(mHTMLMatchAtom) :
                       ni->QualifiedNameEquals(mXMLMatchAtom);
  }
  
  if (wildcard) {
    return matchHTML ? ni->Equals(mHTMLMatchAtom) :
                       ni->Equals(mXMLMatchAtom);
  }
  
  return matchHTML ? ni->Equals(mHTMLMatchAtom, mMatchNameSpaceId) :
                     ni->Equals(mXMLMatchAtom, mMatchNameSpaceId);
}

bool 
nsContentList::MatchSelf(nsIContent *aContent)
{
  NS_PRECONDITION(aContent, "Can't match null stuff, you know");
  NS_PRECONDITION(mDeep || aContent->GetParentNode() == mRootNode,
                  "MatchSelf called on a node that we can't possibly match");

  if (!aContent->IsElement()) {
    return false;
  }
  
  if (Match(aContent->AsElement()))
    return true;

  if (!mDeep)
    return false;

  for (nsIContent* cur = aContent->GetFirstChild();
       cur;
       cur = cur->GetNextNode(aContent)) {
    if (cur->IsElement() && Match(cur->AsElement())) {
      return true;
    }
  }
  
  return false;
}

void 
nsContentList::PopulateSelf(uint32_t aNeededLength)
{
  if (!mRootNode) {
    return;
  }

  ASSERT_IN_SYNC;

  uint32_t count = mElements.Length();
  NS_ASSERTION(mState != LIST_DIRTY || count == 0,
               "Reset() not called when setting state to LIST_DIRTY?");

  if (count >= aNeededLength) 
    return;

  uint32_t elementsToAppend = aNeededLength - count;
#ifdef DEBUG
  uint32_t invariant = elementsToAppend + mElements.Length();
#endif

  if (mDeep) {
    
    
    nsINode* cur = count ? mElements[count - 1] : mRootNode;
    do {
      cur = cur->GetNextNode(mRootNode);
      if (!cur) {
        break;
      }
      if (cur->IsElement() && Match(cur->AsElement())) {
        
        mElements.AppendElement(cur->AsElement());
        --elementsToAppend;
      }
    } while (elementsToAppend);
  } else {
    nsIContent* cur =
      count ? mElements[count-1]->GetNextSibling() : mRootNode->GetFirstChild();
    for ( ; cur && elementsToAppend; cur = cur->GetNextSibling()) {
      if (cur->IsElement() && Match(cur->AsElement())) {
        mElements.AppendElement(cur);
        --elementsToAppend;
      }
    }
  }

  NS_ASSERTION(elementsToAppend + mElements.Length() == invariant,
               "Something is awry!");

  if (elementsToAppend != 0)
    mState = LIST_UP_TO_DATE;
  else
    mState = LIST_LAZY;

  ASSERT_IN_SYNC;
}

void
nsContentList::RemoveFromHashtable()
{
  if (mFunc) {
    
    return;
  }
  
  if (!gContentListHashTable.ops)
    return;

  nsDependentAtomString str(mXMLMatchAtom);
  nsContentListKey key(mRootNode, mMatchNameSpaceId, str);
  PL_DHashTableOperate(&gContentListHashTable,
                       &key,
                       PL_DHASH_REMOVE);

  if (gContentListHashTable.entryCount == 0) {
    PL_DHashTableFinish(&gContentListHashTable);
    gContentListHashTable.ops = nullptr;
  }
}

void
nsContentList::BringSelfUpToDate(bool aDoFlush)
{
  if (mRootNode && aDoFlush && mFlushesNeeded) {
    
    nsIDocument* doc = mRootNode->GetCurrentDoc();
    if (doc) {
      
      doc->FlushPendingNotifications(Flush_ContentAndNotify);
    }
  }

  if (mState != LIST_UP_TO_DATE)
    PopulateSelf(uint32_t(-1));
    
  ASSERT_IN_SYNC;
  NS_ASSERTION(!mRootNode || mState == LIST_UP_TO_DATE,
               "PopulateSelf dod not bring content list up to date!");
}

nsCacheableFuncStringContentList::~nsCacheableFuncStringContentList()
{
  RemoveFromFuncStringHashtable();
}

void
nsCacheableFuncStringContentList::RemoveFromFuncStringHashtable()
{
  if (!gFuncStringContentListHashTable.ops) {
    return;
  }

  nsFuncStringCacheKey key(mRootNode, mFunc, mString);
  PL_DHashTableOperate(&gFuncStringContentListHashTable,
                       &key,
                       PL_DHASH_REMOVE);

  if (gFuncStringContentListHashTable.entryCount == 0) {
    PL_DHashTableFinish(&gFuncStringContentListHashTable);
    gFuncStringContentListHashTable.ops = nullptr;
  }
}

#ifdef DEBUG_CONTENT_LIST
void
nsContentList::AssertInSync()
{
  if (mState == LIST_DIRTY) {
    return;
  }

  if (!mRootNode) {
    NS_ASSERTION(mElements.Length() == 0 && mState == LIST_DIRTY,
                 "Empty iterator isn't quite empty?");
    return;
  }

  
  
  nsIContent *root;
  if (mRootNode->IsNodeOfType(nsINode::eDOCUMENT)) {
    root = static_cast<nsIDocument*>(mRootNode)->GetRootElement();
  }
  else {
    root = static_cast<nsIContent*>(mRootNode);
  }

  nsCOMPtr<nsIContentIterator> iter;
  if (mDeep) {
    iter = NS_NewPreContentIterator();
    iter->Init(root);
    iter->First();
  }

  uint32_t cnt = 0, index = 0;
  while (true) {
    if (cnt == mElements.Length() && mState == LIST_LAZY) {
      break;
    }

    nsIContent *cur = mDeep ? iter->GetCurrentNode() :
                              mRootNode->GetChildAt(index++);
    if (!cur) {
      break;
    }

    if (cur->IsElement() && Match(cur->AsElement())) {
      NS_ASSERTION(cnt < mElements.Length() && mElements[cnt] == cur,
                   "Elements is out of sync");
      ++cnt;
    }

    if (mDeep) {
      iter->Next();
    }
  }

  NS_ASSERTION(cnt == mElements.Length(), "Too few elements");
}
#endif
