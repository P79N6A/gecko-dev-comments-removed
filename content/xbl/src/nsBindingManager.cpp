






































#include "nsCOMPtr.h"
#include "nsIXBLService.h"
#include "nsIInputStream.h"
#include "nsDoubleHashtable.h"
#include "nsIURI.h"
#include "nsIURL.h"
#include "nsIChannel.h"
#include "nsXPIDLString.h"
#include "nsIParser.h"
#include "nsParserCIID.h"
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
#include "nsGenericDOMNodeList.h"

#include "nsXBLBinding.h"
#include "nsXBLPrototypeBinding.h"
#include "nsIXBLDocumentInfo.h"
#include "nsXBLInsertionPoint.h"

#include "nsIStyleSheet.h"
#include "nsHTMLStyleSheet.h"
#include "nsIHTMLCSSStyleSheet.h"

#include "nsIStyleRuleProcessor.h"
#include "nsIWeakReference.h"

#include "jsapi.h"
#include "nsIXPConnect.h"
#include "nsDOMCID.h"
#include "nsIDOMScriptObjectFactory.h"
#include "nsIScriptGlobalObject.h"

#include "nsIScriptContext.h"
#include "nsBindingManager.h"

#include "nsThreadUtils.h"





class nsAnonymousContentList : public nsGenericDOMNodeList
{
public:
  nsAnonymousContentList(nsInsertionPointList* aElements);
  virtual ~nsAnonymousContentList();

  
  NS_DECL_NSIDOMNODELIST

  PRInt32 GetInsertionPointCount() { return mElements->Length(); }

  nsXBLInsertionPoint* GetInsertionPointAt(PRInt32 i) { return NS_STATIC_CAST(nsXBLInsertionPoint*, mElements->ElementAt(i)); }
  void RemoveInsertionPointAt(PRInt32 i) { mElements->RemoveElementAt(i); }

private:
  nsInsertionPointList* mElements;
};

nsAnonymousContentList::nsAnonymousContentList(nsInsertionPointList* aElements)
  : mElements(aElements)
{
  MOZ_COUNT_CTOR(nsAnonymousContentList);

  
  
}

nsAnonymousContentList::~nsAnonymousContentList()
{
  MOZ_COUNT_DTOR(nsAnonymousContentList);
  delete mElements;
}

NS_IMETHODIMP
nsAnonymousContentList::GetLength(PRUint32* aLength)
{
  NS_ASSERTION(aLength != nsnull, "null ptr");
  if (! aLength)
      return NS_ERROR_NULL_POINTER;

  PRInt32 cnt = mElements->Length();

  *aLength = 0;
  for (PRInt32 i = 0; i < cnt; i++)
    *aLength += NS_STATIC_CAST(nsXBLInsertionPoint*, mElements->ElementAt(i))->ChildCount();

  return NS_OK;
}

