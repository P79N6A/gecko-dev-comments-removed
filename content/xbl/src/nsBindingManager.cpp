





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
#include "mozilla/dom/XMLDocument.h"
#include "nsIStreamListener.h"
#include "ChildIterator.h"

#include "nsXBLBinding.h"
#include "nsXBLPrototypeBinding.h"
#include "nsXBLDocumentInfo.h"
#include "mozilla/dom/XBLChildrenElement.h"

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
#include "nsCxPusher.h"

#include "nsThreadUtils.h"
#include "mozilla/dom/NodeListBinding.h"

using namespace mozilla;
using namespace mozilla::dom;






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
  mInsertionParentTable.ops = nullptr;
  mWrapperTable.ops = nullptr;
}

nsBindingManager::~nsBindingManager(void)
{
  mDestroyed = true;

  NS_ASSERTION(!mInsertionParentTable.ops || !mInsertionParentTable.entryCount,
               "Insertion parent table isn't empty!");
  if (mInsertionParentTable.ops)
    PL_DHashTableFinish(&mInsertionParentTable);
  if (mWrapperTable.ops)
    PL_DHashTableFinish(&mWrapperTable);
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

nsXBLBinding*
nsBindingManager::GetBindingWithContent(nsIContent* aContent)
{
  nsXBLBinding* binding = GetBinding(aContent);
  return binding ? binding->GetBindingWithContent() : nullptr;
}

nsresult
nsBindingManager::SetBinding(nsIContent* aContent, nsXBLBinding* aBinding)
{
  if (!mBindingTable.IsInitialized()) {
    mBindingTable.Init();
  }

  
  
  
  
  
  
  nsRefPtr<nsXBLBinding> oldBinding = GetBinding(aContent);
  if (oldBinding) {
    
    
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
                                              nsIDocument* aOldDocument)
{
  NS_PRECONDITION(aOldDocument != nullptr, "no old document");

  if (mDestroyed)
    return;

  nsRefPtr<nsXBLBinding> binding = GetBinding(aContent);
  if (binding) {
    binding->PrototypeBinding()->BindingDetached(binding->GetBoundElement());
    binding->ChangeDocument(aOldDocument, nullptr);
    SetBinding(aContent, nullptr);
  }

  
  SetInsertionParent(aContent, nullptr);
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
nsBindingManager::GetAnonymousNodesFor(nsIContent* aContent,
                                       nsIDOMNodeList** aResult)
{
  NS_IF_ADDREF(*aResult = GetAnonymousNodesFor(aContent));
  return NS_OK;
}

nsINodeList*
nsBindingManager::GetAnonymousNodesFor(nsIContent* aContent)
{
  nsXBLBinding* binding = GetBindingWithContent(aContent);
  return binding ? binding->GetAnonymousNodeList() : nullptr;
}

nsresult
nsBindingManager::ClearBinding(nsIContent* aContent)
{
  
  nsRefPtr<nsXBLBinding> binding = GetBinding(aContent);
  
  if (!binding) {
    return NS_OK;
  }

  
  NS_ENSURE_FALSE(binding->GetBaseBinding(), NS_ERROR_FAILURE);

  
  
  
  
  
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

      nsCOMPtr<nsIScriptGlobalObject> global =
        do_QueryInterface(doc->GetWindow());
      if (!global)
        return NS_NOINTERFACE;

      nsIScriptContext *context = global->GetContext();
      if (!context)
        return NS_NOINTERFACE;

      AutoPushJSContext jscontext(context->GetNativeContext());
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

static void
InsertAppendedContent(nsBindingManager* aManager,
                      XBLChildrenElement* aPoint,
                      nsIContent* aFirstNewContent)
{
  uint32_t insertionIndex;
  if (nsIContent* prevSibling = aFirstNewContent->GetPreviousSibling()) {
    
    
    insertionIndex = aPoint->IndexOfInsertedChild(prevSibling);
    MOZ_ASSERT(insertionIndex != aPoint->NoIndex);

    
    ++insertionIndex;
  } else {
    
    
    
    insertionIndex = aPoint->mInsertedChildren.Length();
  }

  
  for (nsIContent* currentChild = aFirstNewContent;
       currentChild;
       currentChild = currentChild->GetNextSibling()) {
    aPoint->InsertInsertedChildAt(currentChild, insertionIndex++, aManager);
  }
}

void
nsBindingManager::ContentAppended(nsIDocument* aDocument,
                                  nsIContent* aContainer,
                                  nsIContent* aFirstNewContent,
                                  int32_t     aNewIndexInContainer)
{
  if (aNewIndexInContainer == -1) {
    return;
  }

  
  XBLChildrenElement* point = nullptr;
  nsIContent* parent = aContainer;
  bool first = true;
  do {
    nsXBLBinding* binding = GetBindingWithContent(parent);
    if (!binding) {
      break;
    }

    if (binding->HasFilteredInsertionPoints()) {
      
      
      
      
      
      int32_t currentIndex = aNewIndexInContainer;
      for (nsIContent* currentChild = aFirstNewContent; currentChild;
           currentChild = currentChild->GetNextSibling()) {
        HandleChildInsertion(aContainer, currentChild,
                             currentIndex++, true);
      }

      return;
    }

    point = binding->GetDefaultInsertionPoint();
    if (!point) {
      break;
    }

    
    
    
    if (first) {
      first = false;
      for (nsIContent* child = aFirstNewContent; child;
           child = child->GetNextSibling()) {
        point->AppendInsertedChild(child, this);
      }
    } else {
      InsertAppendedContent(this, point, aFirstNewContent);
    }

    nsIContent* newParent = point->GetParent();
    if (newParent == parent) {
      break;
    }
    parent = newParent;
  } while (parent);
}

void
nsBindingManager::ContentInserted(nsIDocument* aDocument,
                                  nsIContent* aContainer,
                                  nsIContent* aChild,
                                  int32_t aIndexInContainer)
{
  if (aIndexInContainer == -1) {
    return;
  }

  HandleChildInsertion(aContainer, aChild, aIndexInContainer, false);
}

void
nsBindingManager::ContentRemoved(nsIDocument* aDocument,
                                 nsIContent* aContainer,
                                 nsIContent* aChild,
                                 int32_t aIndexInContainer,
                                 nsIContent* aPreviousSibling)
{
  SetInsertionParent(aChild, nullptr);

  XBLChildrenElement* point = nullptr;
  nsIContent* parent = aContainer;
  do {
    nsXBLBinding* binding = GetBindingWithContent(parent);
    if (!binding) {
      
      
      
      
      
      
      
      if (aChild->GetBindingParent()) {
        ClearInsertionPointsRecursively(aChild);
      }
      return;
    }

    point = binding->FindInsertionPointFor(aChild);
    if (!point) {
      break;
    }

    point->RemoveInsertedChild(aChild);

    nsIContent* newParent = point->GetParent();
    if (newParent == parent) {
      break;
    }
    parent = newParent;
  } while (parent);
}

void
nsBindingManager::ClearInsertionPointsRecursively(nsIContent* aContent)
{
  if (aContent->NodeInfo()->Equals(nsGkAtoms::children, kNameSpaceID_XBL)) {
    static_cast<XBLChildrenElement*>(aContent)->ClearInsertedChildrenAndInsertionParents(this);
  }

  uint32_t childCount = aContent->GetChildCount();
  for (uint32_t c = 0; c < childCount; c++) {
    ClearInsertionPointsRecursively(aContent->GetChildAt(c));
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

  XBLChildrenElement* point = nullptr;
  nsIContent* parent = aContainer;
  while (parent) {
    nsXBLBinding* binding = GetBindingWithContent(parent);
    if (!binding) {
      break;
    }

    point = binding->FindInsertionPointFor(aChild);
    if (!point) {
      break;
    }

    
    
    
    
    uint32_t index = aAppend ? point->mInsertedChildren.Length() : 0;
    for (nsIContent* currentSibling = aChild->GetPreviousSibling();
         currentSibling;
         currentSibling = currentSibling->GetPreviousSibling()) {
      
      
      
      uint32_t pointIndex = point->IndexOfInsertedChild(currentSibling);
      if (pointIndex != point->NoIndex) {
        index = pointIndex + 1;
        break;
      }
    }

    point->InsertInsertedChildAt(aChild, index, this);

    nsIContent* newParent = point->GetParent();
    if (newParent == parent) {
      break;
    }

    parent = newParent;
  }
}


nsIContent*
nsBindingManager::FindNestedInsertionPoint(nsIContent* aContainer,
                                           nsIContent* aChild)
{
  NS_PRECONDITION(aChild->GetParent() == aContainer,
                  "Wrong container");

  nsIContent* parent = aContainer;
  if (aContainer->NodeInfo()->Equals(nsGkAtoms::children, kNameSpaceID_XBL)) {
    if (static_cast<XBLChildrenElement*>(aContainer)->
          HasInsertedChildren()) {
      return nullptr;
    }
    parent = aContainer->GetParent();
  }

  while (parent) {
    nsXBLBinding* binding = GetBindingWithContent(parent);
    if (!binding) {
      break;
    }
  
    XBLChildrenElement* point = binding->FindInsertionPointFor(aChild);
    if (!point) {
      return nullptr;
    }

    nsIContent* newParent = point->GetParent();
    if (newParent == parent) {
      break;
    }
    parent = newParent;
  }

  return parent;
}

nsIContent*
nsBindingManager::FindNestedSingleInsertionPoint(nsIContent* aContainer,
                                                 bool* aMulti)
{
  *aMulti = false;

  nsIContent* parent = aContainer;
  if (aContainer->NodeInfo()->Equals(nsGkAtoms::children, kNameSpaceID_XBL)) {
    if (static_cast<XBLChildrenElement*>(aContainer)->
          HasInsertedChildren()) {
      return nullptr;
    }
    parent = aContainer->GetParent();
  }

  while(parent) {
    nsXBLBinding* binding = GetBindingWithContent(parent);
    if (!binding) {
      break;
    }

    if (binding->HasFilteredInsertionPoints()) {
      *aMulti = true;
      return nullptr;
    }

    XBLChildrenElement* point = binding->GetDefaultInsertionPoint();
    if (!point) {
      return nullptr;
    }

    nsIContent* newParent = point->GetParent();
    if (newParent == parent) {
      break;
    }
    parent = newParent;
  }

  return parent;
}
