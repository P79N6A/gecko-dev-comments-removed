





#include "nsCOMPtr.h"
#include "nsXBLService.h"
#include "nsIInputStream.h"
#include "nsIURI.h"
#include "nsIURL.h"
#include "nsIChannel.h"
#include "nsXPIDLString.h"
#include "nsNetUtil.h"
#include "plstr.h"
#include "nsIContent.h"
#include "nsIDOMElement.h"
#include "nsIDocument.h"
#include "nsContentUtils.h"
#include "nsIPresShell.h"
#include "nsIXMLContentSink.h"
#include "nsContentCID.h"
#include "nsXMLDocument.h"
#include "nsIStreamListener.h"

#include "nsXBLBinding.h"
#include "nsXBLPrototypeBinding.h"
#include "nsXBLDocumentInfo.h"
#include "nsXBLInsertionPoint.h"

#include "nsIStyleRuleProcessor.h"
#include "nsRuleProcessorData.h"
#include "nsIWeakReference.h"

#include "jsapi.h"
#include "nsIXPConnect.h"
#include "nsDOMCID.h"
#include "nsIDOMScriptObjectFactory.h"
#include "nsIScriptGlobalObject.h"
#include "nsTHashtable.h"

#include "nsIScriptContext.h"
#include "nsBindingManager.h"

#include "nsThreadUtils.h"
#include "mozilla/dom/NodeListBinding.h"





#define NS_ANONYMOUS_CONTENT_LIST_IID \
  { 0xbfb5d8e7, 0xf718, 0x4a46, \
    { 0xb2, 0x2b, 0x22, 0x4a, 0x44, 0x4c, 0xb9, 0x77 } }

class nsAnonymousContentList : public nsINodeList
{
public:
  nsAnonymousContentList(nsIContent *aContent, nsInsertionPointList* aElements);
  virtual ~nsAnonymousContentList();

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(nsAnonymousContentList)
  
  NS_DECL_NSIDOMNODELIST

  
  virtual int32_t IndexOf(nsIContent* aContent);
  virtual nsINode *GetParentObject()
  {
    return mContent;
  }
  virtual nsIContent* Item(uint32_t aIndex);

  int32_t GetInsertionPointCount() { return mElements->Length(); }

  nsXBLInsertionPoint* GetInsertionPointAt(int32_t i) { return static_cast<nsXBLInsertionPoint*>(mElements->ElementAt(i)); }
  void RemoveInsertionPointAt(int32_t i) { mElements->RemoveElementAt(i); }

  virtual JSObject* WrapObject(JSContext *cx, JSObject *scope,
                               bool *triedToWrap)
  {
    return mozilla::dom::NodeListBinding::Wrap(cx, scope, this, triedToWrap);
  }

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ANONYMOUS_CONTENT_LIST_IID)
private:
  nsCOMPtr<nsIContent> mContent;
  nsInsertionPointList* mElements;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsAnonymousContentList,
                              NS_ANONYMOUS_CONTENT_LIST_IID)

nsAnonymousContentList::nsAnonymousContentList(nsIContent *aContent,
                                               nsInsertionPointList* aElements)
  : mContent(aContent),
    mElements(aElements)
{
  MOZ_COUNT_CTOR(nsAnonymousContentList);

  
  
  SetIsDOMBinding();
}

nsAnonymousContentList::~nsAnonymousContentList()
{
  MOZ_COUNT_DTOR(nsAnonymousContentList);
  delete mElements;
}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsAnonymousContentList)

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsAnonymousContentList)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsAnonymousContentList)

NS_INTERFACE_TABLE_HEAD(nsAnonymousContentList)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_NODELIST_OFFSET_AND_INTERFACE_TABLE_BEGIN(nsAnonymousContentList)
    NS_INTERFACE_TABLE_ENTRY(nsAnonymousContentList, nsINodeList)
    NS_INTERFACE_TABLE_ENTRY(nsAnonymousContentList, nsIDOMNodeList)
    NS_INTERFACE_TABLE_ENTRY(nsAnonymousContentList, nsAnonymousContentList)
  NS_OFFSET_AND_INTERFACE_TABLE_END
  NS_OFFSET_AND_INTERFACE_TABLE_TO_MAP_SEGUE
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(NodeList)
  NS_INTERFACE_MAP_ENTRIES_CYCLE_COLLECTION(nsAnonymousContentList)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsAnonymousContentList)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mContent)
  tmp->mElements->Clear();
  NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsAnonymousContentList)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mContent)
  {
    int32_t i, count = tmp->mElements->Length();
    for (i = 0; i < count; ++i) {
      NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mElements->ElementAt(i));
    }
  }
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END
NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(nsAnonymousContentList)
  NS_IMPL_CYCLE_COLLECTION_TRACE_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_IMETHODIMP
nsAnonymousContentList::GetLength(uint32_t* aLength)
{
  NS_ASSERTION(aLength != nullptr, "null ptr");
  if (! aLength)
      return NS_ERROR_NULL_POINTER;

  int32_t cnt = mElements->Length();

  *aLength = 0;
  for (int32_t i = 0; i < cnt; i++)
    *aLength += static_cast<nsXBLInsertionPoint*>(mElements->ElementAt(i))->ChildCount();

  return NS_OK;
}

NS_IMETHODIMP    
nsAnonymousContentList::Item(uint32_t aIndex, nsIDOMNode** aReturn)
{
  nsINode* item = Item(aIndex);
  if (!item)
    return NS_ERROR_FAILURE;

  return CallQueryInterface(item, aReturn);    
}

nsIContent*
nsAnonymousContentList::Item(uint32_t aIndex)
{
  int32_t cnt = mElements->Length();
  uint32_t pointCount = 0;

  for (int32_t i = 0; i < cnt; i++) {
    aIndex -= pointCount;
    
    nsXBLInsertionPoint* point = static_cast<nsXBLInsertionPoint*>(mElements->ElementAt(i));
    pointCount = point->ChildCount();

    if (aIndex < pointCount) {
      return point->ChildAt(aIndex);
    }
  }

  return nullptr;
}

int32_t
nsAnonymousContentList::IndexOf(nsIContent* aContent)
{
  int32_t cnt = mElements->Length();
  int32_t lengthSoFar = 0;

  for (int32_t i = 0; i < cnt; ++i) {
    nsXBLInsertionPoint* point =
      static_cast<nsXBLInsertionPoint*>(mElements->ElementAt(i));
    int32_t idx = point->IndexOf(aContent);
    if (idx != -1) {
      return idx + lengthSoFar;
    }

    lengthSoFar += point->ChildCount();
  }

  
  return -1;
}