NS_IMETHODIMP    
nsAnonymousContentList::Item(PRUint32 aIndex, nsIDOMNode** aReturn)
{
  PRInt32 cnt = mElements->Length();
  PRUint32 pointCount = 0;

  for (PRInt32 i = 0; i < cnt; i++) {
    aIndex -= pointCount;
    
    nsXBLInsertionPoint* point = NS_STATIC_CAST(nsXBLInsertionPoint*, mElements->ElementAt(i));
    pointCount = point->ChildCount();

    if (aIndex < pointCount) {
      nsCOMPtr<nsIContent> result = point->ChildAt(aIndex);
      if (result)
        return CallQueryInterface(result, aReturn);
      return NS_ERROR_FAILURE;
    }
  }

  return NS_ERROR_FAILURE;
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

PR_STATIC_CALLBACK(void)
ClearObjectEntry(PLDHashTable* table, PLDHashEntryHdr *entry)
{
  ObjectEntry* objEntry = NS_STATIC_CAST(ObjectEntry*, entry);
  objEntry->~ObjectEntry();
}

PR_STATIC_CALLBACK(PRBool)
InitObjectEntry(PLDHashTable* table, PLDHashEntryHdr* entry, const void* key)
{
  new (entry) ObjectEntry;
  return PR_TRUE;
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
    NS_STATIC_CAST(ObjectEntry*,
                   PL_DHashTableOperate(&table, aKey, PL_DHASH_ADD));

  if (!entry)
    return NS_ERROR_OUT_OF_MEMORY;

  
  if (!entry->GetKey())
    entry->SetKey(aKey);

  
  
  entry->SetValue(aValue);
  
  return NS_OK;
}



static nsISupports*
LookupObject(PLDHashTable& table, nsISupports* aKey)
{
  ObjectEntry *entry =
    NS_STATIC_CAST(ObjectEntry*,
                   PL_DHashTableOperate(&table, aKey, PL_DHASH_LOOKUP));

  if (PL_DHASH_ENTRY_IS_BUSY(entry))
    return entry->GetValue();

  return nsnull;
}

inline void
RemoveObjectEntry(PLDHashTable& table, nsISupports* aKey)
{
  PL_DHashTableOperate(&table, aKey, PL_DHASH_REMOVE);
}

static nsresult
SetOrRemoveObject(PLDHashTable& table, nsISupports* aKey, nsISupports* aValue)
{
  if (aValue) {
    
    if (!table.ops &&
        !PL_DHashTableInit(&table, &ObjectTableOps, nsnull,
                           sizeof(ObjectEntry), 16)) {
      table.ops = nsnull;
      return NS_ERROR_OUT_OF_MEMORY;
    }
    return AddObjectEntry(table, aKey, aValue);
  }

  
  if (table.ops)
    RemoveObjectEntry(table, aKey);
  return NS_OK;
}







NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsBindingManager)
  if (tmp->mBindingTable.IsInitialized())
    tmp->mBindingTable.Clear();

  if (tmp->mDocumentTable.IsInitialized())
    tmp->mDocumentTable.Clear();

  if (tmp->mLoadingDocTable.IsInitialized())
    tmp->mLoadingDocTable.Clear();

  if (tmp->mContentListTable.ops)
    PL_DHashTableFinish(&(tmp->mContentListTable));
  tmp->mContentListTable.ops = nsnull;

  if (tmp->mAnonymousNodesTable.ops)
    PL_DHashTableFinish(&(tmp->mAnonymousNodesTable));
  tmp->mAnonymousNodesTable.ops = nsnull;

  if (tmp->mInsertionParentTable.ops)
    PL_DHashTableFinish(&(tmp->mInsertionParentTable));
  tmp->mInsertionParentTable.ops = nsnull;

  if (tmp->mWrapperTable.ops)
    PL_DHashTableFinish(&(tmp->mWrapperTable));
  tmp->mWrapperTable.ops = nsnull;

  tmp->mAttachedStack.Clear();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END


static PLDHashOperator
DocumentInfoHashtableTraverser(nsIURI* key,
                               nsIXBLDocumentInfo* di,
                               void* userArg)
{
  nsCycleCollectionTraversalCallback *cb = 
    NS_STATIC_CAST(nsCycleCollectionTraversalCallback*, userArg);
  cb->NoteXPCOMChild(di);
  return PL_DHASH_NEXT;
}

static PLDHashOperator
LoadingDocHashtableTraverser(nsIURI* key,
                             nsIStreamListener* sl,
                             void* userArg)
{
  nsCycleCollectionTraversalCallback *cb = 
    NS_STATIC_CAST(nsCycleCollectionTraversalCallback*, userArg);
  cb->NoteXPCOMChild(sl);
  return PL_DHASH_NEXT;
}

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsBindingManager)
  
  if (tmp->mDocumentTable.IsInitialized())
      tmp->mDocumentTable.EnumerateRead(&DocumentInfoHashtableTraverser, &cb);
  if (tmp->mLoadingDocTable.IsInitialized())
      tmp->mLoadingDocTable.EnumerateRead(&LoadingDocHashtableTraverser, &cb);
  
  
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_CLASS(nsBindingManager)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsBindingManager)
  NS_INTERFACE_MAP_ENTRY(nsIMutationObserver)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsBindingManager)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsBindingManager)


nsBindingManager::nsBindingManager(void)
: mProcessingAttachedStack(PR_FALSE)
{
  mContentListTable.ops = nsnull;
  mAnonymousNodesTable.ops = nsnull;
  mInsertionParentTable.ops = nsnull;
  mWrapperTable.ops = nsnull;
}

nsBindingManager::~nsBindingManager(void)
{
  if (mContentListTable.ops)
    PL_DHashTableFinish(&mContentListTable);
  if (mAnonymousNodesTable.ops)
    PL_DHashTableFinish(&mAnonymousNodesTable);
  if (mInsertionParentTable.ops)
    PL_DHashTableFinish(&mInsertionParentTable);
  if (mWrapperTable.ops)
    PL_DHashTableFinish(&mWrapperTable);
}

nsXBLBinding*
nsBindingManager::GetBinding(nsIContent* aContent)
{
  if (mBindingTable.IsInitialized())
    return mBindingTable.GetWeak(aContent);

  return nsnull;
}

