





#include "nsBindingManager.h"

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
#include "nsITimer.h"

#include "nsXBLBinding.h"
#include "nsXBLPrototypeBinding.h"
#include "nsXBLDocumentInfo.h"
#include "mozilla/dom/XBLChildrenElement.h"

#include "nsIStyleRuleProcessor.h"
#include "nsRuleProcessorData.h"
#include "nsIWeakReference.h"

#include "nsWrapperCacheInlines.h"
#include "nsIXPConnect.h"
#include "nsDOMCID.h"
#include "nsIDOMScriptObjectFactory.h"
#include "nsIScriptGlobalObject.h"
#include "nsTHashtable.h"

#include "nsIScriptContext.h"
#include "xpcpublic.h"
#include "jswrapper.h"

#include "nsThreadUtils.h"
#include "mozilla/dom/NodeListBinding.h"
#include "mozilla/dom/ScriptSettings.h"
#include "mozilla/unused.h"

using namespace mozilla;
using namespace mozilla::dom;



NS_IMPL_CYCLE_COLLECTION_CLASS(nsBindingManager)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsBindingManager)
  tmp->mDestroyed = true;

  if (tmp->mBoundContentSet)
    tmp->mBoundContentSet->Clear();

  if (tmp->mDocumentTable)
    tmp->mDocumentTable->Clear();

  if (tmp->mLoadingDocTable)
    tmp->mLoadingDocTable->Clear();

  if (tmp->mWrapperTable) {
    tmp->mWrapperTable->Clear();
    tmp->mWrapperTable = nullptr;
  }

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
  cb->NoteXPCOMChild(di);
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
  
  if (tmp->mDocumentTable)
      tmp->mDocumentTable->EnumerateRead(&DocumentInfoHashtableTraverser, &cb);
  if (tmp->mLoadingDocTable)
      tmp->mLoadingDocTable->EnumerateRead(&LoadingDocHashtableTraverser, &cb);
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
}

nsBindingManager::~nsBindingManager(void)
{
  mDestroyed = true;
}

nsXBLBinding*
nsBindingManager::GetBindingWithContent(nsIContent* aContent)
{
  nsXBLBinding* binding = aContent ? aContent->GetXBLBinding() : nullptr;
  return binding ? binding->GetBindingWithContent() : nullptr;
}

void
nsBindingManager::AddBoundContent(nsIContent* aContent)
{
  if (!mBoundContentSet) {
    mBoundContentSet = new nsTHashtable<nsRefPtrHashKey<nsIContent> >;
  }
  mBoundContentSet->PutEntry(aContent);
}

void
nsBindingManager::RemoveBoundContent(nsIContent* aContent)
{
  if (mBoundContentSet) {
    mBoundContentSet->RemoveEntry(aContent);
  }

  
  SetWrappedJS(aContent, nullptr);
}

nsIXPConnectWrappedJS*
nsBindingManager::GetWrappedJS(nsIContent* aContent)
{
  if (!mWrapperTable) {
    return nullptr;
  }

  if (!aContent || !aContent->HasFlag(NODE_MAY_BE_IN_BINDING_MNGR)) {
    return nullptr;
  }

  return mWrapperTable->GetWeak(aContent);
}

nsresult
nsBindingManager::SetWrappedJS(nsIContent* aContent, nsIXPConnectWrappedJS* aWrappedJS)
{
  if (mDestroyed) {
    return NS_OK;
  }

  if (aWrappedJS) {
    
    if (!mWrapperTable) {
      mWrapperTable = new WrapperHashtable();
    }
    aContent->SetFlags(NODE_MAY_BE_IN_BINDING_MNGR);

    NS_ASSERTION(aContent, "key must be non-null");
    if (!aContent) return NS_ERROR_INVALID_ARG;

    mWrapperTable->Put(aContent, aWrappedJS);

    return NS_OK;
  }

  
  if (mWrapperTable) {
    mWrapperTable->Remove(aContent);
  }

  return NS_OK;
}

void
nsBindingManager::RemovedFromDocumentInternal(nsIContent* aContent,
                                              nsIDocument* aOldDocument)
{
  NS_PRECONDITION(aOldDocument != nullptr, "no old document");

  nsRefPtr<nsXBLBinding> binding = aContent->GetXBLBinding();
  if (binding) {
    
    
    
    
    
    if (!mDestroyed) {
      binding->PrototypeBinding()->BindingDetached(binding->GetBoundElement());
      binding->ChangeDocument(aOldDocument, nullptr);
    }

    aContent->SetXBLBinding(nullptr, this);
  }

  
  aContent->SetXBLInsertionParent(nullptr);
}