class ObjectEntry : public PLDHashEntryHdr
{
public:

  
  
  ObjectEntry() { MOZ_COUNT_CTOR(ObjectEntry); }
  ~ObjectEntry() { MOZ_COUNT_DTOR(ObjectEntry); }
  
  nsISupports* GetValue() { return mValue; }
  nsISupports* GetKey() { return mKey; }
  void SetValue(nsISupports* aValue) { mValue = aValue; }
  void SetKey(nsISupports* aKey) { mKey = aKey; }
  
private:
  nsCOMPtr<nsISupports> mKey;
  nsCOMPtr<nsISupports> mValue;
};

static void
ClearObjectEntry(PLDHashTable* table, PLDHashEntryHdr *entry)
{
  ObjectEntry* objEntry = static_cast<ObjectEntry*>(entry);
  objEntry->~ObjectEntry();
}

static bool
InitObjectEntry(PLDHashTable* table, PLDHashEntryHdr* entry, const void* key)
{
  new (entry) ObjectEntry;
  return true;
}
  


static PLDHashTableOps ObjectTableOps = {
  PL_DHashAllocTable,
  PL_DHashFreeTable,
  PL_DHashVoidPtrKeyStub,
  PL_DHashMatchEntryStub,
  PL_DHashMoveEntryStub,
  ClearObjectEntry,
  PL_DHashFinalizeStub,
  InitObjectEntry
};


static nsresult
AddObjectEntry(PLDHashTable& table, nsISupports* aKey, nsISupports* aValue)
{
  NS_ASSERTION(aKey, "key must be non-null");
  if (!aKey) return NS_ERROR_INVALID_ARG;
  
  ObjectEntry *entry =
    static_cast<ObjectEntry*>
               (PL_DHashTableOperate(&table, aKey, PL_DHASH_ADD));

  if (!entry)
    return NS_ERROR_OUT_OF_MEMORY;

  
  if (!entry->GetKey())
    entry->SetKey(aKey);

  
  
  entry->SetValue(aValue);
  
  return NS_OK;
}



static nsISupports*
LookupObject(PLDHashTable& table, nsIContent* aKey)
{
  if (aKey && aKey->HasFlag(NODE_MAY_BE_IN_BINDING_MNGR)) {
    ObjectEntry *entry =
      static_cast<ObjectEntry*>
                 (PL_DHashTableOperate(&table, aKey, PL_DHASH_LOOKUP));

    if (PL_DHASH_ENTRY_IS_BUSY(entry))
      return entry->GetValue();
  }

  return nullptr;
}

inline void
RemoveObjectEntry(PLDHashTable& table, nsISupports* aKey)
{
  PL_DHashTableOperate(&table, aKey, PL_DHASH_REMOVE);
}

static nsresult
SetOrRemoveObject(PLDHashTable& table, nsIContent* aKey, nsISupports* aValue)
{
  if (aValue) {
    
    if (!table.ops &&
        !PL_DHashTableInit(&table, &ObjectTableOps, nullptr,
                           sizeof(ObjectEntry), 16)) {
      table.ops = nullptr;
      return NS_ERROR_OUT_OF_MEMORY;
    }
    aKey->SetFlags(NODE_MAY_BE_IN_BINDING_MNGR);
    return AddObjectEntry(table, aKey, aValue);
  }

  
  if (table.ops) {
    ObjectEntry* entry =
      static_cast<ObjectEntry*>
        (PL_DHashTableOperate(&table, aKey, PL_DHASH_LOOKUP));
    if (entry && PL_DHASH_ENTRY_IS_BUSY(entry)) {
      
      nsCOMPtr<nsISupports> key = entry->GetKey();
      nsCOMPtr<nsISupports> value = entry->GetValue();
      RemoveObjectEntry(table, aKey);
    }
  }
  return NS_OK;
}







NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsBindingManager)
  tmp->mDestroyed = true;

  if (tmp->mBindingTable.IsInitialized())
    tmp->mBindingTable.Clear();

  if (tmp->mDocumentTable.IsInitialized())
    tmp->mDocumentTable.Clear();

  if (tmp->mLoadingDocTable.IsInitialized())
    tmp->mLoadingDocTable.Clear();

  if (tmp->mContentListTable.ops)
    PL_DHashTableFinish(&(tmp->mContentListTable));
  tmp->mContentListTable.ops = nullptr;

  if (tmp->mAnonymousNodesTable.ops)
    PL_DHashTableFinish(&(tmp->mAnonymousNodesTable));
  tmp->mAnonymousNodesTable.ops = nullptr;

  if (tmp->mInsertionParentTable.ops)
    PL_DHashTableFinish(&(tmp->mInsertionParentTable));
  tmp->mInsertionParentTable.ops = nullptr;

  if (tmp->mWrapperTable.ops)
    PL_DHashTableFinish(&(tmp->mWrapperTable));
  tmp->mWrapperTable.ops = nullptr;

  NS_IMPL_CYCLE_COLLECTION_UNLINK(mAttachedStack)

  if (tmp->mProcessAttachedQueueEvent) {
    tmp->mProcessAttachedQueueEvent->Revoke();
  }
NS_IMPL_CYCLE_COLLECTION_UNLINK_END


static PLDHashOperator
DocumentInfoHashtableTraverser(nsIURI* key,
                               nsXBLDocumentInfo* di,
                               void* userArg)
{
  nsCycleCollectionTraversalCallback *cb = 
    static_cast<nsCycleCollectionTraversalCallback*>(userArg);
  NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(*cb, "mDocumentTable value");
  cb->NoteXPCOMChild(static_cast<nsIScriptGlobalObjectOwner*>(di));
  return PL_DHASH_NEXT;
}

static PLDHashOperator
LoadingDocHashtableTraverser(nsIURI* key,
                             nsIStreamListener* sl,
                             void* userArg)
{
  nsCycleCollectionTraversalCallback *cb = 
    static_cast<nsCycleCollectionTraversalCallback*>(userArg);
  NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(*cb, "mLoadingDocTable value");
  cb->NoteXPCOMChild(sl);
  return PL_DHASH_NEXT;
}

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsBindingManager)
  
  if (tmp->mDocumentTable.IsInitialized())
      tmp->mDocumentTable.EnumerateRead(&DocumentInfoHashtableTraverser, &cb);
  if (tmp->mLoadingDocTable.IsInitialized())
      tmp->mLoadingDocTable.EnumerateRead(&LoadingDocHashtableTraverser, &cb);
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mAttachedStack)
  
  
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_CLASS(nsBindingManager)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsBindingManager)
  NS_INTERFACE_MAP_ENTRY(nsIMutationObserver)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsBindingManager)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsBindingManager)