nsresult
nsBindingManager::SetBinding(nsIContent* aContent, nsXBLBinding* aBinding)
{
  if (!mBindingTable.IsInitialized()) {
    if (!mBindingTable.Init())
      return NS_ERROR_OUT_OF_MEMORY;
  }

  
  
  
  
  
  
  nsXBLBinding* oldBinding = mBindingTable.GetWeak(aContent);
  if (oldBinding) {
    mAttachedStack.RemoveElement(oldBinding);
  }
  
  PRBool result = PR_TRUE;

  if (aBinding) {
    result = mBindingTable.Put(aContent, aBinding);
  } else {
    mBindingTable.Remove(aContent);

    
    
    
    SetWrappedJS(aContent, nsnull);
    SetContentListFor(aContent, nsnull);
    SetAnonymousNodesFor(aContent, nsnull);
  }

  return result ? NS_OK : NS_ERROR_FAILURE;
}

nsIContent*
nsBindingManager::GetInsertionParent(nsIContent* aContent)
{ 
  if (mInsertionParentTable.ops) {
    return NS_STATIC_CAST(nsIContent*,
                          LookupObject(mInsertionParentTable, aContent));
  }

  return nsnull;
}

nsresult
nsBindingManager::SetInsertionParent(nsIContent* aContent, nsIContent* aParent)
{
  return SetOrRemoveObject(mInsertionParentTable, aContent, aParent);
}

nsIXPConnectWrappedJS*
nsBindingManager::GetWrappedJS(nsIContent* aContent)
{ 
  if (mWrapperTable.ops) {
    return NS_STATIC_CAST(nsIXPConnectWrappedJS*, LookupObject(mWrapperTable, aContent));
  }

  return nsnull;
}

nsresult
nsBindingManager::SetWrappedJS(nsIContent* aContent, nsIXPConnectWrappedJS* aWrappedJS)
{
  return SetOrRemoveObject(mWrapperTable, aContent, aWrappedJS);
}

nsresult
nsBindingManager::ChangeDocumentFor(nsIContent* aContent, nsIDocument* aOldDocument,
                                    nsIDocument* aNewDocument)
{
  
  
  NS_PRECONDITION(aOldDocument != nsnull, "no old document");
  NS_PRECONDITION(!aNewDocument,
                  "Changing to a non-null new document not supported yet");
  if (! aOldDocument)
    return NS_ERROR_NULL_POINTER;

  
  
  nsRefPtr<nsXBLBinding> binding = GetBinding(aContent);
  if (binding) {
    binding->ChangeDocument(aOldDocument, aNewDocument);
    SetBinding(aContent, nsnull);
    if (aNewDocument)
      aNewDocument->BindingManager()->SetBinding(aContent, binding);
  }

  
  SetInsertionParent(aContent, nsnull);
  SetContentListFor(aContent, nsnull);
  SetAnonymousNodesFor(aContent, nsnull);

  return NS_OK;
}

nsIAtom*
nsBindingManager::ResolveTag(nsIContent* aContent, PRInt32* aNameSpaceID)
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
  
  *aResult = nsnull;
  
  if (mContentListTable.ops) {
    *aResult = NS_STATIC_CAST(nsIDOMNodeList*,
                              LookupObject(mContentListTable, aContent));
    NS_IF_ADDREF(*aResult);
  }
  
  if (!*aResult) {
    nsCOMPtr<nsIDOMNode> node(do_QueryInterface(aContent));
    node->GetChildNodes(aResult);
  }

  return NS_OK;
}

