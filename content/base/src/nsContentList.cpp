











































#include "nsContentList.h"
#include "nsIContent.h"
#include "nsIDOMNode.h"
#include "nsIDOM3Node.h"
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


static nsContentList *gCachedContentList;

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



NS_INTERFACE_TABLE_HEAD(nsBaseContentList)
  NS_NODELIST_OFFSET_AND_INTERFACE_TABLE_BEGIN(nsBaseContentList)
    NS_CONTENT_LIST_INTERFACES(nsBaseContentList)
  NS_OFFSET_AND_INTERFACE_TABLE_END
  NS_OFFSET_AND_INTERFACE_TABLE_TO_MAP_SEGUE
  NS_INTERFACE_MAP_ENTRIES_CYCLE_COLLECTION(nsBaseContentList)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(NodeList)
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


void nsBaseContentList::Shutdown() {
  NS_IF_RELEASE(gCachedContentList);
}



nsFormContentList::nsFormContentList(nsIDOMHTMLFormElement *aForm,
                                     nsBaseContentList& aContentList)
  : nsBaseContentList()
{

  

  PRUint32 i, length = 0;
  nsCOMPtr<nsIDOMNode> item;

  aContentList.GetLength(&length);

  for (i = 0; i < length; i++) {
    aContentList.Item(i, getter_AddRefs(item));

    nsCOMPtr<nsIContent> c(do_QueryInterface(item));

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
  const nsContentListKey* list1 = e->mContentList->GetKey();
  const nsContentListKey* list2 = static_cast<const nsContentListKey *>(key);

  return list1->Equals(*list2);
}

already_AddRefed<nsContentList>
NS_GetContentList(nsINode* aRootNode, nsIAtom* aMatchAtom,
                  PRInt32 aMatchNameSpaceId)
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
    nsContentListKey hashKey(aRootNode, aMatchAtom,
                             aMatchNameSpaceId);
    
    
    
    entry = static_cast<ContentListHashEntry *>
                       (PL_DHashTableOperate(&gContentListHashTable,
                                                &hashKey,
                                                PL_DHASH_ADD));
    if (entry)
      list = entry->mContentList;
  }

  if (!list) {
    
    
    list = new nsContentList(aRootNode, aMatchAtom,
                             aMatchNameSpaceId);
    if (entry) {
      if (list)
        entry->mContentList = list;
      else
        PL_DHashTableRawRemove(&gContentListHashTable, entry);
    }

    NS_ENSURE_TRUE(list, nsnull);
  }

  NS_ADDREF(list);

  
  
  
  

  if (gCachedContentList != list) {
    NS_IF_RELEASE(gCachedContentList);

    gCachedContentList = list;
    NS_ADDREF(gCachedContentList);
  }

  return list;
}