nsIAtom*
nsBindingManager::ResolveTag(nsIContent* aContent, int32_t* aNameSpaceID)
{
  nsXBLBinding *binding = aContent->GetXBLBinding();

  if (binding) {
    nsIAtom* base = binding->GetBaseTag(aNameSpaceID);

    if (base) {
      return base;
    }
  }

  *aNameSpaceID = aContent->GetNameSpaceID();
  return aContent->NodeInfo()->NameAtom();
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
  
  nsRefPtr<nsXBLBinding> binding =
    aContent ? aContent->GetXBLBinding() : nullptr;

  if (!binding) {
    return NS_OK;
  }

  
  NS_ENSURE_FALSE(binding->GetBaseBinding(), NS_ERROR_FAILURE);

  
  
  
  
  
  nsCOMPtr<nsIDocument> doc = aContent->OwnerDoc();

  
  
  
  binding->UnhookEventHandlers();
  binding->ChangeDocument(doc, nullptr);
  aContent->SetXBLBinding(nullptr, this);
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

void
nsBindingManager::RemoveFromAttachedQueue(nsXBLBinding* aBinding)
{
  
  
  size_t index = mAttachedStack.IndexOf(aBinding);
  if (index != mAttachedStack.NoIndex) {
    mAttachedStack[index] = nullptr;
  }
}

nsresult
nsBindingManager::AddToAttachedQueue(nsXBLBinding* aBinding)
{
  mAttachedStack.AppendElement(aBinding);

  
  
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
nsBindingManager::PostPAQEventCallback(nsITimer* aTimer, void* aClosure)
{
  nsRefPtr<nsBindingManager> mgr = 
    already_AddRefed<nsBindingManager>(static_cast<nsBindingManager*>(aClosure));
  mgr->PostProcessAttachedQueueEvent();
  NS_RELEASE(aTimer);
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
    
    
    
    
    
    
    nsresult rv = NS_ERROR_FAILURE;
    nsCOMPtr<nsITimer> timer = do_CreateInstance(NS_TIMER_CONTRACTID);
    if (timer) {
      rv = timer->InitWithFuncCallback(PostPAQEventCallback, this,
                                       100, nsITimer::TYPE_ONE_SHOT);
    }
    if (NS_SUCCEEDED(rv)) {
      NS_ADDREF_THIS();
      
      
      unused << timer.forget().take();
    }
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
AccumulateBindingsToDetach(nsRefPtrHashKey<nsIContent> *aKey,
                           void* aClosure)
{
  nsXBLBinding *binding = aKey->GetKey()->GetXBLBinding();
  BindingTableReadClosure* closure =
    static_cast<BindingTableReadClosure*>(aClosure);
  if (binding && closure->mBindings.AppendElement(binding)) {
    if (!closure->mBoundElements.AppendObject(binding->GetBoundElement())) {
      closure->mBindings.RemoveElementAt(closure->mBindings.Length() - 1);
    }
  }
  return PL_DHASH_NEXT;
}

void
nsBindingManager::ExecuteDetachedHandlers()
{
  
  if (mBoundContentSet) {
    BindingTableReadClosure closure;
    mBoundContentSet->EnumerateEntries(AccumulateBindingsToDetach, &closure);
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

  if (!mDocumentTable) {
    mDocumentTable = new nsRefPtrHashtable<nsURIHashKey,nsXBLDocumentInfo>();
  }

  mDocumentTable->Put(aDocumentInfo->DocumentURI(), aDocumentInfo);

  return NS_OK;
}

void
nsBindingManager::RemoveXBLDocumentInfo(nsXBLDocumentInfo* aDocumentInfo)
{
  if (mDocumentTable) {
    mDocumentTable->Remove(aDocumentInfo->DocumentURI());
  }
}

nsXBLDocumentInfo*
nsBindingManager::GetXBLDocumentInfo(nsIURI* aURL)
{
  if (!mDocumentTable)
    return nullptr;

  return mDocumentTable->GetWeak(aURL);
}

nsresult
nsBindingManager::PutLoadingDocListener(nsIURI* aURL, nsIStreamListener* aListener)
{
  NS_PRECONDITION(aListener, "Must have a non-null listener!");

  if (!mLoadingDocTable) {
    mLoadingDocTable =
      new nsInterfaceHashtable<nsURIHashKey,nsIStreamListener>();
  }
  mLoadingDocTable->Put(aURL, aListener);

  return NS_OK;
}

nsIStreamListener*
nsBindingManager::GetLoadingDocListener(nsIURI* aURL)
{
  if (!mLoadingDocTable)
    return nullptr;

  return mLoadingDocTable->GetWeak(aURL);
}

void
nsBindingManager::RemoveLoadingDocListener(nsIURI* aURL)
{
  if (mLoadingDocTable) {
    mLoadingDocTable->Remove(aURL);
  }
}

static PLDHashOperator
MarkForDeath(nsRefPtrHashKey<nsIContent> *aKey, void* aClosure)
{
  nsXBLBinding *binding = aKey->GetKey()->GetXBLBinding();

  if (binding->MarkedForDeath())
    return PL_DHASH_NEXT; 

  nsAutoCString path;
  binding->PrototypeBinding()->DocURI()->GetPath(path);

  if (!strncmp(path.get(), "/skin", 5))
    binding->MarkForDeath();

  return PL_DHASH_NEXT;
}

void
nsBindingManager::FlushSkinBindings()
{
  if (mBoundContentSet) {
    mBoundContentSet->EnumerateEntries(MarkForDeath, nullptr);
  }
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
  nsXBLBinding *binding = aContent ? aContent->GetXBLBinding() : nullptr;
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

      
      
      AutoJSAPI jsapi;
      jsapi.Init();
      JSContext* cx = jsapi.cx();

      nsIXPConnect *xpConnect = nsContentUtils::XPConnect();

      JS::Rooted<JSObject*> jsobj(cx, aContent->GetWrapper());
      NS_ENSURE_TRUE(jsobj, NS_NOINTERFACE);

      
      
      
      
      
      
      
      
      JS::Rooted<JSObject*> xblScope(cx, xpc::GetXBLScopeOrGlobal(cx, jsobj));
      NS_ENSURE_TRUE(xblScope, NS_ERROR_UNEXPECTED);
      JSAutoCompartment ac(cx, xblScope);
      bool ok = JS_WrapObject(cx, &jsobj);
      NS_ENSURE_TRUE(ok, NS_ERROR_OUT_OF_MEMORY);
      MOZ_ASSERT_IF(js::IsWrapper(jsobj), xpc::IsXrayWrapper(jsobj));

      nsresult rv = xpConnect->WrapJSAggregatedToNative(aContent, cx,
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
    nsXBLBinding *binding = content->GetXBLBinding();
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
EnumRuleProcessors(nsRefPtrHashKey<nsIContent> *aKey, void* aClosure)
{
  nsIContent *boundContent = aKey->GetKey();
  nsAutoPtr<RuleProcessorSet> *set = static_cast<nsAutoPtr<RuleProcessorSet>*>(aClosure);
  for (nsXBLBinding *binding = boundContent->GetXBLBinding(); binding;
       binding = binding->GetBaseBinding()) {
    nsIStyleRuleProcessor *ruleProc =
      binding->PrototypeBinding()->GetRuleProcessor();
    if (ruleProc) {
      if (!(*set)) {
        *set = new RuleProcessorSet;
      }
      (*set)->PutEntry(ruleProc);
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
  if (!mBoundContentSet) {
    return;
  }

  nsAutoPtr<RuleProcessorSet> set;
  mBoundContentSet->EnumerateEntries(EnumRuleProcessors, &set);
  if (!set)
    return;

  WalkAllRulesData data = { aFunc, aData };
  set->EnumerateEntries(EnumWalkAllRules, &data);
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
  if (!mBoundContentSet) {
    return NS_OK;
  }

  nsAutoPtr<RuleProcessorSet> set;
  mBoundContentSet->EnumerateEntries(EnumRuleProcessors, &set);
  if (!set) {
    return NS_OK;
  }

  MediumFeaturesChangedData data = { aPresContext, aRulesChanged };
  set->EnumerateEntries(EnumMediumFeaturesChanged, &data);
  return NS_OK;
}

static PLDHashOperator
EnumAppendAllSheets(nsRefPtrHashKey<nsIContent> *aKey, void* aClosure)
{
  nsIContent *boundContent = aKey->GetKey();
  nsTArray<CSSStyleSheet*>* array =
    static_cast<nsTArray<CSSStyleSheet*>*>(aClosure);
  for (nsXBLBinding *binding = boundContent->GetXBLBinding(); binding;
       binding = binding->GetBaseBinding()) {
    binding->PrototypeBinding()->AppendStyleSheetsTo(*array);
  }
  return PL_DHASH_NEXT;
}

void
nsBindingManager::AppendAllSheets(nsTArray<CSSStyleSheet*>& aArray)
{
  if (mBoundContentSet) {
    mBoundContentSet->EnumerateEntries(EnumAppendAllSheets, &aArray);
  }
}

static void
InsertAppendedContent(XBLChildrenElement* aPoint,
                      nsIContent* aFirstNewContent)
{
  int32_t insertionIndex;
  if (nsIContent* prevSibling = aFirstNewContent->GetPreviousSibling()) {
    
    
    insertionIndex = aPoint->IndexOfInsertedChild(prevSibling);
    MOZ_ASSERT(insertionIndex != -1);

    
    ++insertionIndex;
  } else {
    
    
    
    insertionIndex = aPoint->InsertedChildrenLength();
  }

  
  for (nsIContent* currentChild = aFirstNewContent;
       currentChild;
       currentChild = currentChild->GetNextSibling()) {
    aPoint->InsertInsertedChildAt(currentChild, insertionIndex++);
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

  
  if (parent && parent->IsActiveChildrenElement()) {
    XBLChildrenElement* childrenEl = static_cast<XBLChildrenElement*>(parent);
    if (childrenEl->HasInsertedChildren()) {
      
      return;
    }

    childrenEl->MaybeSetupDefaultContent();
    parent = childrenEl->GetParent();
  }

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
        point->AppendInsertedChild(child);
      }
    } else {
      InsertAppendedContent(point, aFirstNewContent);
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
  aChild->SetXBLInsertionParent(nullptr);

  XBLChildrenElement* point = nullptr;
  nsIContent* parent = aContainer;

  
  if (parent && parent->IsActiveChildrenElement()) {
    XBLChildrenElement* childrenEl = static_cast<XBLChildrenElement*>(parent);
    if (childrenEl->HasInsertedChildren()) {
      
      return;
    }

    parent = childrenEl->GetParent();
  }

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
    static_cast<XBLChildrenElement*>(aContent)->ClearInsertedChildren();
  }

  for (nsIContent* child = aContent->GetFirstChild(); child;
       child = child->GetNextSibling()) {
    ClearInsertionPointsRecursively(child);
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

  if (mBoundContentSet) {
    mBoundContentSet->Clear();
  }

  mDocument = nullptr;
}

void
nsBindingManager::Traverse(nsIContent *aContent,
                           nsCycleCollectionTraversalCallback &cb)
{
  if (!aContent->HasFlag(NODE_MAY_BE_IN_BINDING_MNGR) ||
      !aContent->IsElement()) {
    
    
    
    
    
    return;
  }

  if (mBoundContentSet && mBoundContentSet->Contains(aContent)) {
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "[via binding manager] mBoundContentSet entry");
    cb.NoteXPCOMChild(aContent);
  }

  nsIXPConnectWrappedJS *value = GetWrappedJS(aContent);
  if (value) {
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

  
  if (parent && parent->IsActiveChildrenElement()) {
    XBLChildrenElement* childrenEl = static_cast<XBLChildrenElement*>(parent);
    if (childrenEl->HasInsertedChildren()) {
      
      return;
    }

    childrenEl->MaybeSetupDefaultContent();
    parent = childrenEl->GetParent();
  }

  while (parent) {
    nsXBLBinding* binding = GetBindingWithContent(parent);
    if (!binding) {
      break;
    }

    point = binding->FindInsertionPointFor(aChild);
    if (!point) {
      break;
    }

    
    
    
    
    uint32_t index = aAppend ? point->InsertedChildrenLength() : 0;
    for (nsIContent* currentSibling = aChild->GetPreviousSibling();
         currentSibling;
         currentSibling = currentSibling->GetPreviousSibling()) {
      
      
      
      int32_t pointIndex = point->IndexOfInsertedChild(currentSibling);
      if (pointIndex != -1) {
        index = pointIndex + 1;
        break;
      }
    }

    point->InsertInsertedChildAt(aChild, index);

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
  if (aContainer->IsActiveChildrenElement()) {
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
  if (aContainer->IsActiveChildrenElement()) {
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
