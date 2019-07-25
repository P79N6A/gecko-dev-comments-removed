











































#include "nsContentList.h"
#include "nsIContent.h"
#include "nsIDOMNode.h"
#include "nsIDocument.h"
#include "nsGenericElement.h"

#include "nsContentUtils.h"

#include "nsGkAtoms.h"


#include "nsIDOMHTMLFormElement.h"

#include "pldhash.h"

#ifdef DEBUG_CONTENT_LIST
#include "nsIContentIterator.h"
nsresult
NS_NewPreContentIterator(nsIContentIterator** aInstancePtrResult);
#define ASSERT_IN_SYNC AssertInSync()
#else
#define ASSERT_IN_SYNC PR_BEGIN_MACRO PR_END_MACRO
#endif


using namespace mozilla::dom;

nsBaseContentList::~nsBaseContentList()
{
}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsBaseContentList)
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsBaseContentList)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMARRAY(mElements)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsBaseContentList)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMARRAY(mElements)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

#define NS_CONTENT_LIST_INTERFACES(_class)                                    \
    NS_INTERFACE_TABLE_ENTRY(_class, nsINodeList)                             \
    NS_INTERFACE_TABLE_ENTRY(_class, nsIDOMNodeList)

DOMCI_DATA(NodeList, nsBaseContentList)


NS_INTERFACE_TABLE_HEAD(nsBaseContentList)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_NODELIST_OFFSET_AND_INTERFACE_TABLE_BEGIN(nsBaseContentList)
    NS_CONTENT_LIST_INTERFACES(nsBaseContentList)
  NS_OFFSET_AND_INTERFACE_TABLE_END
  NS_OFFSET_AND_INTERFACE_TABLE_TO_MAP_SEGUE
  NS_INTERFACE_MAP_ENTRIES_CYCLE_COLLECTION(nsBaseContentList)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(NodeList)
NS_INTERFACE_MAP_END


NS_IMPL_CYCLE_COLLECTING_ADDREF(nsBaseContentList)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsBaseContentList)


NS_IMETHODIMP
nsBaseContentList::GetLength(PRUint32* aLength)
{
  *aLength = mElements.Count();

  return NS_OK;
}

NS_IMETHODIMP
nsBaseContentList::Item(PRUint32 aIndex, nsIDOMNode** aReturn)
{
  nsISupports *tmp = GetNodeAt(aIndex);

  if (!tmp) {
    *aReturn = nsnull;

    return NS_OK;
  }

  return CallQueryInterface(tmp, aReturn);
}

nsIContent*
nsBaseContentList::GetNodeAt(PRUint32 aIndex)
{
  return mElements.SafeObjectAt(aIndex);
}


PRInt32
nsBaseContentList::IndexOf(nsIContent *aContent, PRBool aDoFlush)
{
  return mElements.IndexOf(aContent);
}

PRInt32
nsBaseContentList::IndexOf(nsIContent* aContent)
{
  return IndexOf(aContent, PR_TRUE);
}

void nsBaseContentList::AppendElement(nsIContent *aContent) 
{
  mElements.AppendObject(aContent);
}

void nsBaseContentList::RemoveElement(nsIContent *aContent) 
{
  mElements.RemoveObject(aContent);
}

void nsBaseContentList::InsertElementAt(nsIContent* aContent, PRInt32 aIndex)
{
  NS_ASSERTION(aContent, "Element to insert must not be null");
  mElements.InsertObjectAt(aContent, aIndex);
}


NS_IMPL_CYCLE_COLLECTION_CLASS(nsSimpleContentList)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsSimpleContentList,
                                                  nsBaseContentList)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mRoot)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(nsSimpleContentList,
                                                nsBaseContentList)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mRoot)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(nsSimpleContentList)
NS_INTERFACE_MAP_END_INHERITING(nsBaseContentList)


NS_IMPL_ADDREF_INHERITED(nsSimpleContentList, nsBaseContentList)
NS_IMPL_RELEASE_INHERITED(nsSimpleContentList, nsBaseContentList)