nsresult
nsBindingManager::SetContentListFor(nsIContent* aContent,
                                    nsInsertionPointList* aList)
{
  nsIDOMNodeList* contentList = nsnull;
  if (aList) {
    contentList = new nsAnonymousContentList(aList);
    if (!contentList) {
      delete aList;
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  return SetOrRemoveObject(mContentListTable, aContent, contentList);
}

PRBool
nsBindingManager::HasContentListFor(nsIContent* aContent)
{
  return mContentListTable.ops && LookupObject(mContentListTable, aContent);
}

nsresult
nsBindingManager::GetAnonymousNodesInternal(nsIContent* aContent,
                                            nsIDOMNodeList** aResult,
                                            PRBool* aIsAnonymousContentList)
{ 
  
  *aResult = nsnull;
  if (mAnonymousNodesTable.ops) {
    *aResult = NS_STATIC_CAST(nsIDOMNodeList*,
                              LookupObject(mAnonymousNodesTable, aContent));
    NS_IF_ADDREF(*aResult);
  }

  if (!*aResult) {
    *aIsAnonymousContentList = PR_FALSE;
    nsXBLBinding *binding = GetBinding(aContent);
    if (binding) {
      *aResult = binding->GetAnonymousNodes().get();
      return NS_OK;
    }
  } else
    *aIsAnonymousContentList = PR_TRUE;

  return NS_OK;
}

nsresult
nsBindingManager::GetAnonymousNodesFor(nsIContent* aContent,
                                       nsIDOMNodeList** aResult)
{
  PRBool dummy;
  return GetAnonymousNodesInternal(aContent, aResult, &dummy);
}

nsresult
nsBindingManager::SetAnonymousNodesFor(nsIContent* aContent,
                                       nsInsertionPointList* aList)
{
  nsIDOMNodeList* contentList = nsnull;
  if (aList) {
    contentList = new nsAnonymousContentList(aList);
    if (!contentList) {
      delete aList;
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  return SetOrRemoveObject(mAnonymousNodesTable, aContent, contentList);
}

nsresult
nsBindingManager::GetXBLChildNodesInternal(nsIContent* aContent,
                                           nsIDOMNodeList** aResult,
                                           PRBool* aIsAnonymousContentList)
{
  *aResult = nsnull;

  PRUint32 length;

  
  nsCOMPtr<nsIDOMNodeList> result;
  GetAnonymousNodesInternal(aContent, getter_AddRefs(result),
                            aIsAnonymousContentList);
  if (result) {
    result->GetLength(&length);
    if (length == 0)
      result = nsnull;
  }
    
  
  
  
  if (!result) {
    if (mContentListTable.ops) {
      result = NS_STATIC_CAST(nsIDOMNodeList*,
                              LookupObject(mContentListTable, aContent));
      *aIsAnonymousContentList = PR_TRUE;
    }
  }

  result.swap(*aResult);

  return NS_OK;
}

nsresult
nsBindingManager::GetXBLChildNodesFor(nsIContent* aContent, nsIDOMNodeList** aResult)
{
  PRBool dummy;
  return GetXBLChildNodesInternal(aContent, aResult, &dummy);
}

nsIContent*
nsBindingManager::GetInsertionPoint(nsIContent* aParent, nsIContent* aChild,
                                    PRUint32* aIndex)
{
  nsXBLBinding *binding = GetBinding(aParent);
  return binding ? binding->GetInsertionPoint(aChild, aIndex) : nsnull;
}

nsIContent*
nsBindingManager::GetSingleInsertionPoint(nsIContent* aParent,
                                          PRUint32* aIndex,
                                          PRBool* aMultipleInsertionPoints)
{
  nsXBLBinding *binding = GetBinding(aParent);
  if (binding)
    return binding->GetSingleInsertionPoint(aIndex, aMultipleInsertionPoints);

  *aMultipleInsertionPoints = PR_FALSE;
  return nsnull;
}

nsresult
nsBindingManager::AddLayeredBinding(nsIContent* aContent, nsIURI* aURL)
{
  
  nsresult rv;
  nsCOMPtr<nsIXBLService> xblService = 
           do_GetService("@mozilla.org/xbl;1", &rv);
  if (!xblService)
    return rv;

  
  nsRefPtr<nsXBLBinding> binding;
  PRBool dummy;
  xblService->LoadBindings(aContent, aURL, PR_TRUE, getter_AddRefs(binding),
                           &dummy);
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

  
  nsIURI* bindingUri = binding->PrototypeBinding()->BindingURI();
  
  PRBool equalUri;
  nsresult rv = aURL->Equals(bindingUri, &equalUri);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!equalUri) {
    return NS_OK;
  }

  
  if (binding->IsStyleBinding()) {
    return NS_OK;
  }

  
  
  
  
  
  nsCOMPtr<nsIDocument> doc = aContent->GetOwnerDoc();
  NS_ASSERTION(doc, "No owner document?");
  
  
  binding->UnhookEventHandlers();
  binding->ChangeDocument(doc, nsnull);
  SetBinding(aContent, nsnull);
  binding->MarkForDeath();
  
  
  
  
  
  nsIPresShell *presShell = doc->GetShellAt(0);
  NS_ENSURE_TRUE(presShell, NS_ERROR_FAILURE);

  return presShell->RecreateFramesFor(aContent);;
}

nsresult
nsBindingManager::LoadBindingDocument(nsIDocument* aBoundDoc,
                                      nsIURI* aURL,
                                      nsIDocument** aResult)
{
  NS_PRECONDITION(aURL, "Must have a URI to load!");
  
  nsCAutoString otherScheme;
  aURL->GetScheme(otherScheme);
  
  nsCAutoString scheme;
  aBoundDoc->GetDocumentURI()->GetScheme(scheme);

  
  *aResult = nsnull;
  nsresult rv;
  nsCOMPtr<nsIXBLService> xblService = 
           do_GetService("@mozilla.org/xbl;1", &rv);
  if (!xblService)
    return rv;

  
  nsCOMPtr<nsIXBLDocumentInfo> info;
  xblService->LoadBindingDocumentInfo(nsnull, aBoundDoc, aURL,
                                      PR_TRUE, getter_AddRefs(info));
  if (!info)
    return NS_ERROR_FAILURE;

  
  
    if (!strcmp(scheme.get(), otherScheme.get()))
    info->GetDocument(aResult); 
    
  return NS_OK;
}

nsresult
nsBindingManager::AddToAttachedQueue(nsXBLBinding* aBinding)
{
  if (!mAttachedStack.AppendElement(aBinding))
    return NS_ERROR_OUT_OF_MEMORY;

  
  
  if (!mProcessingAttachedStack && !mProcessAttachedQueueEvent) {
    mProcessAttachedQueueEvent =
      new nsRunnableMethod<nsBindingManager>(
        this, &nsBindingManager::DoProcessAttachedQueue);
    NS_DispatchToCurrentThread(mProcessAttachedQueueEvent);
  }

  return NS_OK;
}

void
nsBindingManager::DoProcessAttachedQueue()
{
  ProcessAttachedQueue();

  NS_ASSERTION(mAttachedStack.Length() == 0,
               "Shouldn't have pending bindings!");
  
  mProcessAttachedQueueEvent = nsnull;
}

void
nsBindingManager::ProcessAttachedQueue()
{
  if (mProcessingAttachedStack || mAttachedStack.Length() == 0)
    return;

  mProcessingAttachedStack = PR_TRUE;

  PRInt32 lastItem;
  while ((lastItem = mAttachedStack.Length() - 1) >= 0) {
    nsRefPtr<nsXBLBinding> binding = mAttachedStack.ElementAt(lastItem);
    mAttachedStack.RemoveElementAt(lastItem);

    NS_ASSERTION(binding, "null item in attached stack?");
    binding->ExecuteAttachedHandler();
  }

  mProcessingAttachedStack = PR_FALSE;

  NS_ASSERTION(mAttachedStack.Length() == 0, "How did we get here?");
  
  mAttachedStack.Compact();
}

PR_STATIC_CALLBACK(PLDHashOperator)
AccumulateBindingsToDetach(nsISupports *aKey, nsXBLBinding *aBinding,
                           void* aVoidArray)
{
  nsVoidArray* arr = NS_STATIC_CAST(nsVoidArray*, aVoidArray);
  
  if (arr->AppendElement(aBinding))
    NS_ADDREF(aBinding);
  return PL_DHASH_NEXT;
}

PR_STATIC_CALLBACK(PRBool)
ExecuteDetachedHandler(void* aBinding, void* aClosure)
{
  NS_PRECONDITION(aBinding, "Null binding in list?");
  nsXBLBinding* binding = NS_STATIC_CAST(nsXBLBinding*, aBinding);
  binding->ExecuteDetachedHandler();
  
  NS_RELEASE(binding);
  return PR_TRUE;
}

void
nsBindingManager::ExecuteDetachedHandlers()
{
  
  if (mBindingTable.IsInitialized()) {
    nsVoidArray bindingsToDetach;
    mBindingTable.EnumerateRead(AccumulateBindingsToDetach, &bindingsToDetach);
    bindingsToDetach.EnumerateForwards(ExecuteDetachedHandler, nsnull);
  }
}

nsresult
nsBindingManager::PutXBLDocumentInfo(nsIXBLDocumentInfo* aDocumentInfo)
{
  NS_PRECONDITION(aDocumentInfo, "Must have a non-null documentinfo!");
  
  NS_ENSURE_TRUE(mDocumentTable.IsInitialized() || mDocumentTable.Init(16),
                 NS_ERROR_OUT_OF_MEMORY);

  NS_ENSURE_TRUE(mDocumentTable.Put(aDocumentInfo->DocumentURI(),
                                    aDocumentInfo),
                 NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}

void
nsBindingManager::RemoveXBLDocumentInfo(nsIXBLDocumentInfo* aDocumentInfo)
{
  if (mDocumentTable.IsInitialized()) {
    mDocumentTable.Remove(aDocumentInfo->DocumentURI());
  }
}

nsIXBLDocumentInfo*
nsBindingManager::GetXBLDocumentInfo(nsIURI* aURL)
{
  if (!mDocumentTable.IsInitialized())
    return nsnull;

  return mDocumentTable.GetWeak(aURL);
}

nsresult
nsBindingManager::PutLoadingDocListener(nsIURI* aURL, nsIStreamListener* aListener)
{
  NS_PRECONDITION(aListener, "Must have a non-null listener!");
  
  NS_ENSURE_TRUE(mLoadingDocTable.IsInitialized() || mLoadingDocTable.Init(16),
                 NS_ERROR_OUT_OF_MEMORY);
  
  NS_ENSURE_TRUE(mLoadingDocTable.Put(aURL, aListener),
                 NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}

nsIStreamListener*
nsBindingManager::GetLoadingDocListener(nsIURI* aURL)
{
  if (!mLoadingDocTable.IsInitialized())
    return nsnull;

  return mLoadingDocTable.GetWeak(aURL);
}

void
nsBindingManager::RemoveLoadingDocListener(nsIURI* aURL)
{
  if (mLoadingDocTable.IsInitialized()) {
    mLoadingDocTable.Remove(aURL);
  }
}

PR_STATIC_CALLBACK(PLDHashOperator)
MarkForDeath(nsISupports *aKey, nsXBLBinding *aBinding, void* aClosure)
{
  if (aBinding->MarkedForDeath())
    return PL_DHASH_NEXT; 

  nsCAutoString path;
  aBinding->PrototypeBinding()->DocURI()->GetPath(path);

  if (!strncmp(path.get(), "/skin", 5))
    aBinding->MarkForDeath();
  
  return PL_DHASH_NEXT;
}

void
nsBindingManager::FlushSkinBindings()
{
  if (mBindingTable.IsInitialized())
    mBindingTable.EnumerateRead(MarkForDeath, nsnull);
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
  *aResult = nsnull;
  nsXBLBinding *binding = GetBinding(aContent);
  if (binding) {
    
    NS_ASSERTION(!aIID.Equals(NS_GET_IID(nsISupports)), "Asking a binding for nsISupports");
    if (binding->ImplementsInterface(aIID)) {
      nsCOMPtr<nsIXPConnectWrappedJS> wrappedJS = GetWrappedJS(aContent);

      if (wrappedJS) {
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        

        static AntiRecursionData* list = nsnull;

        for (AntiRecursionData* p = list; p; p = p->next) {
          if (p->element == aContent && p->iid.Equals(aIID)) {
            *aResult = nsnull;
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

      
      

      nsIDocument* doc = aContent->GetOwnerDoc();
      if (!doc)
        return NS_NOINTERFACE;

      nsIScriptGlobalObject *global = doc->GetScriptGlobalObject();
      if (!global)
        return NS_NOINTERFACE;

      nsIScriptContext *context = global->GetContext();
      if (!context)
        return NS_NOINTERFACE;

      JSContext* jscontext = (JSContext*)context->GetNativeContext();
      if (!jscontext)
        return NS_NOINTERFACE;

      nsIXPConnect *xpConnect = nsContentUtils::XPConnect();

      nsCOMPtr<nsIXPConnectWrappedNative> wrapper;
      xpConnect->GetWrappedNativeOfNativeObject(jscontext,
                                                global->GetGlobalJSObject(),
                                                aContent,
                                                NS_GET_IID(nsISupports),
                                                getter_AddRefs(wrapper));
      NS_ENSURE_TRUE(wrapper, NS_NOINTERFACE);

      JSObject* jsobj = nsnull;

      wrapper->GetJSObject(&jsobj);
      NS_ENSURE_TRUE(jsobj, NS_NOINTERFACE);

      nsresult rv = xpConnect->WrapJSAggregatedToNative(aContent, jscontext,
                                                        jsobj, aIID, aResult);
      if (NS_FAILED(rv))
        return rv;

      
      
      
      nsISupports* supp = NS_STATIC_CAST(nsISupports*, *aResult);
      wrappedJS = do_QueryInterface(supp);
      SetWrappedJS(aContent, wrappedJS);

      return rv;
    }
  }
  
  *aResult = nsnull;
  return NS_NOINTERFACE;
}

nsresult
nsBindingManager::WalkRules(nsStyleSet* aStyleSet,
                            nsIStyleRuleProcessor::EnumFunc aFunc,
                            RuleProcessorData* aData,
                            PRBool* aCutOffInheritance)
{
  *aCutOffInheritance = PR_FALSE;
  
  if (!aData->mContent)
    return NS_OK;

  
  
  nsIContent *content = aData->mContent;
  
  do {
    nsXBLBinding *binding = GetBinding(content);
    if (binding) {
      aData->mScopedRoot = content;
      binding->WalkRules(aFunc, aData);
      
      
      if (content != aData->mContent) {
        if (!binding->InheritsStyle()) {
          
          break;
        }
      }
    }

    nsIContent* parent = content->GetBindingParent();
    if (parent == content)
      break; 
             
             
             

    content = parent;
  } while (content);

  
  
  *aCutOffInheritance = (content != nsnull);

  
  aData->mScopedRoot = nsnull;

  return NS_OK;
}

PRBool
nsBindingManager::ShouldBuildChildFrames(nsIContent* aContent)
{
  nsXBLBinding *binding = GetBinding(aContent);

  return !binding || binding->ShouldBuildChildFrames();
}

nsIContent*
nsBindingManager::GetNestedInsertionPoint(nsIContent* aParent, nsIContent* aChild)
{
  
  if (aChild->GetBindingParent() == aParent)
    return nsnull; 
                   

  PRUint32 index;
  nsIContent *insertionElement = GetInsertionPoint(aParent, aChild, &index);
  if (insertionElement != aParent) {
    
    nsIContent* nestedPoint = GetNestedInsertionPoint(insertionElement, aChild);
    if (nestedPoint)
      insertionElement = nestedPoint;
  }

  return insertionElement;
}



void
nsBindingManager::AddObserver(nsIMutationObserver* aObserver)
{
  
  mObservers.AppendObserver(aObserver);
}

PRBool
nsBindingManager::RemoveObserver(nsIMutationObserver* aObserver)
{
  return mObservers.RemoveObserver(aObserver);
}

void
nsBindingManager::CharacterDataChanged(nsIDocument* aDocument,
                                       nsIContent* aContent,
                                       CharacterDataChangeInfo* aInfo)
{
  NS_BINDINGMANAGER_NOTIFY_OBSERVERS(CharacterDataChanged,
                                     (aDocument, aContent, aInfo));
}

void
nsBindingManager::AttributeChanged(nsIDocument* aDocument,
                                   nsIContent* aContent,
                                   PRInt32 aNameSpaceID,
                                   nsIAtom* aAttribute,
                                   PRInt32 aModType)
{
  NS_BINDINGMANAGER_NOTIFY_OBSERVERS(AttributeChanged,
                                     (aDocument, aContent, aNameSpaceID,
                                      aAttribute, aModType));
}

void
nsBindingManager::ContentAppended(nsIDocument* aDocument,
                                  nsIContent* aContainer,
                                  PRInt32     aNewIndexInContainer)
{
  
  if (aNewIndexInContainer != -1 &&
      (mContentListTable.ops || mAnonymousNodesTable.ops)) {
    
    PRInt32 childCount = aContainer->GetChildCount();

    nsIContent *child = aContainer->GetChildAt(aNewIndexInContainer);

    nsCOMPtr<nsIContent> ins = GetNestedInsertionPoint(aContainer, child);

    if (ins) {
      nsCOMPtr<nsIDOMNodeList> nodeList;
      PRBool isAnonymousContentList;
      GetXBLChildNodesInternal(ins, getter_AddRefs(nodeList),
                               &isAnonymousContentList);

      if (nodeList && isAnonymousContentList) {
        
        
        nsAnonymousContentList* contentList =
          NS_STATIC_CAST(nsAnonymousContentList*, nodeList.get());

        PRInt32 count = contentList->GetInsertionPointCount();
        for (PRInt32 i = 0; i < count; i++) {
          nsXBLInsertionPoint* point = contentList->GetInsertionPointAt(i);
          PRInt32 index = point->GetInsertionIndex();
          if (index != -1) {
            
            
            for (PRInt32 j = aNewIndexInContainer; j < childCount; j++) {
              child = aContainer->GetChildAt(j);
              point->AddChild(child);
              SetInsertionParent(child, ins);
            }
            break;
          }
        }
      }
    }
  }

  NS_BINDINGMANAGER_NOTIFY_OBSERVERS(ContentAppended,
                                     (aDocument, aContainer,
                                      aNewIndexInContainer));
}

void
nsBindingManager::ContentInserted(nsIDocument* aDocument,
                                  nsIContent* aContainer,
                                  nsIContent* aChild,
                                  PRInt32 aIndexInContainer)
{
  
  if (aIndexInContainer != -1 &&
      (mContentListTable.ops || mAnonymousNodesTable.ops)) {
    
    nsCOMPtr<nsIContent> ins = GetNestedInsertionPoint(aContainer, aChild);

    if (ins) {
      nsCOMPtr<nsIDOMNodeList> nodeList;
      PRBool isAnonymousContentList;
      GetXBLChildNodesInternal(ins, getter_AddRefs(nodeList),
                               &isAnonymousContentList);

      if (nodeList && isAnonymousContentList) {
        
        
        nsAnonymousContentList* contentList =
          NS_STATIC_CAST(nsAnonymousContentList*, nodeList.get());

        PRInt32 count = contentList->GetInsertionPointCount();
        for (PRInt32 i = 0; i < count; i++) {
          nsXBLInsertionPoint* point = contentList->GetInsertionPointAt(i);
          if (point->GetInsertionIndex() != -1) {
            
            

            
            
            
            
            
            PRInt32 pointSize = point->ChildCount();
            PRBool inserted = PR_FALSE;
            for (PRInt32 parentIndex = aIndexInContainer - 1;
                 parentIndex >= 0 && !inserted; --parentIndex) {
              nsIContent* currentSibling = aContainer->GetChildAt(parentIndex);
              for (PRInt32 pointIndex = pointSize - 1; pointIndex >= 0;
                   --pointIndex) {
                nsCOMPtr<nsIContent> currContent = point->ChildAt(pointIndex);
                if (currContent == currentSibling) {
                  point->InsertChildAt(pointIndex + 1, aChild);
                  inserted = PR_TRUE;
                  break;
                }
              }
            }
            if (!inserted) {
              
              
              
              
              
              point->InsertChildAt(0, aChild);
            }
            SetInsertionParent(aChild, ins);
            break;
          }
        }
      }
    }
  }

  NS_BINDINGMANAGER_NOTIFY_OBSERVERS(ContentInserted,
                                     (aDocument, aContainer, aChild,
                                      aIndexInContainer));  
}

void
nsBindingManager::ContentRemoved(nsIDocument* aDocument,
                                 nsIContent* aContainer,
                                 nsIContent* aChild,
                                 PRInt32 aIndexInContainer)
{
  NS_BINDINGMANAGER_NOTIFY_OBSERVERS(ContentRemoved,
                                     (aDocument, aContainer, aChild,
                                      aIndexInContainer));  

  if (aIndexInContainer == -1 ||
      (!mContentListTable.ops && !mAnonymousNodesTable.ops))
    
    return;

  nsCOMPtr<nsIContent> point = GetNestedInsertionPoint(aContainer, aChild);

  if (point) {
    nsCOMPtr<nsIDOMNodeList> nodeList;
    PRBool isAnonymousContentList;
    GetXBLChildNodesInternal(point, getter_AddRefs(nodeList),
                             &isAnonymousContentList);

    if (nodeList && isAnonymousContentList) {
      
      nsAnonymousContentList* contentList = NS_STATIC_CAST(nsAnonymousContentList*, NS_STATIC_CAST(nsIDOMNodeList*, nodeList));
      PRInt32 count = contentList->GetInsertionPointCount();
      for (PRInt32 i =0; i < count; i++) {
        nsXBLInsertionPoint* point = contentList->GetInsertionPointAt(i);
        if (point->GetInsertionIndex() != -1) {
          point->RemoveChild(aChild);
        }
      }
    }
  }
}

void
nsBindingManager::NodeWillBeDestroyed(const nsINode *aNode)
{
  
  mProcessingAttachedStack = PR_TRUE;
  
  NS_BINDINGMANAGER_NOTIFY_OBSERVERS(NodeWillBeDestroyed, (aNode));
}

void
nsBindingManager::ParentChainChanged(nsIContent *aContent)
{
  NS_BINDINGMANAGER_NOTIFY_OBSERVERS(ParentChainChanged, (aContent));
}

void
nsBindingManager::Traverse(nsIContent *aContent,
                           nsCycleCollectionTraversalCallback &cb)
{
  nsXBLBinding *binding = GetBinding(aContent);
  if (binding) {
    
    
    cb.NoteXPCOMChild(aContent);
  }
  nsISupports *value;
  if (mContentListTable.ops &&
      (value = LookupObject(mContentListTable, aContent))) {
    cb.NoteXPCOMChild(aContent);
    cb.NoteXPCOMChild(value);
  }
  if (mAnonymousNodesTable.ops &&
      (value = LookupObject(mAnonymousNodesTable, aContent))) {
    cb.NoteXPCOMChild(aContent);
    cb.NoteXPCOMChild(value);
  }
  if (mInsertionParentTable.ops &&
      (value = LookupObject(mInsertionParentTable, aContent))) {
    cb.NoteXPCOMChild(aContent);
    cb.NoteXPCOMChild(value);
  }
  if (mWrapperTable.ops &&
      (value = LookupObject(mWrapperTable, aContent))) {
    cb.NoteXPCOMChild(aContent);
    cb.NoteXPCOMChild(value);
  }
}