nsBindingManager::nsBindingManager(nsIDocument* aDocument)
  : mProcessingAttachedStack(false),
    mDestroyed(false),
    mAttachedStackSizeOnOutermost(0),
    mDocument(aDocument)
{
  mContentListTable.ops = nullptr;
  mAnonymousNodesTable.ops = nullptr;
  mInsertionParentTable.ops = nullptr;
  mWrapperTable.ops = nullptr;
}

nsBindingManager::~nsBindingManager(void)
{
  mDestroyed = true;

  if (mContentListTable.ops)
    PL_DHashTableFinish(&mContentListTable);
  if (mAnonymousNodesTable.ops)
    PL_DHashTableFinish(&mAnonymousNodesTable);
  NS_ASSERTION(!mInsertionParentTable.ops || !mInsertionParentTable.entryCount,
               "Insertion parent table isn't empty!");
  if (mInsertionParentTable.ops)
    PL_DHashTableFinish(&mInsertionParentTable);
  if (mWrapperTable.ops)
    PL_DHashTableFinish(&mWrapperTable);
}

PLDHashOperator
RemoveInsertionParentCB(PLDHashTable* aTable, PLDHashEntryHdr* aEntry,
                        uint32_t aNumber, void* aArg)
{
  return (static_cast<ObjectEntry*>(aEntry)->GetValue() ==
          static_cast<nsISupports*>(aArg)) ? PL_DHASH_REMOVE : PL_DHASH_NEXT;
}

static void
RemoveInsertionParentForNodeList(nsIDOMNodeList* aList, nsIContent* aParent)
{
  nsAnonymousContentList* list = nullptr;
  if (aList) {
    CallQueryInterface(aList, &list);
  }
  if (list) {
    int32_t count = list->GetInsertionPointCount();
    for (int32_t i = 0; i < count; ++i) {
      nsRefPtr<nsXBLInsertionPoint> currPoint = list->GetInsertionPointAt(i);
      currPoint->UnbindDefaultContent();
#ifdef DEBUG
      nsCOMPtr<nsIContent> parent = currPoint->GetInsertionParent();
      NS_ASSERTION(!parent || parent == aParent, "Wrong insertion parent!");
#endif
      currPoint->ClearInsertionParent();
    }
    NS_RELEASE(list);
  }
}

void
nsBindingManager::RemoveInsertionParent(nsIContent* aParent)
{
  RemoveInsertionParentForNodeList(GetContentListFor(aParent), aParent);

  RemoveInsertionParentForNodeList(GetAnonymousNodesFor(aParent), aParent);

  if (mInsertionParentTable.ops) {
    PL_DHashTableEnumerate(&mInsertionParentTable, RemoveInsertionParentCB,
                           static_cast<nsISupports*>(aParent));
  }
}

nsXBLBinding*
nsBindingManager::GetBinding(nsIContent* aContent)
{
  if (aContent && aContent->HasFlag(NODE_MAY_BE_IN_BINDING_MNGR) &&
      mBindingTable.IsInitialized()) {
    return mBindingTable.GetWeak(aContent);
  }

  return nullptr;
}

nsresult
nsBindingManager::SetBinding(nsIContent* aContent, nsXBLBinding* aBinding)
{
  if (!mBindingTable.IsInitialized()) {
    mBindingTable.Init();
  }

  
  
  
  
  
  
  nsRefPtr<nsXBLBinding> oldBinding = GetBinding(aContent);
  if (oldBinding) {
    if (aContent->HasFlag(NODE_IS_INSERTION_PARENT)) {
      nsRefPtr<nsXBLBinding> parentBinding =
        GetBinding(aContent->GetBindingParent());
      
      
      if (!parentBinding || !parentBinding->HasInsertionParent(aContent)) {
        RemoveInsertionParent(aContent);
        aContent->UnsetFlags(NODE_IS_INSERTION_PARENT);
      }
    }
    
    
    uint32_t index = mAttachedStack.IndexOf(oldBinding);
    if (index != mAttachedStack.NoIndex) {
      mAttachedStack[index] = nullptr;
    }
  }
  
  if (aBinding) {
    aContent->SetFlags(NODE_MAY_BE_IN_BINDING_MNGR);
    mBindingTable.Put(aContent, aBinding);
  } else {
    mBindingTable.Remove(aContent);

    
    
    
    SetWrappedJS(aContent, nullptr);
    SetContentListFor(aContent, nullptr);
    SetAnonymousNodesFor(aContent, nullptr);
    if (oldBinding) {
      oldBinding->SetBoundElement(nullptr);
    }
  }

  return NS_OK;
}

nsIContent*
nsBindingManager::GetInsertionParent(nsIContent* aContent)
{ 
  if (mInsertionParentTable.ops) {
    return static_cast<nsIContent*>
                      (LookupObject(mInsertionParentTable, aContent));
  }

  return nullptr;
}

nsresult
nsBindingManager::SetInsertionParent(nsIContent* aContent, nsIContent* aParent)
{
  NS_ASSERTION(!aParent || aParent->HasFlag(NODE_IS_INSERTION_PARENT),
               "Insertion parent should have NODE_IS_INSERTION_PARENT flag!");

  if (mDestroyed) {
    return NS_OK;
  }

  return SetOrRemoveObject(mInsertionParentTable, aContent, aParent);
}

nsIXPConnectWrappedJS*
nsBindingManager::GetWrappedJS(nsIContent* aContent)
{ 
  if (mWrapperTable.ops) {
    return static_cast<nsIXPConnectWrappedJS*>(LookupObject(mWrapperTable, aContent));
  }

  return nullptr;
}

nsresult
nsBindingManager::SetWrappedJS(nsIContent* aContent, nsIXPConnectWrappedJS* aWrappedJS)
{
  if (mDestroyed) {
    return NS_OK;
  }

  return SetOrRemoveObject(mWrapperTable, aContent, aWrappedJS);
}