nsFormContentList::nsFormContentList(nsIContent *aForm,
                                     nsBaseContentList& aContentList)
  : nsSimpleContentList(aForm)
{

  

  PRUint32 i, length = 0;
  aContentList.GetLength(&length);

  for (i = 0; i < length; i++) {
    nsIContent *c = aContentList.GetNodeAt(i);
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

static PRBool
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
                  PRInt32  aMatchNameSpaceId,
                  const nsAString& aTagname)
                  
{
  NS_ASSERTION(aRootNode, "content list has to have a root");

  nsContentList* list = nsnull;

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
    PRBool success = PL_DHashTableInit(&gContentListHashTable,
                                       &hash_table_ops, nsnull,
                                       sizeof(ContentListHashEntry),
                                       16);

    if (!success) {
      gContentListHashTable.ops = nsnull;
    }
  }
  
  ContentListHashEntry *entry = nsnull;
  
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

static PRBool
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

already_AddRefed<nsContentList>
NS_GetFuncStringContentList(nsINode* aRootNode,
                            nsContentListMatchFunc aFunc,
                            nsContentListDestroyFunc aDestroyFunc,
                            nsFuncStringContentListDataAllocator aDataAllocator,
                            const nsAString& aString)
{
  NS_ASSERTION(aRootNode, "content list has to have a root");

  nsCacheableFuncStringContentList* list = nsnull;

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
    PRBool success = PL_DHashTableInit(&gFuncStringContentListHashTable,
                                       &hash_table_ops, nsnull,
                                       sizeof(FuncStringContentListHashEntry),
                                       16);

    if (!success) {
      gFuncStringContentListHashTable.ops = nsnull;
    }
  }

  FuncStringContentListHashEntry *entry = nsnull;
  
  if (gFuncStringContentListHashTable.ops) {
    nsFuncStringCacheKey hashKey(aRootNode, aFunc, aString);

    
    
    entry = static_cast<FuncStringContentListHashEntry *>
                       (PL_DHashTableOperate(&gFuncStringContentListHashTable,
                                             &hashKey,
                                             PL_DHASH_ADD));
    if (entry)
      list = entry->mContentList;
  }

  if (!list) {
    
    
    list = new nsCacheableFuncStringContentList(aRootNode, aFunc, aDestroyFunc,
                                                aDataAllocator, aString);
    if (list && !list->AllocatedData()) {
      
      delete list;
      list = nsnull;
    }

    if (entry) {
      if (list)
        entry->mContentList = list;
      else
        PL_DHashTableRawRemove(&gContentListHashTable, entry);
    }

    NS_ENSURE_TRUE(list, nsnull);
  }

  NS_ADDREF(list);

  

  return list;
}



nsContentList::nsContentList(nsINode* aRootNode,
                             PRInt32 aMatchNameSpaceId,
                             nsIAtom* aHTMLMatchAtom,
                             nsIAtom* aXMLMatchAtom,
                             PRBool aDeep)
  : nsBaseContentList(),
    mRootNode(aRootNode),
    mMatchNameSpaceId(aMatchNameSpaceId),
    mHTMLMatchAtom(aHTMLMatchAtom),
    mXMLMatchAtom(aXMLMatchAtom),
    mFunc(nsnull),
    mDestroyFunc(nsnull),
    mData(nsnull),
    mState(LIST_DIRTY),
    mDeep(aDeep),
    mFuncMayDependOnAttr(PR_FALSE)
{
  NS_ASSERTION(mRootNode, "Must have root");
  if (nsGkAtoms::_asterix == mHTMLMatchAtom) {
    NS_ASSERTION(mXMLMatchAtom == nsGkAtoms::_asterix, "HTML atom and XML atom are not both asterix?");
    mMatchAll = PR_TRUE;
  }
  else {
    mMatchAll = PR_FALSE;
  }
  mRootNode->AddMutationObserver(this);

  
  
  
  
  
  nsIDocument* doc = mRootNode->GetCurrentDoc();
  mFlushesNeeded = doc && !doc->IsHTML();
}