nsContentList::nsContentList(nsINode* aRootNode,
                             nsIAtom* aMatchAtom,
                             PRInt32 aMatchNameSpaceId,
                             PRBool aDeep)
  : nsBaseContentList(),
    nsContentListKey(aRootNode, aMatchAtom, aMatchNameSpaceId),
    mFunc(nsnull),
    mDestroyFunc(nsnull),
    mData(nsnull),
    mState(LIST_DIRTY),
    mDeep(aDeep),
    mFuncMayDependOnAttr(PR_FALSE)
{
  NS_ASSERTION(mRootNode, "Must have root");
  if (nsGkAtoms::_asterix == mMatchAtom) {
    mMatchAll = PR_TRUE;
  }
  else {
    mMatchAll = PR_FALSE;
  }
  mRootNode->AddMutationObserver(this);
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
    nsContentListKey(aRootNode, aMatchAtom, aMatchNameSpaceId),
    mFunc(aFunc),
    mDestroyFunc(aDestroyFunc),
    mData(aData),
    mMatchAll(PR_FALSE),
    mState(LIST_DIRTY),
    mDeep(aDeep),
    mFuncMayDependOnAttr(aFuncMayDependOnAttr)
{
  NS_ASSERTION(mRootNode, "Must have root");
  mRootNode->AddMutationObserver(this);
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



NS_INTERFACE_TABLE_HEAD(nsContentList)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_NODELIST_OFFSET_AND_INTERFACE_TABLE_BEGIN(nsContentList)
    NS_CONTENT_LIST_INTERFACES(nsContentList)
    NS_INTERFACE_TABLE_ENTRY(nsContentList, nsIHTMLCollection)
    NS_INTERFACE_TABLE_ENTRY(nsContentList, nsIDOMHTMLCollection)
    NS_INTERFACE_TABLE_ENTRY(nsContentList, nsIMutationObserver)
  NS_OFFSET_AND_INTERFACE_TABLE_END
  NS_OFFSET_AND_INTERFACE_TABLE_TO_MAP_SEGUE
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(ContentList)
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
  if (mRootNode && aDoFlush) {
    
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
  

  RemoveFromHashtable();
  mRootNode = nsnull;

  
  
  SetDirty();
}


void
nsContentList::OnDocumentDestroy(nsIDocument *aDocument)
{
  
  
  
  

  if (gCachedContentList && gCachedContentList->mRootNode &&
      gCachedContentList->mRootNode->GetOwnerDoc() == aDocument) {
    NS_RELEASE(gCachedContentList);
  }
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
nsContentList::GetNodeAt(PRUint32 aIndex, nsresult* aResult)
{
  *aResult = NS_OK;
  return Item(aIndex, PR_TRUE);
}

nsISupports*
nsContentList::GetNamedItem(const nsAString& aName, nsresult* aResult)
{
  *aResult = NS_OK;
  return NamedItem(aName, PR_TRUE);
}

void
nsContentList::AttributeChanged(nsIDocument *aDocument, nsIContent* aContent,
                                PRInt32 aNameSpaceID, nsIAtom* aAttribute,
                                PRInt32 aModType)
{
  NS_PRECONDITION(aContent, "Must have a content node to work with");
  NS_PRECONDITION(aContent->IsNodeOfType(nsINode::eELEMENT),
                  "Should be an element");
  
  if (!mFunc || !mFuncMayDependOnAttr || mState == LIST_DIRTY ||
      !MayContainRelevantNodes(aContent->GetNodeParent()) ||
      !nsContentUtils::IsInSameAnonymousTree(mRootNode, aContent)) {
    
    
    return;
  }
  
  if (Match(aContent)) {
    if (mElements.IndexOf(aContent) == -1) {
      
      
      
      SetDirty();
    }
  } else {
    
    
    
    
    mElements.RemoveObject(aContent);
  }
}

void
nsContentList::ContentAppended(nsIDocument *aDocument, nsIContent* aContainer,
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
      



      if (nsContentUtils::PositionIsBefore(ourLastContent,
                                           aContainer->GetChildAt(aNewIndexInContainer))) {
        appendToList = PR_TRUE;
      }
    }
    

    if (!appendToList) {
      
      
      for (nsINode::ChildIterator iter(aContainer, aNewIndexInContainer);
           !iter.IsDone();
           iter.Next()) {
        if (MatchSelf(iter)) {
          
          
          SetDirty();
          break;
        }
      }

      ASSERT_IN_SYNC;
      return;
    }

    





    if (mState == LIST_LAZY) 
      return;

    



    for (nsINode::ChildIterator iter(aContainer, aNewIndexInContainer);
         !iter.IsDone();
         iter.Next()) {
      PRUint32 limit = PRUint32(-1);
      nsIContent* newContent = iter;
      if (newContent->IsNodeOfType(nsINode::eELEMENT)) {
        PopulateWith(newContent, limit);
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

PRBool
nsContentList::Match(nsIContent *aContent)
{
  if (!aContent)
    return PR_FALSE;

  NS_ASSERTION(aContent->IsNodeOfType(nsINode::eELEMENT),
               "Must have element here");

  if (mFunc) {
    return (*mFunc)(aContent, mMatchNameSpaceId, mMatchAtom, mData);
  }

  if (mMatchAtom) {
    nsINodeInfo *ni = aContent->NodeInfo();

    if (mMatchNameSpaceId == kNameSpaceID_Unknown) {
      return (mMatchAll || ni->QualifiedNameEquals(mMatchAtom));
    }

    if (mMatchNameSpaceId == kNameSpaceID_Wildcard) {
      return (mMatchAll || ni->Equals(mMatchAtom));
    }

    return ((mMatchAll && ni->NamespaceEquals(mMatchNameSpaceId)) ||
            ni->Equals(mMatchAtom, mMatchNameSpaceId));
  }

  return PR_FALSE;
}

PRBool 
nsContentList::MatchSelf(nsIContent *aContent)
{
  NS_PRECONDITION(aContent, "Can't match null stuff, you know");
  NS_PRECONDITION(mDeep || aContent->GetNodeParent() == mRootNode,
                  "MatchSelf called on a node that we can't possibly match");

  if (!aContent->IsNodeOfType(nsINode::eELEMENT)) {
    return PR_FALSE;
  }
  
  if (Match(aContent))
    return PR_TRUE;

  if (!mDeep)
    return PR_FALSE;

  for (nsINode::ChildIterator iter(aContent); !iter.IsDone(); iter.Next()) {
    if (MatchSelf(iter)) {
      return PR_TRUE;
    }
  }
  
  return PR_FALSE;
}

void
nsContentList::PopulateWith(nsIContent *aContent, PRUint32& aElementsToAppend)
{
  NS_PRECONDITION(mDeep || aContent->GetNodeParent() == mRootNode,
                  "PopulateWith called on nodes we can't possibly match");
  NS_PRECONDITION(aContent != mRootNode,
                  "We should never be trying to match mRootNode");
  NS_PRECONDITION(aContent->IsNodeOfType(nsINode::eELEMENT),
                  "Should be an element");

  if (Match(aContent)) {
    mElements.AppendObject(aContent);
    --aElementsToAppend;
    if (aElementsToAppend == 0)
      return;
  }

  
  if (!mDeep)
    return;

  for (nsINode::ChildIterator iter(aContent); !iter.IsDone(); iter.Next()) {
    nsIContent* curContent = iter;
    if (curContent->IsNodeOfType(nsINode::eELEMENT)) {
      PopulateWith(curContent, aElementsToAppend);
      if (aElementsToAppend == 0)
        break;
    }
  }
}

void 
nsContentList::PopulateWithStartingAfter(nsINode *aStartRoot,
                                         nsINode *aStartChild,
                                         PRUint32 & aElementsToAppend)
{
  NS_PRECONDITION(mDeep || aStartRoot == mRootNode ||
                  (aStartRoot->GetNodeParent() == mRootNode &&
                   aStartChild == nsnull),
                  "Bogus aStartRoot or aStartChild");

  if (mDeep || aStartRoot == mRootNode) {
#ifdef DEBUG
    PRUint32 invariant = aElementsToAppend + mElements.Count();
#endif
    PRInt32 i = 0;
    if (aStartChild) {
      i = aStartRoot->IndexOf(aStartChild);
      NS_ASSERTION(i >= 0, "The start child must be a child of the start root!");
      ++i;  
    }

    
    for (nsINode::ChildIterator iter(aStartRoot, i);
         !iter.IsDone();
         iter.Next()) {
      nsIContent* content = iter;
      if (content->IsNodeOfType(nsINode::eELEMENT)) {
        PopulateWith(content, aElementsToAppend);

        NS_ASSERTION(aElementsToAppend + mElements.Count() == invariant,
                     "Something is awry in PopulateWith!");
        if (aElementsToAppend == 0)
          break;
      }
    }
  }

  if (aElementsToAppend == 0) {
    return;
  }

  
  
  if (aStartRoot == mRootNode)
    return;
  
  
  
  
  
  nsINode* parent = aStartRoot->GetNodeParent();
  
  if (parent)
    PopulateWithStartingAfter(parent, aStartRoot, aElementsToAppend);
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

  
  
  nsINode* startRoot = count == 0 ? mRootNode : mElements[count - 1];

  PopulateWithStartingAfter(startRoot, nsnull, elementsToAppend);
  NS_ASSERTION(elementsToAppend + mElements.Count() == invariant,
               "Something is awry in PopulateWith!");

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

  PL_DHashTableOperate(&gContentListHashTable,
                       GetKey(),
                       PL_DHASH_REMOVE);

  if (gContentListHashTable.entryCount == 0) {
    PL_DHashTableFinish(&gContentListHashTable);
    gContentListHashTable.ops = nsnull;
  }
}

void
nsContentList::BringSelfUpToDate(PRBool aDoFlush)
{
  if (mRootNode && aDoFlush) {
    
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
    root = static_cast<nsIDocument*>(mRootNode)->GetRootContent();
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

    if (cur->IsNodeOfType(nsINode::eELEMENT) && Match(cur)) {
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