void
nsBindingManager::RemovedFromDocumentInternal(nsIContent* aContent,
                                              nsIDocument* aOldDocument,
                                              nsIContent* aContentBindingParent)
{
  NS_PRECONDITION(aOldDocument != nullptr, "no old document");

  if (mDestroyed)
    return;

  
  
  nsRefPtr<nsXBLBinding> binding = GetBinding(aContent);
  if (aContent->HasFlag(NODE_IS_INSERTION_PARENT)) {
    nsRefPtr<nsXBLBinding> parentBinding = GetBinding(aContentBindingParent);
    if (parentBinding) {
      parentBinding->RemoveInsertionParent(aContent);
      
      
      if (!binding || !binding->HasInsertionParent(aContent)) {
        RemoveInsertionParent(aContent);
        aContent->UnsetFlags(NODE_IS_INSERTION_PARENT);
      }
    }
  }

  if (binding) {
    binding->PrototypeBinding()->BindingDetached(binding->GetBoundElement());
    binding->ChangeDocument(aOldDocument, nullptr);
    SetBinding(aContent, nullptr);
  }

  
  SetInsertionParent(aContent, nullptr);
  SetContentListFor(aContent, nullptr);
  SetAnonymousNodesFor(aContent, nullptr);
}

nsIAtom*
nsBindingManager::ResolveTag(nsIContent* aContent, int32_t* aNameSpaceID)
{
  nsXBLBinding *binding = GetBinding(aContent);
  
  if (binding) {
    nsIAtom* base = binding->GetBaseTag(aNameSpaceID);

    if (base) {
      return base;
    }
  }

  *aNameSpaceID = aContent->GetNameSpaceID();
  return aContent->Tag();
}

nsresult
nsBindingManager::GetContentListFor(nsIContent* aContent, nsIDOMNodeList** aResult)
{
  NS_IF_ADDREF(*aResult = GetContentListFor(aContent));
  return NS_OK;
}

nsINodeList*
nsBindingManager::GetContentListFor(nsIContent* aContent)
{ 
  nsINodeList* result = nullptr;

  if (mContentListTable.ops) {
    result = static_cast<nsAnonymousContentList*>
      (LookupObject(mContentListTable, aContent));
  }

  if (!result) {
    result = aContent->ChildNodes();
  }

  return result;
}