nsContentList::nsContentList(nsINode* aRootNode,
                             nsContentListMatchFunc aFunc,
                             nsContentListDestroyFunc aDestroyFunc,
                             void* aData,
                             PRBool aDeep,
                             nsIAtom* aMatchAtom,
                             PRInt32 aMatchNameSpaceId,
                             PRBool aFuncMayDependOnAttr)
  : nsBaseContentList(),
    mRootNode(aRootNode),
    mMatchNameSpaceId(aMatchNameSpaceId),
    mHTMLMatchAtom(aMatchAtom),
    mXMLMatchAtom(aMatchAtom),
    mFunc(aFunc),
    mDestroyFunc(aDestroyFunc),
    mData(aData),
    mState(LIST_DIRTY),
    mMatchAll(PR_FALSE),
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

DOMCI_DATA(ContentList, nsContentList)


NS_INTERFACE_TABLE_HEAD(nsContentList)
  NS_NODELIST_OFFSET_AND_INTERFACE_TABLE_BEGIN(nsContentList)
    NS_CONTENT_LIST_INTERFACES(nsContentList)
    NS_INTERFACE_TABLE_ENTRY(nsContentList, nsIHTMLCollection)
    NS_INTERFACE_TABLE_ENTRY(nsContentList, nsIDOMHTMLCollection)
    NS_INTERFACE_TABLE_ENTRY(nsContentList, nsIMutationObserver)
  NS_OFFSET_AND_INTERFACE_TABLE_END
  NS_OFFSET_AND_INTERFACE_TABLE_TO_MAP_SEGUE
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(ContentList)
NS_INTERFACE_MAP_END_INHERITING(nsBaseContentList)


NS_IMPL_ADDREF_INHERITED(nsContentList, nsBaseContentList)
NS_IMPL_RELEASE_INHERITED(nsContentList, nsBaseContentList)

PRUint32
nsContentList::Length(PRBool aDoFlush)
{
  BringSelfUpToDate(aDoFlush);
    
  return mElements.Count();
}

nsIContent *
nsContentList::Item(PRUint32 aIndex, PRBool aDoFlush)
{
  if (mRootNode && aDoFlush && mFlushesNeeded) {
    
    nsIDocument* doc = mRootNode->GetCurrentDoc();
    if (doc) {
      
      doc->FlushPendingNotifications(Flush_ContentAndNotify);
    }
  }

  if (mState != LIST_UP_TO_DATE)
    PopulateSelf(aIndex+1);

  ASSERT_IN_SYNC;
  NS_ASSERTION(!mRootNode || mState != LIST_DIRTY,
               "PopulateSelf left the list in a dirty (useless) state!");

  return mElements.SafeObjectAt(aIndex);
}

nsIContent *
nsContentList::NamedItem(const nsAString& aName, PRBool aDoFlush)
{
  BringSelfUpToDate(aDoFlush);
    
  PRInt32 i, count = mElements.Count();

  
  nsCOMPtr<nsIAtom> name = do_GetAtom(aName);
  NS_ENSURE_TRUE(name, nsnull);

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

  return nsnull;
}

PRInt32
nsContentList::IndexOf(nsIContent *aContent, PRBool aDoFlush)
{
  BringSelfUpToDate(aDoFlush);
    
  return mElements.IndexOf(aContent);
}

PRInt32
nsContentList::IndexOf(nsIContent* aContent)
{
  return IndexOf(aContent, PR_TRUE);
}

void
nsContentList::NodeWillBeDestroyed(const nsINode* aNode)
{
  

  RemoveFromCaches();
  mRootNode = nsnull;

  
  
  SetDirty();
}

NS_IMETHODIMP
nsContentList::GetLength(PRUint32* aLength)
{
  *aLength = Length(PR_TRUE);

  return NS_OK;
}