nsresult
nsBindingManager::SetContentListFor(nsIContent* aContent,
                                    nsInsertionPointList* aList)
{
  if (mDestroyed) {
    return NS_OK;
  }

  nsAnonymousContentList* contentList = nullptr;
  if (aList) {
    contentList = new nsAnonymousContentList(aContent, aList);
    if (!contentList) {
      delete aList;
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  return SetOrRemoveObject(mContentListTable, aContent, contentList);
}

bool
nsBindingManager::HasContentListFor(nsIContent* aContent)
{
  return mContentListTable.ops && LookupObject(mContentListTable, aContent);
}

nsINodeList*
nsBindingManager::GetAnonymousNodesInternal(nsIContent* aContent,
                                            bool* aIsAnonymousContentList)
{ 
  nsINodeList* result = nullptr;
  if (mAnonymousNodesTable.ops) {
    result = static_cast<nsAnonymousContentList*>
                        (LookupObject(mAnonymousNodesTable, aContent));
  }

  if (!result) {
    *aIsAnonymousContentList = false;
    nsXBLBinding *binding = GetBinding(aContent);
    if (binding) {
      result = binding->GetAnonymousNodes();
    }
  } else
    *aIsAnonymousContentList = true;

  return result;
}

nsresult
nsBindingManager::GetAnonymousNodesFor(nsIContent* aContent,
                                       nsIDOMNodeList** aResult)
{
  bool dummy;
  NS_IF_ADDREF(*aResult = GetAnonymousNodesInternal(aContent, &dummy));
  return NS_OK;
}

nsINodeList*
nsBindingManager::GetAnonymousNodesFor(nsIContent* aContent)
{
  bool dummy;
  return GetAnonymousNodesInternal(aContent, &dummy);
}

nsresult
nsBindingManager::SetAnonymousNodesFor(nsIContent* aContent,
                                       nsInsertionPointList* aList)
{
  if (mDestroyed) {
    return NS_OK;
  }

  nsAnonymousContentList* contentList = nullptr;
  if (aList) {
    contentList = new nsAnonymousContentList(aContent, aList);
    if (!contentList) {
      delete aList;
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  return SetOrRemoveObject(mAnonymousNodesTable, aContent, contentList);
}

nsINodeList*
nsBindingManager::GetXBLChildNodesInternal(nsIContent* aContent,
                                           bool* aIsAnonymousContentList)
{
  uint32_t length;

  
  nsINodeList* result = GetAnonymousNodesInternal(aContent,
                                                  aIsAnonymousContentList);
  if (result) {
    result->GetLength(&length);
    if (length == 0)
      result = nullptr;
  }
    
  
  
  
  if (!result) {
    if (mContentListTable.ops) {
      result = static_cast<nsAnonymousContentList*>
                          (LookupObject(mContentListTable, aContent));
      *aIsAnonymousContentList = true;
    }
  }

  return result;
}

nsresult
nsBindingManager::GetXBLChildNodesFor(nsIContent* aContent, nsIDOMNodeList** aResult)
{
  NS_IF_ADDREF(*aResult = GetXBLChildNodesFor(aContent));
  return NS_OK;
}

nsINodeList*
nsBindingManager::GetXBLChildNodesFor(nsIContent* aContent)
{
  bool dummy;
  return GetXBLChildNodesInternal(aContent, &dummy);
}

nsIContent*
nsBindingManager::GetInsertionPoint(nsIContent* aParent,
                                    const nsIContent* aChild,
                                    uint32_t* aIndex)
{
  nsXBLBinding *binding = GetBinding(aParent);
  return binding ? binding->GetInsertionPoint(aChild, aIndex) : nullptr;
}

nsIContent*
nsBindingManager::GetSingleInsertionPoint(nsIContent* aParent,
                                          uint32_t* aIndex,
                                          bool* aMultipleInsertionPoints)
{
  nsXBLBinding *binding = GetBinding(aParent);
  if (binding)
    return binding->GetSingleInsertionPoint(aIndex, aMultipleInsertionPoints);

  *aMultipleInsertionPoints = false;
  return nullptr;
}

nsresult
nsBindingManager::AddLayeredBinding(nsIContent* aContent, nsIURI* aURL,
                                    nsIPrincipal* aOriginPrincipal)
{
  
  nsXBLService* xblService = nsXBLService::GetInstance();
  if (!xblService)
    return NS_ERROR_FAILURE;

  
  nsRefPtr<nsXBLBinding> binding;
  bool dummy;
  xblService->LoadBindings(aContent, aURL, aOriginPrincipal, true,
                           getter_AddRefs(binding), &dummy);
  if (binding) {
    AddToAttachedQueue(binding);
    ProcessAttachedQueue();
  }

  return NS_OK;
}

nsresult
nsBindingManager::RemoveLayeredBinding(nsIContent* aContent, nsIURI* aURL)
{
  
  nsRefPtr<nsXBLBinding> binding = GetBinding(aContent);
  
  if (!binding) {
    return NS_OK;
  }

  
  NS_ENSURE_FALSE(binding->GetBaseBinding(), NS_ERROR_FAILURE);

  
  if (!binding->PrototypeBinding()->CompareBindingURI(aURL)) {
    return NS_OK;
  }

  
  if (binding->IsStyleBinding()) {
    return NS_OK;
  }

  
  
  
  
  
  nsCOMPtr<nsIDocument> doc = aContent->OwnerDoc();
  
  
  
  
  binding->UnhookEventHandlers();
  binding->ChangeDocument(doc, nullptr);
  SetBinding(aContent, nullptr);
  binding->MarkForDeath();
  
  
  
  
  
  nsIPresShell *presShell = doc->GetShell();
  NS_ENSURE_TRUE(presShell, NS_ERROR_FAILURE);

  return presShell->RecreateFramesFor(aContent);;
}

nsresult
nsBindingManager::LoadBindingDocument(nsIDocument* aBoundDoc,
                                      nsIURI* aURL,
                                      nsIPrincipal* aOriginPrincipal)
{
  NS_PRECONDITION(aURL, "Must have a URI to load!");
  
  
  nsXBLService* xblService = nsXBLService::GetInstance();
  if (!xblService)
    return NS_ERROR_FAILURE;

  
  nsRefPtr<nsXBLDocumentInfo> info;
  xblService->LoadBindingDocumentInfo(nullptr, aBoundDoc, aURL,
                                      aOriginPrincipal, true,
                                      getter_AddRefs(info));
  if (!info)
    return NS_ERROR_FAILURE;

  return NS_OK;
}

nsresult
nsBindingManager::AddToAttachedQueue(nsXBLBinding* aBinding)
{
  if (!mAttachedStack.AppendElement(aBinding))
    return NS_ERROR_OUT_OF_MEMORY;

  
  
  if (!mProcessingAttachedStack && !mProcessAttachedQueueEvent) {
    PostProcessAttachedQueueEvent();
  }

  
  mDocument->SetNeedStyleFlush();

  return NS_OK;

}

void
nsBindingManager::PostProcessAttachedQueueEvent()
{
  mProcessAttachedQueueEvent =
    NS_NewRunnableMethod(this, &nsBindingManager::DoProcessAttachedQueue);
  nsresult rv = NS_DispatchToCurrentThread(mProcessAttachedQueueEvent);
  if (NS_SUCCEEDED(rv) && mDocument) {
    mDocument->BlockOnload();
  }
}

void
nsBindingManager::DoProcessAttachedQueue()
{
  if (!mProcessingAttachedStack) {
    ProcessAttachedQueue();

    NS_ASSERTION(mAttachedStack.Length() == 0,
               "Shouldn't have pending bindings!");
  
    mProcessAttachedQueueEvent = nullptr;
  } else {
    
    
    
    PostProcessAttachedQueueEvent();
  }

  
  if (mDocument) {
    
    
    nsCOMPtr<nsIDocument> doc = mDocument;
    doc->UnblockOnload(true);
  }
}

void
nsBindingManager::ProcessAttachedQueue(uint32_t aSkipSize)
{
  if (mProcessingAttachedStack || mAttachedStack.Length() <= aSkipSize)
    return;

  mProcessingAttachedStack = true;

  
  while (mAttachedStack.Length() > aSkipSize) {
    uint32_t lastItem = mAttachedStack.Length() - 1;
    nsRefPtr<nsXBLBinding> binding = mAttachedStack.ElementAt(lastItem);
    mAttachedStack.RemoveElementAt(lastItem);
    if (binding) {
      binding->ExecuteAttachedHandler();
    }
  }

  
  
  if (mDocument) {
    mProcessingAttachedStack = false;
  }

  NS_ASSERTION(mAttachedStack.Length() == aSkipSize, "How did we get here?");
  
  mAttachedStack.Compact();
}


struct BindingTableReadClosure
{
  nsCOMArray<nsIContent> mBoundElements;
  nsBindingList          mBindings;
};

static PLDHashOperator
AccumulateBindingsToDetach(nsISupports *aKey, nsXBLBinding *aBinding,
                           void* aClosure)
 {
  BindingTableReadClosure* closure =
    static_cast<BindingTableReadClosure*>(aClosure);
  if (aBinding && closure->mBindings.AppendElement(aBinding)) {
    if (!closure->mBoundElements.AppendObject(aBinding->GetBoundElement())) {
      closure->mBindings.RemoveElementAt(closure->mBindings.Length() - 1);
    }
  }
  return PL_DHASH_NEXT;
}

void
nsBindingManager::ExecuteDetachedHandlers()
{
  
  if (mBindingTable.IsInitialized()) {
    BindingTableReadClosure closure;
    mBindingTable.EnumerateRead(AccumulateBindingsToDetach, &closure);
    uint32_t i, count = closure.mBindings.Length();
    for (i = 0; i < count; ++i) {
      closure.mBindings[i]->ExecuteDetachedHandler();
    }
  }
}

nsresult
nsBindingManager::PutXBLDocumentInfo(nsXBLDocumentInfo* aDocumentInfo)
{
  NS_PRECONDITION(aDocumentInfo, "Must have a non-null documentinfo!");
  
  if (!mDocumentTable.IsInitialized())
    mDocumentTable.Init(16);

  mDocumentTable.Put(aDocumentInfo->DocumentURI(),
                     aDocumentInfo);

  return NS_OK;
}

void
nsBindingManager::RemoveXBLDocumentInfo(nsXBLDocumentInfo* aDocumentInfo)
{
  if (mDocumentTable.IsInitialized()) {
    mDocumentTable.Remove(aDocumentInfo->DocumentURI());
  }
}

nsXBLDocumentInfo*
nsBindingManager::GetXBLDocumentInfo(nsIURI* aURL)
{
  if (!mDocumentTable.IsInitialized())
    return nullptr;

  return mDocumentTable.GetWeak(aURL);
}

nsresult
nsBindingManager::PutLoadingDocListener(nsIURI* aURL, nsIStreamListener* aListener)
{
  NS_PRECONDITION(aListener, "Must have a non-null listener!");
  
  if (!mLoadingDocTable.IsInitialized())
    mLoadingDocTable.Init(16);
  
  mLoadingDocTable.Put(aURL, aListener);

  return NS_OK;
}

nsIStreamListener*
nsBindingManager::GetLoadingDocListener(nsIURI* aURL)
{
  if (!mLoadingDocTable.IsInitialized())
    return nullptr;

  return mLoadingDocTable.GetWeak(aURL);
}

void
nsBindingManager::RemoveLoadingDocListener(nsIURI* aURL)
{
  if (mLoadingDocTable.IsInitialized()) {
    mLoadingDocTable.Remove(aURL);
  }
}

static PLDHashOperator
MarkForDeath(nsISupports *aKey, nsXBLBinding *aBinding, void* aClosure)
{
  if (aBinding->MarkedForDeath())
    return PL_DHASH_NEXT; 

  nsAutoCString path;
  aBinding->PrototypeBinding()->DocURI()->GetPath(path);

  if (!strncmp(path.get(), "/skin", 5))
    aBinding->MarkForDeath();
  
  return PL_DHASH_NEXT;
}

void
nsBindingManager::FlushSkinBindings()
{
  if (mBindingTable.IsInitialized())
    mBindingTable.EnumerateRead(MarkForDeath, nullptr);
}


struct AntiRecursionData {
  nsIContent* element; 
  REFNSIID iid; 
  AntiRecursionData* next;

  AntiRecursionData(nsIContent* aElement, 
                    REFNSIID aIID, 
                    AntiRecursionData* aNext)
    : element(aElement), iid(aIID), next(aNext) {}
};

nsresult
nsBindingManager::GetBindingImplementation(nsIContent* aContent, REFNSIID aIID,
                                           void** aResult)
{
  *aResult = nullptr;
  nsXBLBinding *binding = GetBinding(aContent);
  if (binding) {
    
    NS_ASSERTION(!aIID.Equals(NS_GET_IID(nsISupports)), "Asking a binding for nsISupports");
    if (binding->ImplementsInterface(aIID)) {
      nsCOMPtr<nsIXPConnectWrappedJS> wrappedJS = GetWrappedJS(aContent);

      if (wrappedJS) {
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        

        static AntiRecursionData* list = nullptr;

        for (AntiRecursionData* p = list; p; p = p->next) {
          if (p->element == aContent && p->iid.Equals(aIID)) {
            *aResult = nullptr;
            return NS_NOINTERFACE;
          }
        }

        AntiRecursionData item(aContent, aIID, list);
        list = &item;

        nsresult rv = wrappedJS->AggregatedQueryInterface(aIID, aResult);
        
        list = item.next;
        
        if (*aResult)
          return rv;
        
        
        
      }

      
      

      nsIDocument* doc = aContent->OwnerDoc();

      nsIScriptGlobalObject *global = doc->GetScriptGlobalObject();
      if (!global)
        return NS_NOINTERFACE;

      nsIScriptContext *context = global->GetContext();
      if (!context)
        return NS_NOINTERFACE;

      JSContext* jscontext = context->GetNativeContext();
      if (!jscontext)
        return NS_NOINTERFACE;

      nsIXPConnect *xpConnect = nsContentUtils::XPConnect();

      JSObject* jsobj = aContent->GetWrapper();
      NS_ENSURE_TRUE(jsobj, NS_NOINTERFACE);

      nsresult rv = xpConnect->WrapJSAggregatedToNative(aContent, jscontext,
                                                        jsobj, aIID, aResult);
      if (NS_FAILED(rv))
        return rv;

      
      
      
      nsISupports* supp = static_cast<nsISupports*>(*aResult);
      wrappedJS = do_QueryInterface(supp);
      SetWrappedJS(aContent, wrappedJS);

      return rv;
    }
  }
  
  *aResult = nullptr;
  return NS_NOINTERFACE;
}

nsresult
nsBindingManager::WalkRules(nsIStyleRuleProcessor::EnumFunc aFunc,
                            ElementDependentRuleProcessorData* aData,
                            bool* aCutOffInheritance)
{
  *aCutOffInheritance = false;
  
  NS_ASSERTION(aData->mElement, "How did that happen?");

  
  
  nsIContent *content = aData->mElement;
  
  do {
    nsXBLBinding *binding = GetBinding(content);
    if (binding) {
      aData->mTreeMatchContext.mScopedRoot = content;
      binding->WalkRules(aFunc, aData);
      
      
      if (content != aData->mElement) {
        if (!binding->InheritsStyle()) {
          
          break;
        }
      }
    }

    if (content->IsRootOfNativeAnonymousSubtree()) {
      break; 
    }

    content = content->GetBindingParent();
  } while (content);

  
  
  *aCutOffInheritance = (content != nullptr);

  
  aData->mTreeMatchContext.mScopedRoot = nullptr;

  return NS_OK;
}

typedef nsTHashtable<nsPtrHashKey<nsIStyleRuleProcessor> > RuleProcessorSet;

static PLDHashOperator
EnumRuleProcessors(nsISupports *aKey, nsXBLBinding *aBinding, void* aClosure)
{
  RuleProcessorSet *set = static_cast<RuleProcessorSet*>(aClosure);
  for (nsXBLBinding *binding = aBinding; binding;
       binding = binding->GetBaseBinding()) {
    nsIStyleRuleProcessor *ruleProc =
      binding->PrototypeBinding()->GetRuleProcessor();
    if (ruleProc) {
      if (!set->IsInitialized()) {
        set->Init(16);
      }
      set->PutEntry(ruleProc);
    }
  }
  return PL_DHASH_NEXT;
}

struct WalkAllRulesData {
  nsIStyleRuleProcessor::EnumFunc mFunc;
  ElementDependentRuleProcessorData* mData;
};

static PLDHashOperator
EnumWalkAllRules(nsPtrHashKey<nsIStyleRuleProcessor> *aKey, void* aClosure)
{
  nsIStyleRuleProcessor *ruleProcessor = aKey->GetKey();
    
  WalkAllRulesData *data = static_cast<WalkAllRulesData*>(aClosure);

  (*(data->mFunc))(ruleProcessor, data->mData);

  return PL_DHASH_NEXT;
}

void
nsBindingManager::WalkAllRules(nsIStyleRuleProcessor::EnumFunc aFunc,
                               ElementDependentRuleProcessorData* aData)
{
  if (!mBindingTable.IsInitialized())
    return;

  RuleProcessorSet set;
  mBindingTable.EnumerateRead(EnumRuleProcessors, &set);
  if (!set.IsInitialized())
    return;

  WalkAllRulesData data = { aFunc, aData };
  set.EnumerateEntries(EnumWalkAllRules, &data);
}

struct MediumFeaturesChangedData {
  nsPresContext *mPresContext;
  bool *mRulesChanged;
};

static PLDHashOperator
EnumMediumFeaturesChanged(nsPtrHashKey<nsIStyleRuleProcessor> *aKey, void* aClosure)
{
  nsIStyleRuleProcessor *ruleProcessor = aKey->GetKey();
    
  MediumFeaturesChangedData *data =
    static_cast<MediumFeaturesChangedData*>(aClosure);

  bool thisChanged = ruleProcessor->MediumFeaturesChanged(data->mPresContext);
  *data->mRulesChanged = *data->mRulesChanged || thisChanged;

  return PL_DHASH_NEXT;
}

nsresult
nsBindingManager::MediumFeaturesChanged(nsPresContext* aPresContext,
                                        bool* aRulesChanged)
{
  *aRulesChanged = false;
  if (!mBindingTable.IsInitialized())
    return NS_OK;

  RuleProcessorSet set;
  mBindingTable.EnumerateRead(EnumRuleProcessors, &set);
  if (!set.IsInitialized())
    return NS_OK;

  MediumFeaturesChangedData data = { aPresContext, aRulesChanged };
  set.EnumerateEntries(EnumMediumFeaturesChanged, &data);
  return NS_OK;
}

static PLDHashOperator
EnumAppendAllSheets(nsISupports *aKey, nsXBLBinding *aBinding, void* aClosure)
{
  nsTArray<nsCSSStyleSheet*>* array =
    static_cast<nsTArray<nsCSSStyleSheet*>*>(aClosure);
  for (nsXBLBinding *binding = aBinding; binding;
       binding = binding->GetBaseBinding()) {
    nsXBLPrototypeResources::sheet_array_type* sheets =
      binding->PrototypeBinding()->GetStyleSheets();
    if (sheets) {
      
      
      array->AppendElements(*sheets);
    }
  }
  return PL_DHASH_NEXT;
}

void
nsBindingManager::AppendAllSheets(nsTArray<nsCSSStyleSheet*>& aArray)
{
  if (!mBindingTable.IsInitialized())
    return;

  mBindingTable.EnumerateRead(EnumAppendAllSheets, &aArray);
}

nsIContent*
nsBindingManager::GetNestedInsertionPoint(nsIContent* aParent,
                                          const nsIContent* aChild)
{
  
  if (aChild->GetBindingParent() == aParent)
    return nullptr; 
                   

  uint32_t index;
  nsIContent *insertionElement = GetInsertionPoint(aParent, aChild, &index);
  if (insertionElement && insertionElement != aParent) {
    
    nsIContent* nestedPoint = GetNestedInsertionPoint(insertionElement, aChild);
    if (nestedPoint)
      insertionElement = nestedPoint;
  }

  return insertionElement;
}

nsIContent*
nsBindingManager::GetNestedSingleInsertionPoint(nsIContent* aParent,
                                                bool* aMultipleInsertionPoints)
{
  *aMultipleInsertionPoints = false;
  
  uint32_t index;
  nsIContent *insertionElement =
    GetSingleInsertionPoint(aParent, &index, aMultipleInsertionPoints);
  if (*aMultipleInsertionPoints) {
    return nullptr;
  }
  if (insertionElement && insertionElement != aParent) {
    
    nsIContent* nestedPoint =
      GetNestedSingleInsertionPoint(insertionElement,
                                    aMultipleInsertionPoints);
    if (nestedPoint)
      insertionElement = nestedPoint;
  }

  return insertionElement;
}

nsXBLInsertionPoint*
nsBindingManager::FindInsertionPointAndIndex(nsIContent* aContainer,
                                             nsIContent* aInsertionParent,
                                             uint32_t aIndexInContainer,
                                             int32_t aAppend,
                                             int32_t* aInsertionIndex)
{
  bool isAnonymousContentList;
  nsINodeList* nodeList =
    GetXBLChildNodesInternal(aInsertionParent, &isAnonymousContentList);
  if (!nodeList || !isAnonymousContentList) {
    return nullptr;
  }

  
  
  
  
  nsAnonymousContentList* contentList =
    static_cast<nsAnonymousContentList*>(nodeList);

  int32_t count = contentList->GetInsertionPointCount();
  for (int32_t i = 0; i < count; i++) {
    nsXBLInsertionPoint* point = contentList->GetInsertionPointAt(i);
    if (point->GetInsertionIndex() != -1) {
      

      
      
      
      
      
      int32_t pointSize = point->ChildCount();
      for (int32_t parentIndex = aIndexInContainer - 1; parentIndex >= 0;
           --parentIndex) {
        nsIContent* currentSibling = aContainer->GetChildAt(parentIndex);
        for (int32_t pointIndex = pointSize - 1; pointIndex >= 0;
             --pointIndex) {
          if (point->ChildAt(pointIndex) == currentSibling) {
            *aInsertionIndex = pointIndex + 1;
            return point;
          }
        }
      }

      
      
      
      
      
      
      
      
      
      if (aAppend) {
        *aInsertionIndex = pointSize;
      } else {
        *aInsertionIndex = 0;
      }
      return point;
    }
  }

  return nullptr;  
}

void
nsBindingManager::ContentAppended(nsIDocument* aDocument,
                                  nsIContent* aContainer,
                                  nsIContent* aFirstNewContent,
                                  int32_t     aNewIndexInContainer)
{
  if (aNewIndexInContainer != -1 &&
      (mContentListTable.ops || mAnonymousNodesTable.ops)) {
    
    NS_ASSERTION(aNewIndexInContainer >= 0, "Bogus index");

    bool multiple;
    nsIContent* ins = GetNestedSingleInsertionPoint(aContainer, &multiple);

    if (multiple) {
      
      int32_t childCount = aContainer->GetChildCount();
      for (int32_t idx = aNewIndexInContainer; idx < childCount; ++idx) {
        HandleChildInsertion(aContainer, aContainer->GetChildAt(idx),
                             idx, true);
      }
    }
    else if (ins) {
      int32_t insertionIndex;
      nsXBLInsertionPoint* point =
        FindInsertionPointAndIndex(aContainer, ins, aNewIndexInContainer,
                                   true, &insertionIndex);
      if (point) {
        int32_t childCount = aContainer->GetChildCount();
        for (int32_t j = aNewIndexInContainer; j < childCount;
             j++, insertionIndex++) {
          nsIContent* child = aContainer->GetChildAt(j);
          point->InsertChildAt(insertionIndex, child);
          SetInsertionParent(child, ins);
        }
      }
    }
  }
}

void
nsBindingManager::ContentInserted(nsIDocument* aDocument,
                                  nsIContent* aContainer,
                                  nsIContent* aChild,
                                  int32_t aIndexInContainer)
{
  if (aIndexInContainer != -1 &&
      (mContentListTable.ops || mAnonymousNodesTable.ops)) {
    
    NS_ASSERTION(aIndexInContainer >= 0, "Bogus index");
    HandleChildInsertion(aContainer, aChild, aIndexInContainer, false);
  }
}

static void
RemoveChildFromInsertionPoint(nsAnonymousContentList* aInsertionPointList,
                              nsIContent* aChild,
                              bool aRemoveFromPseudoPoints)
{
  
  
  
  
  

  
  
  
  
  int32_t count = aInsertionPointList->GetInsertionPointCount();
  for (int32_t i = 0; i < count; i++) {
    nsXBLInsertionPoint* point =
      aInsertionPointList->GetInsertionPointAt(i);
    if ((point->GetInsertionIndex() == -1) == aRemoveFromPseudoPoints) {
      point->RemoveChild(aChild);
    }
  }
}

void
nsBindingManager::ContentRemoved(nsIDocument* aDocument,
                                 nsIContent* aContainer,
                                 nsIContent* aChild,
                                 int32_t aIndexInContainer,
                                 nsIContent* aPreviousSibling)
{
  if (aContainer && aIndexInContainer != -1 &&
      (mContentListTable.ops || mAnonymousNodesTable.ops)) {
    
    nsCOMPtr<nsIContent> point = GetNestedInsertionPoint(aContainer, aChild);

    if (point) {
      bool isAnonymousContentList;
      nsCOMPtr<nsIDOMNodeList> nodeList =
        GetXBLChildNodesInternal(point, &isAnonymousContentList);
      
      if (nodeList && isAnonymousContentList) {
        
        RemoveChildFromInsertionPoint(static_cast<nsAnonymousContentList*>
                                        (static_cast<nsIDOMNodeList*>
                                                    (nodeList)),
                                      aChild,
                                      false);
        SetInsertionParent(aChild, nullptr);
      }

      
      if (mContentListTable.ops) {
        nsCOMPtr<nsIDOMNodeList> otherNodeList =
          static_cast<nsAnonymousContentList*>
                     (LookupObject(mContentListTable, point));
        if (otherNodeList && otherNodeList != nodeList) {
          
          RemoveChildFromInsertionPoint(static_cast<nsAnonymousContentList*>
                                        (static_cast<nsIDOMNodeList*>
                                                    (otherNodeList)),
                                        aChild,
                                        false);
        }
      }
    }

    
    
    
    if (mContentListTable.ops) {
      nsAnonymousContentList* insertionPointList =
        static_cast<nsAnonymousContentList*>(LookupObject(mContentListTable,
                                                          aContainer));
      if (insertionPointList) {
        RemoveChildFromInsertionPoint(insertionPointList, aChild, true);
      }
    }
  }
}

void
nsBindingManager::DropDocumentReference()
{
  mDestroyed = true;

  
  mProcessingAttachedStack = true;
  if (mProcessAttachedQueueEvent) {
    mProcessAttachedQueueEvent->Revoke();
  }

  if (mContentListTable.ops)
    PL_DHashTableFinish(&(mContentListTable));
  mContentListTable.ops = nullptr;

  if (mAnonymousNodesTable.ops)
    PL_DHashTableFinish(&(mAnonymousNodesTable));
  mAnonymousNodesTable.ops = nullptr;

  if (mInsertionParentTable.ops)
    PL_DHashTableFinish(&(mInsertionParentTable));
  mInsertionParentTable.ops = nullptr;

  if (mBindingTable.IsInitialized())
    mBindingTable.Clear();

  mDocument = nullptr;
}

void
nsBindingManager::Traverse(nsIContent *aContent,
                           nsCycleCollectionTraversalCallback &cb)
{
  if (!aContent->HasFlag(NODE_MAY_BE_IN_BINDING_MNGR)) {
    return;
  }

  nsISupports *value;
  if (mInsertionParentTable.ops &&
      (value = LookupObject(mInsertionParentTable, aContent))) {
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "[via binding manager] mInsertionParentTable key");
    cb.NoteXPCOMChild(aContent);
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "[via binding manager] mInsertionParentTable value");
    cb.NoteXPCOMChild(value);
  }

  
  if (!aContent->IsElement()) {
    return;
  }

  nsXBLBinding *binding = GetBinding(aContent);
  if (binding) {
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "[via binding manager] mBindingTable key");
    cb.NoteXPCOMChild(aContent);
    CycleCollectionNoteChild(cb, binding, "[via binding manager] mBindingTable value");
  }
  if (mContentListTable.ops &&
      (value = LookupObject(mContentListTable, aContent))) {
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "[via binding manager] mContentListTable key");
    cb.NoteXPCOMChild(aContent);
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "[via binding manager] mContentListTable value");
    cb.NoteXPCOMChild(value);
  }
  if (mAnonymousNodesTable.ops &&
      (value = LookupObject(mAnonymousNodesTable, aContent))) {
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "[via binding manager] mAnonymousNodesTable key");
    cb.NoteXPCOMChild(aContent);
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "[via binding manager] mAnonymousNodesTable value");
    cb.NoteXPCOMChild(value);
  }
  if (mWrapperTable.ops &&
      (value = LookupObject(mWrapperTable, aContent))) {
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "[via binding manager] mWrapperTable key");
    cb.NoteXPCOMChild(aContent);
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "[via binding manager] mWrapperTable value");
    cb.NoteXPCOMChild(value);
  }
}

void
nsBindingManager::BeginOutermostUpdate()
{
  mAttachedStackSizeOnOutermost = mAttachedStack.Length();
}

void
nsBindingManager::EndOutermostUpdate()
{
  if (!mProcessingAttachedStack) {
    ProcessAttachedQueue(mAttachedStackSizeOnOutermost);
    mAttachedStackSizeOnOutermost = 0;
  }
}

void
nsBindingManager::HandleChildInsertion(nsIContent* aContainer,
                                       nsIContent* aChild,
                                       uint32_t aIndexInContainer,
                                       bool aAppend)
{
  NS_PRECONDITION(aChild, "Must have child");
  NS_PRECONDITION(!aContainer ||
                  uint32_t(aContainer->IndexOf(aChild)) == aIndexInContainer,
                  "Child not at the right index?");

  nsIContent* ins = GetNestedInsertionPoint(aContainer, aChild);

  if (ins) {
    int32_t insertionIndex;
    nsXBLInsertionPoint* point =
      FindInsertionPointAndIndex(aContainer, ins, aIndexInContainer, aAppend,
                                 &insertionIndex);
    if (point) {
      point->InsertChildAt(insertionIndex, aChild);
      SetInsertionParent(aChild, ins);
    }
  }
}