NS_IMETHODIMP
nsContentList::Item(PRUint32 aIndex, nsIDOMNode** aReturn)
{
  nsINode* node = GetNodeAt(aIndex);

  if (node) {
    return CallQueryInterface(node, aReturn);
  }

  *aReturn = nsnull;

  return NS_OK;
}

NS_IMETHODIMP
nsContentList::NamedItem(const nsAString& aName, nsIDOMNode** aReturn)
{
  nsIContent *content = NamedItem(aName, PR_TRUE);

  if (content) {
    return CallQueryInterface(content, aReturn);
  }

  *aReturn = nsnull;

  return NS_OK;
}

nsIContent*
nsContentList::GetNodeAt(PRUint32 aIndex)
{
  return Item(aIndex, PR_TRUE);
}

nsISupports*
nsContentList::GetNamedItem(const nsAString& aName, nsWrapperCache **aCache)
{
  nsIContent *item;
  *aCache = item = NamedItem(aName, PR_TRUE);
  return item;
}

void
nsContentList::AttributeChanged(nsIDocument *aDocument, Element* aElement,
                                PRInt32 aNameSpaceID, nsIAtom* aAttribute,
                                PRInt32 aModType)
{
  NS_PRECONDITION(aElement, "Must have a content node to work with");
  
  if (!mFunc || !mFuncMayDependOnAttr || mState == LIST_DIRTY ||
      !MayContainRelevantNodes(aElement->GetNodeParent()) ||
      !nsContentUtils::IsInSameAnonymousTree(mRootNode, aElement)) {
    
    
    return;
  }
  
  if (Match(aElement)) {
    if (mElements.IndexOf(aElement) == -1) {
      
      
      
      SetDirty();
    }
  } else {
    
    
    
    
    mElements.RemoveObject(aElement);
  }
}

void
nsContentList::ContentAppended(nsIDocument* aDocument, nsIContent* aContainer,
                               nsIContent* aFirstNewContent,
                               PRInt32 aNewIndexInContainer)
{
  NS_PRECONDITION(aContainer, "Can't get at the new content if no container!");
  
  





  if (mState == LIST_DIRTY ||
      !nsContentUtils::IsInSameAnonymousTree(mRootNode, aContainer) ||
      !MayContainRelevantNodes(aContainer))
    return;

  







  
  PRInt32 count = aContainer->GetChildCount();

  if (count > 0) {
    PRInt32 ourCount = mElements.Count();
    PRBool appendToList = PR_FALSE;
    if (ourCount == 0) {
      appendToList = PR_TRUE;
    } else {
      nsIContent* ourLastContent = mElements[ourCount - 1];
      



      if (nsContentUtils::PositionIsBefore(ourLastContent, aFirstNewContent)) {
        appendToList = PR_TRUE;
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
          mElements.AppendObject(cur);
        }
      }
    } else {
      for (nsIContent* cur = aFirstNewContent; cur; cur = cur->GetNextSibling()) {
        if (cur->IsElement() && Match(cur->AsElement())) {
          mElements.AppendObject(cur);
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
                               PRInt32 aIndexInContainer)
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
                              PRInt32 aIndexInContainer,
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

PRBool
nsContentList::Match(Element *aElement)
{
  if (mFunc) {
    return (*mFunc)(aElement, mMatchNameSpaceId, mXMLMatchAtom, mData);
  }

  if (!mXMLMatchAtom)
    return PR_FALSE;

  nsINodeInfo *ni = aElement->NodeInfo();
 
  PRBool unknown = mMatchNameSpaceId == kNameSpaceID_Unknown;
  PRBool wildcard = mMatchNameSpaceId == kNameSpaceID_Wildcard;
  PRBool toReturn = mMatchAll;
  if (!unknown && !wildcard)
    toReturn &= ni->NamespaceEquals(mMatchNameSpaceId);

  if (toReturn)
    return toReturn;

  nsIDocument* doc = aElement->GetOwnerDoc();
  PRBool matchHTML = aElement->GetNameSpaceID() == kNameSpaceID_XHTML &&
    doc && doc->IsHTML();
 
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

PRBool 
nsContentList::MatchSelf(nsIContent *aContent)
{
  NS_PRECONDITION(aContent, "Can't match null stuff, you know");
  NS_PRECONDITION(mDeep || aContent->GetNodeParent() == mRootNode,
                  "MatchSelf called on a node that we can't possibly match");

  if (!aContent->IsElement()) {
    return PR_FALSE;
  }
  
  if (Match(aContent->AsElement()))
    return PR_TRUE;

  if (!mDeep)
    return PR_FALSE;

  for (nsIContent* cur = aContent->GetFirstChild();
       cur;
       cur = cur->GetNextNode(aContent)) {
    if (cur->IsElement() && Match(cur->AsElement())) {
      return PR_TRUE;
    }
  }
  
  return PR_FALSE;
}

void 
nsContentList::PopulateSelf(PRUint32 aNeededLength)
{
  if (!mRootNode) {
    return;
  }

  ASSERT_IN_SYNC;

  PRUint32 count = mElements.Count();
  NS_ASSERTION(mState != LIST_DIRTY || count == 0,
               "Reset() not called when setting state to LIST_DIRTY?");

  if (count >= aNeededLength) 
    return;

  PRUint32 elementsToAppend = aNeededLength - count;
#ifdef DEBUG
  PRUint32 invariant = elementsToAppend + mElements.Count();
#endif

  if (mDeep) {
    
    
    nsINode* cur = count ? mElements[count - 1] : mRootNode;
    do {
      cur = cur->GetNextNode(mRootNode);
      if (!cur) {
        break;
      }
      if (cur->IsElement() && Match(cur->AsElement())) {
        mElements.AppendObject(cur->AsElement());
        --elementsToAppend;
      }
    } while (elementsToAppend);
  } else {
    nsIContent* cur =
      count ? mElements[count-1]->GetNextSibling() : mRootNode->GetFirstChild();
    for ( ; cur && elementsToAppend; cur = cur->GetNextSibling()) {
      if (cur->IsElement() && Match(cur->AsElement())) {
        mElements.AppendObject(cur);
        --elementsToAppend;
      }
    }
  }

  NS_ASSERTION(elementsToAppend + mElements.Count() == invariant,
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
    gContentListHashTable.ops = nsnull;
  }
}

void
nsContentList::BringSelfUpToDate(PRBool aDoFlush)
{
  if (mRootNode && aDoFlush && mFlushesNeeded) {
    
    nsIDocument* doc = mRootNode->GetCurrentDoc();
    if (doc) {
      
      doc->FlushPendingNotifications(Flush_ContentAndNotify);
    }
  }

  if (mState != LIST_UP_TO_DATE)
    PopulateSelf(PRUint32(-1));
    
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
    gFuncStringContentListHashTable.ops = nsnull;
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
    NS_ASSERTION(mElements.Count() == 0 && mState == LIST_DIRTY,
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
    NS_NewPreContentIterator(getter_AddRefs(iter));
    iter->Init(root);
    iter->First();
  }

  PRInt32 cnt = 0, index = 0;
  while (PR_TRUE) {
    if (cnt == mElements.Count() && mState == LIST_LAZY) {
      break;
    }

    nsIContent *cur = mDeep ? iter->GetCurrentNode() :
                              mRootNode->GetChildAt(index++);
    if (!cur) {
      break;
    }

    if (cur->IsElement() && Match(cur->AsElement())) {
      NS_ASSERTION(cnt < mElements.Count() && mElements[cnt] == cur,
                   "Elements is out of sync");
      ++cnt;
    }

    if (mDeep) {
      iter->Next();
    }
  }

  NS_ASSERTION(cnt == mElements.Count(), "Too few elements");
}
#endif
