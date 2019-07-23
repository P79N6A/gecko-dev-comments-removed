















































#include "plstr.h"
#include "prprf.h"

#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsDocument.h"
#include "nsUnicharUtils.h"
#include "nsIPrivateDOMEvent.h"
#include "nsIEventStateManager.h"
#include "nsIFocusController.h"
#include "nsContentList.h"
#include "nsIObserver.h"
#include "nsIBaseWindow.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"
#include "nsIScriptRuntime.h"
#include "nsCOMArray.h"

#include "nsGUIEvent.h"

#include "nsIDOMStyleSheet.h"
#include "nsDOMAttribute.h"
#include "nsIDOMDOMStringList.h"
#include "nsIDOMDOMImplementation.h"
#include "nsIDOMDocumentView.h"
#include "nsIDOMAbstractView.h"
#include "nsIDOMDocumentXBL.h"
#include "nsGenericElement.h"
#include "nsGenericHTMLElement.h"
#include "nsIDOMEventGroup.h"
#include "nsIDOMCDATASection.h"
#include "nsIDOMProcessingInstruction.h"
#include "nsDOMString.h"
#include "nsNodeUtils.h"
#include "nsLayoutUtils.h" 
#include "nsIFrame.h"

#include "nsRange.h"
#include "nsIDOMText.h"
#include "nsIDOMComment.h"
#include "nsDOMDocumentType.h"
#include "nsNodeIterator.h"
#include "nsTreeWalker.h"

#include "nsIServiceManager.h"

#include "nsContentCID.h"
#include "nsDOMError.h"
#include "nsIPresShell.h"
#include "nsPresContext.h"
#include "nsThreadUtils.h"
#include "nsNodeInfoManager.h"
#include "nsIXBLService.h"
#include "nsIXPointer.h"
#include "nsIFileChannel.h"
#include "nsIMultiPartChannel.h"
#include "nsIRefreshURI.h"
#include "nsIWebNavigation.h"
#include "nsIScriptError.h"

#include "nsNetUtil.h"     

#include "nsIScriptSecurityManager.h"
#include "nsIPrincipal.h"
#include "nsIPrivateDOMImplementation.h"

#include "nsIDOMWindowInternal.h"
#include "nsPIDOMWindow.h"
#include "nsIDOMElement.h"
#include "nsFocusManager.h"


#include "nsIDOMHTMLInputElement.h"
#include "nsIRadioVisitor.h"
#include "nsIFormControl.h"

#include "nsXMLEventsManager.h"

#include "nsBidiUtils.h"

static NS_DEFINE_CID(kDOMEventGroupCID, NS_DOMEVENTGROUP_CID);

#include "nsIDOMUserDataHandler.h"
#include "nsScriptEventManager.h"
#include "nsIDOMXPathEvaluator.h"
#include "nsIXPathEvaluatorInternal.h"
#include "nsIParserService.h"
#include "nsContentCreatorFunctions.h"

#include "nsIScriptContext.h"
#include "nsBindingManager.h"
#include "nsIDOMHTMLDocument.h"
#include "nsIDOMHTMLFormElement.h"
#include "nsIRequest.h"
#include "nsILink.h"

#include "nsICharsetAlias.h"
#include "nsIParser.h"
#include "nsIContentSink.h"

#include "nsDateTimeFormatCID.h"
#include "nsIDateTimeFormat.h"
#include "nsEventDispatcher.h"
#include "nsMutationEvent.h"
#include "nsIDOMXPathEvaluator.h"
#include "nsDOMCID.h"

#include "jsapi.h"
#include "nsIJSContextStack.h"
#include "nsIXPConnect.h"
#include "nsCycleCollector.h"
#include "nsCCUncollectableMarker.h"
#include "nsIContentPolicy.h"
#include "nsContentPolicyUtils.h"
#include "nsICategoryManager.h"
#include "nsIDocumentLoaderFactory.h"
#include "nsIContentViewer.h"
#include "nsIXMLContentSink.h"
#include "nsContentErrors.h"
#include "nsIXULDocument.h"
#include "nsIPrompt.h"
#include "nsIPropertyBag2.h"
#include "nsIDOMPageTransitionEvent.h"
#include "nsFrameLoader.h"

#include "mozAutoDocUpdate.h"

#ifdef MOZ_SMIL
#include "nsSMILAnimationController.h"
#include "imgIContainer.h"
#include "nsSVGUtils.h"
#endif 


#ifdef MOZ_LOGGING

#define FORCE_PR_LOG 1
#endif
#include "prlog.h"

#ifdef PR_LOGGING
static PRLogModuleInfo* gDocumentLeakPRLog;
#endif

void
nsUint32ToContentHashEntry::Destroy()
{
  HashSet* set = GetHashSet();
  if (set) {
    delete set;
  } else {
    nsIContent* content = GetContent();
    NS_IF_RELEASE(content);
  }
}

nsresult
nsUint32ToContentHashEntry::PutContent(nsIContent* aVal)
{
  
  HashSet* set = GetHashSet();
  if (set) {
    nsISupportsHashKey* entry = set->PutEntry(aVal);
    return entry ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
  }

  
  nsIContent* oldVal = GetContent();
  if (oldVal) {
    nsresult rv = InitHashSet(&set);
    NS_ENSURE_SUCCESS(rv, rv);

    nsISupportsHashKey* entry = set->PutEntry(oldVal);
    if (!entry) {
      
      
      delete set;
      SetContent(oldVal);
      
      NS_RELEASE(oldVal);
      return NS_ERROR_OUT_OF_MEMORY;
    }
    
    NS_RELEASE(oldVal);

    entry = set->PutEntry(aVal);
    return entry ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
  }

  
  return SetContent(aVal);
}

void
nsUint32ToContentHashEntry::RemoveContent(nsIContent* aVal)
{
  
  HashSet* set = GetHashSet();
  if (set) {
    set->RemoveEntry(aVal);
    if (set->Count() == 0) {
      delete set;
      mValOrHash = nsnull;
    }
    return;
  }

  
  nsIContent* v = GetContent();
  if (v == aVal) {
    NS_IF_RELEASE(v);
    mValOrHash = nsnull;
  }
}

nsresult
nsUint32ToContentHashEntry::InitHashSet(HashSet** aSet)
{
  HashSet* newSet = new HashSet();
  if (!newSet) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  nsresult rv = newSet->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  mValOrHash = newSet;
  *aSet = newSet;
  return NS_OK;
}

static PLDHashOperator
nsUint32ToContentHashEntryVisitorCallback(nsISupportsHashKey* aEntry,
                                          void* aClosure)
{
  nsUint32ToContentHashEntry::Visitor* visitor =
    static_cast<nsUint32ToContentHashEntry::Visitor*>(aClosure);
  visitor->Visit(static_cast<nsIContent*>(aEntry->GetKey()));
  return PL_DHASH_NEXT;
}

void
nsUint32ToContentHashEntry::VisitContent(Visitor* aVisitor)
{
  HashSet* set = GetHashSet();
  if (set) {
    set->EnumerateEntries(nsUint32ToContentHashEntryVisitorCallback, aVisitor);
    if (set->Count() == 0) {
      delete set;
      mValOrHash = nsnull;
    }
    return;
  }

  nsIContent* v = GetContent();
  if (v) {
    aVisitor->Visit(v);
  }
}

#define NAME_NOT_VALID ((nsBaseContentList*)1)

nsIdentifierMapEntry::~nsIdentifierMapEntry()
{
  if (mNameContentList && mNameContentList != NAME_NOT_VALID) {
    NS_RELEASE(mNameContentList);
  }
}

void
nsIdentifierMapEntry::Traverse(nsCycleCollectionTraversalCallback* aCallback)
{
  if (mNameContentList != NAME_NOT_VALID) {
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(*aCallback,
                                       "mIdentifierMap mNameContentList");
    aCallback->NoteXPCOMChild(static_cast<nsIDOMNodeList*>(mNameContentList));
  }

  NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(*aCallback, "mIdentifierMap mDocAllList");
  aCallback->NoteXPCOMChild(static_cast<nsIDOMNodeList*>(mDocAllList));
}

void
nsIdentifierMapEntry::SetInvalidName()
{
  mNameContentList = NAME_NOT_VALID;
}

PRBool
nsIdentifierMapEntry::IsInvalidName()
{
  return mNameContentList == NAME_NOT_VALID;
}

nsresult
nsIdentifierMapEntry::CreateNameContentList()
{
  mNameContentList = new nsBaseContentList();
  NS_ENSURE_TRUE(mNameContentList, NS_ERROR_OUT_OF_MEMORY);
  NS_ADDREF(mNameContentList);
  return NS_OK;
}

nsIContent*
nsIdentifierMapEntry::GetIdContent()
{
  return static_cast<nsIContent*>(mIdContentList.SafeElementAt(0));
}

void
nsIdentifierMapEntry::AppendAllIdContent(nsCOMArray<nsIContent>* aElements)
{
  for (PRInt32 i = 0; i < mIdContentList.Count(); ++i) {
    aElements->AppendObject(static_cast<nsIContent*>(mIdContentList[i]));
  }
}

void
nsIdentifierMapEntry::AddContentChangeCallback(nsIDocument::IDTargetObserver aCallback,
                                               void* aData)
{
  if (!mChangeCallbacks) {
    mChangeCallbacks = new nsTHashtable<ChangeCallbackEntry>;
    if (!mChangeCallbacks)
      return;
    mChangeCallbacks->Init();
  }

  ChangeCallback cc = { aCallback, aData };
  mChangeCallbacks->PutEntry(cc);
}

void
nsIdentifierMapEntry::RemoveContentChangeCallback(nsIDocument::IDTargetObserver aCallback,
                                                  void* aData)
{
  if (!mChangeCallbacks)
    return;
  ChangeCallback cc = { aCallback, aData };
  mChangeCallbacks->RemoveEntry(cc);
  if (mChangeCallbacks->Count() == 0) {
    mChangeCallbacks = nsnull;
  }
}

struct FireChangeArgs {
  nsIContent* mFrom;
  nsIContent* mTo;
};

static PLDHashOperator
FireChangeEnumerator(nsIdentifierMapEntry::ChangeCallbackEntry *aEntry, void *aArg)
{
  FireChangeArgs* args = static_cast<FireChangeArgs*>(aArg);
  return aEntry->mKey.mCallback(args->mFrom, args->mTo, aEntry->mKey.mData)
      ? PL_DHASH_NEXT : PL_DHASH_REMOVE;
}

void
nsIdentifierMapEntry::FireChangeCallbacks(nsIContent* aOldContent,
                                          nsIContent* aNewContent)
{
  if (!mChangeCallbacks)
    return;

  FireChangeArgs args = { aOldContent, aNewContent };
  mChangeCallbacks->EnumerateEntries(FireChangeEnumerator, &args);
}

PRBool
nsIdentifierMapEntry::AddIdContent(nsIContent* aContent)
{
  NS_PRECONDITION(aContent, "Must have content");
  NS_PRECONDITION(mIdContentList.IndexOf(nsnull) < 0,
                  "Why is null in our list?");

#ifdef DEBUG
  nsIContent* currentContent =
    static_cast<nsIContent*>(mIdContentList.SafeElementAt(0));
#endif

  
  if (mIdContentList.Count() == 0) {
    if (!mIdContentList.AppendElement(aContent))
      return PR_FALSE;
    NS_ASSERTION(currentContent == nsnull, "How did that happen?");
    FireChangeCallbacks(nsnull, aContent);
    return PR_TRUE;
  }

  
  
  PRInt32 start = 0;
  PRInt32 end = mIdContentList.Count();
  do {
    NS_ASSERTION(start < end, "Bogus start/end");
    
    PRInt32 cur = (start + end) / 2;
    NS_ASSERTION(cur >= start && cur < end, "What happened here?");

    nsIContent* curContent = static_cast<nsIContent*>(mIdContentList[cur]);
    if (curContent == aContent) {
      
      
      
      return PR_TRUE;
    }

    if (nsContentUtils::PositionIsBefore(aContent, curContent)) {
      end = cur;
    } else {
      start = cur + 1;
    }
  } while (start != end);

  if (!mIdContentList.InsertElementAt(aContent, start))
    return PR_FALSE;
  if (start == 0) {
    nsIContent* oldContent =
      static_cast<nsIContent*>(mIdContentList.SafeElementAt(1));
    NS_ASSERTION(currentContent == oldContent, "How did that happen?");
    FireChangeCallbacks(oldContent, aContent);
  }
  return PR_TRUE;
}

PRBool
nsIdentifierMapEntry::RemoveIdContent(nsIContent* aContent)
{
  
  

  
  
  nsIContent* currentContent = static_cast<nsIContent*>(mIdContentList.SafeElementAt(0));
  if (!mIdContentList.RemoveElement(aContent))
    return PR_FALSE;
  if (currentContent == aContent) {
    FireChangeCallbacks(currentContent,
                        static_cast<nsIContent*>(mIdContentList.SafeElementAt(0)));
  }
  return mIdContentList.Count() == 0 && !mNameContentList && !mChangeCallbacks;
}

void
nsIdentifierMapEntry::AddNameContent(nsIContent* aContent)
{
  if (!mNameContentList || mNameContentList == NAME_NOT_VALID)
    return;

  
  
  
  
  if (mNameContentList->IndexOf(aContent, PR_FALSE) < 0) {
    mNameContentList->AppendElement(aContent);
  }
}

void
nsIdentifierMapEntry::RemoveNameContent(nsIContent* aContent)
{
  if (mNameContentList && mNameContentList != NAME_NOT_VALID) {
    mNameContentList->RemoveElement(aContent);
  }
}



class SubDocMapEntry : public PLDHashEntryHdr
{
public:
  
  nsIContent *mKey; 
  nsIDocument *mSubDocument;
};

struct FindContentData
{
  FindContentData(nsIDocument *aSubDoc)
    : mSubDocument(aSubDoc), mResult(nsnull)
  {
  }

  nsISupports *mSubDocument;
  nsIContent *mResult;
};





struct nsRadioGroupStruct
{
  


  nsCOMPtr<nsIDOMHTMLInputElement> mSelectedRadioButton;
  nsCOMArray<nsIFormControl> mRadioButtons;
};


nsDOMStyleSheetList::nsDOMStyleSheetList(nsIDocument *aDocument)
{
  mLength = -1;
  
  
  mDocument = aDocument;
  mDocument->AddObserver(this);
}

nsDOMStyleSheetList::~nsDOMStyleSheetList()
{
  if (mDocument) {
    mDocument->RemoveObserver(this);
  }
}



NS_INTERFACE_TABLE_HEAD(nsDOMStyleSheetList)
  NS_INTERFACE_TABLE3(nsDOMStyleSheetList,
                      nsIDOMStyleSheetList,
                      nsIDocumentObserver,
                      nsIMutationObserver)
  NS_INTERFACE_TABLE_TO_MAP_SEGUE
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(StyleSheetList)
NS_INTERFACE_MAP_END


NS_IMPL_ADDREF(nsDOMStyleSheetList)
NS_IMPL_RELEASE(nsDOMStyleSheetList)


NS_IMETHODIMP
nsDOMStyleSheetList::GetLength(PRUint32* aLength)
{
  if (mDocument) {
    
    
    
    if (-1 == mLength) {
      mLength = mDocument->GetNumberOfStyleSheets();

#ifdef DEBUG
      PRInt32 i;
      for (i = 0; i < mLength; i++) {
        nsIStyleSheet *sheet = mDocument->GetStyleSheetAt(i);
        nsCOMPtr<nsIDOMStyleSheet> domss(do_QueryInterface(sheet));
        NS_ASSERTION(domss, "All \"normal\" sheets implement nsIDOMStyleSheet");
      }
#endif
    }
    *aLength = mLength;
  }
  else {
    *aLength = 0;
  }

  return NS_OK;
}

nsIStyleSheet*
nsDOMStyleSheetList::GetItemAt(PRUint32 aIndex)
{
  if (!mDocument || aIndex >= (PRUint32)mDocument->GetNumberOfStyleSheets()) {
    return nsnull;
  }

  nsIStyleSheet *sheet = mDocument->GetStyleSheetAt(aIndex);
  NS_ASSERTION(sheet, "Must have a sheet");

  return sheet;
}

NS_IMETHODIMP
nsDOMStyleSheetList::Item(PRUint32 aIndex, nsIDOMStyleSheet** aReturn)
{
  nsIStyleSheet *sheet = GetItemAt(aIndex);
  if (!sheet) {
      *aReturn = nsnull;

      return NS_OK;
  }

  return CallQueryInterface(sheet, aReturn);
}

void
nsDOMStyleSheetList::NodeWillBeDestroyed(const nsINode *aNode)
{
  mDocument = nsnull;
}

void
nsDOMStyleSheetList::StyleSheetAdded(nsIDocument *aDocument,
                                     nsIStyleSheet* aStyleSheet,
                                     PRBool aDocumentSheet)
{
  if (aDocumentSheet && -1 != mLength) {
    nsCOMPtr<nsIDOMStyleSheet> domss(do_QueryInterface(aStyleSheet));
    if (domss) {
      mLength++;
    }
  }
}

void
nsDOMStyleSheetList::StyleSheetRemoved(nsIDocument *aDocument,
                                       nsIStyleSheet* aStyleSheet,
                                       PRBool aDocumentSheet)
{
  if (aDocumentSheet && -1 != mLength) {
    nsCOMPtr<nsIDOMStyleSheet> domss(do_QueryInterface(aStyleSheet));
    if (domss) {
      mLength--;
    }
  }
}


NS_IMPL_ISUPPORTS1(nsOnloadBlocker, nsIRequest)

NS_IMETHODIMP
nsOnloadBlocker::GetName(nsACString &aResult)
{ 
  aResult.AssignLiteral("about:document-onload-blocker");
  return NS_OK;
}

NS_IMETHODIMP
nsOnloadBlocker::IsPending(PRBool *_retval)
{
  *_retval = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
nsOnloadBlocker::GetStatus(nsresult *status)
{
  *status = NS_OK;
  return NS_OK;
} 

NS_IMETHODIMP
nsOnloadBlocker::Cancel(nsresult status)
{
  return NS_OK;
}
NS_IMETHODIMP
nsOnloadBlocker::Suspend(void)
{
  return NS_OK;
}
NS_IMETHODIMP
nsOnloadBlocker::Resume(void)
{
  return NS_OK;
}

NS_IMETHODIMP
nsOnloadBlocker::GetLoadGroup(nsILoadGroup * *aLoadGroup)
{
  *aLoadGroup = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsOnloadBlocker::SetLoadGroup(nsILoadGroup * aLoadGroup)
{
  return NS_OK;
}

NS_IMETHODIMP
nsOnloadBlocker::GetLoadFlags(nsLoadFlags *aLoadFlags)
{
  *aLoadFlags = nsIRequest::LOAD_NORMAL;
  return NS_OK;
}

NS_IMETHODIMP
nsOnloadBlocker::SetLoadFlags(nsLoadFlags aLoadFlags)
{
  return NS_OK;
}



nsExternalResourceMap::nsExternalResourceMap()
  : mHaveShutDown(PR_FALSE)
{
  mMap.Init();
  mPendingLoads.Init();
}

nsIDocument*
nsExternalResourceMap::RequestResource(nsIURI* aURI,
                                       nsINode* aRequestingNode,
                                       nsDocument* aDisplayDocument,
                                       ExternalResourceLoad** aPendingLoad)
{
  
  
  
  NS_PRECONDITION(aURI, "Must have a URI");
  NS_PRECONDITION(aRequestingNode, "Must have a node");
  *aPendingLoad = nsnull;
  if (mHaveShutDown) {
    return nsnull;
  }
  
  
  nsCOMPtr<nsIURI> clone;
  aURI->Clone(getter_AddRefs(clone));
  if (!clone) {
    return nsnull;
  }
  nsCOMPtr<nsIURL> url(do_QueryInterface(clone));
  if (url) {
    url->SetRef(EmptyCString());
  }
  
  ExternalResource* resource;
  mMap.Get(clone, &resource);
  if (resource) {
    return resource->mDocument;
  }

  nsRefPtr<PendingLoad> load;
  mPendingLoads.Get(clone, getter_AddRefs(load));
  if (load) {
    NS_ADDREF(*aPendingLoad = load);
    return nsnull;
  }

  load = new PendingLoad(aDisplayDocument);
  if (!load) {
    return nsnull;
  }

  if (!mPendingLoads.Put(clone, load)) {
    return nsnull;
  }

  if (NS_FAILED(load->StartLoad(clone, aRequestingNode))) {
    
    
    AddExternalResource(clone, nsnull, nsnull, aDisplayDocument);
  } else {
    NS_ADDREF(*aPendingLoad = load);
  }

  return nsnull;
}

struct
nsExternalResourceEnumArgs
{
  nsIDocument::nsSubDocEnumFunc callback;
  void *data;
};

static PLDHashOperator
ExternalResourceEnumerator(nsIURI* aKey,
                           nsExternalResourceMap::ExternalResource* aData,
                           void* aClosure)
{
  nsExternalResourceEnumArgs* args =
    static_cast<nsExternalResourceEnumArgs*>(aClosure);
  PRBool next =
    aData->mDocument ? args->callback(aData->mDocument, args->data) : PR_TRUE;
  return next ? PL_DHASH_NEXT : PL_DHASH_STOP;
}

void
nsExternalResourceMap::EnumerateResources(nsIDocument::nsSubDocEnumFunc aCallback,
                                          void* aData)
{
  nsExternalResourceEnumArgs args = { aCallback, aData };
  mMap.EnumerateRead(ExternalResourceEnumerator, &args);
}

static PLDHashOperator
ExternalResourceTraverser(nsIURI* aKey,
                          nsExternalResourceMap::ExternalResource* aData,
                          void* aClosure)
{
  nsCycleCollectionTraversalCallback *cb = 
    static_cast<nsCycleCollectionTraversalCallback*>(aClosure);

  NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(*cb,
                                     "mExternalResourceMap.mMap entry"
                                     "->mDocument");
  cb->NoteXPCOMChild(aData->mDocument);

  NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(*cb,
                                     "mExternalResourceMap.mMap entry"
                                     "->mViewer");
  cb->NoteXPCOMChild(aData->mViewer);

  NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(*cb,
                                     "mExternalResourceMap.mMap entry"
                                     "->mLoadGroup");
  cb->NoteXPCOMChild(aData->mLoadGroup);

  return PL_DHASH_NEXT;
}

void
nsExternalResourceMap::Traverse(nsCycleCollectionTraversalCallback* aCallback) const
{
  
  
  mMap.EnumerateRead(ExternalResourceTraverser, aCallback);
}

nsresult
nsExternalResourceMap::AddExternalResource(nsIURI* aURI,
                                           nsIDocumentViewer* aViewer,
                                           nsILoadGroup* aLoadGroup,
                                           nsIDocument* aDisplayDocument)
{
  NS_PRECONDITION(aURI, "Unexpected call");
  NS_PRECONDITION((aViewer && aLoadGroup) || (!aViewer && !aLoadGroup),
                  "Must have both or neither");
  
  nsRefPtr<PendingLoad> load;
  mPendingLoads.Get(aURI, getter_AddRefs(load));
  mPendingLoads.Remove(aURI);

  nsresult rv = NS_OK;
  
  nsCOMPtr<nsIDocument> doc;
  if (aViewer) {
    aViewer->GetDocument(getter_AddRefs(doc));
    NS_ASSERTION(doc, "Must have a document");

    nsCOMPtr<nsIXULDocument> xulDoc = do_QueryInterface(doc);
    if (xulDoc) {
      
      rv = NS_ERROR_NOT_AVAILABLE;
    } else {
      doc->SetDisplayDocument(aDisplayDocument);

      rv = aViewer->Init(nsnull, nsIntRect(0, 0, 0, 0));
      if (NS_SUCCEEDED(rv)) {
        rv = aViewer->Open(nsnull, nsnull);
      }
    }
    
    if (NS_FAILED(rv)) {
      doc = nsnull;
      aViewer = nsnull;
      aLoadGroup = nsnull;
    }
  }

  ExternalResource* newResource = new ExternalResource();
  if (newResource && !mMap.Put(aURI, newResource)) {
    delete newResource;
    newResource = nsnull;
    if (NS_SUCCEEDED(rv)) {
      rv = NS_ERROR_OUT_OF_MEMORY;
    }
  }

  if (newResource) {
    newResource->mDocument = doc;
    newResource->mViewer = aViewer;
    newResource->mLoadGroup = aLoadGroup;
  }

  const nsTArray< nsCOMPtr<nsIObserver> > & obs = load->Observers();
  for (PRUint32 i = 0; i < obs.Length(); ++i) {
    obs[i]->Observe(doc, "external-resource-document-created", nsnull);
  }

  return rv;
}

NS_IMPL_ISUPPORTS2(nsExternalResourceMap::PendingLoad,
                   nsIStreamListener,
                   nsIRequestObserver)

NS_IMETHODIMP
nsExternalResourceMap::PendingLoad::OnStartRequest(nsIRequest *aRequest,
                                                   nsISupports *aContext)
{
  nsExternalResourceMap& map = mDisplayDocument->ExternalResourceMap();
  if (map.HaveShutDown()) {
    return NS_BINDING_ABORTED;
  }

  nsCOMPtr<nsIDocumentViewer> viewer;
  nsCOMPtr<nsILoadGroup> loadGroup;
  nsresult rv = SetupViewer(aRequest, getter_AddRefs(viewer),
                            getter_AddRefs(loadGroup));

  
  nsresult rv2 = map.AddExternalResource(mURI, viewer, loadGroup,
                                         mDisplayDocument);
  if (NS_FAILED(rv)) {
    return rv;
  }
  if (NS_FAILED(rv2)) {
    mTargetListener = nsnull;
    return rv2;
  }
  
  return mTargetListener->OnStartRequest(aRequest, aContext);
}

nsresult
nsExternalResourceMap::PendingLoad::SetupViewer(nsIRequest* aRequest,
                                                nsIDocumentViewer** aViewer,
                                                nsILoadGroup** aLoadGroup)
{
  NS_PRECONDITION(!mTargetListener, "Unexpected call to OnStartRequest");
  *aViewer = nsnull;
  *aLoadGroup = nsnull;
  
  nsCOMPtr<nsIChannel> chan(do_QueryInterface(aRequest));
  NS_ENSURE_TRUE(chan, NS_ERROR_UNEXPECTED);

  nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(aRequest));
  if (httpChannel) {
    PRBool requestSucceeded;
    if (NS_FAILED(httpChannel->GetRequestSucceeded(&requestSucceeded)) ||
        !requestSucceeded) {
      
      return NS_BINDING_ABORTED;
    }
  }
 
  nsCAutoString type;
  chan->GetContentType(type);

  nsCOMPtr<nsILoadGroup> loadGroup;
  chan->GetLoadGroup(getter_AddRefs(loadGroup));

  
  nsCOMPtr<nsILoadGroup> newLoadGroup =
        do_CreateInstance(NS_LOADGROUP_CONTRACTID);
  NS_ENSURE_TRUE(newLoadGroup, NS_ERROR_OUT_OF_MEMORY);
  newLoadGroup->SetLoadGroup(loadGroup);

  nsCOMPtr<nsIInterfaceRequestor> callbacks;
  loadGroup->GetNotificationCallbacks(getter_AddRefs(callbacks));

  nsCOMPtr<nsIInterfaceRequestor> newCallbacks =
    new LoadgroupCallbacks(callbacks);
  newLoadGroup->SetNotificationCallbacks(newCallbacks);

  
  nsCOMPtr<nsICategoryManager> catMan =
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID);
  NS_ENSURE_TRUE(catMan, NS_ERROR_NOT_AVAILABLE);
  nsXPIDLCString contractId;
  nsresult rv = catMan->GetCategoryEntry("Gecko-Content-Viewers", type.get(),
                                         getter_Copies(contractId));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIDocumentLoaderFactory> docLoaderFactory =
    do_GetService(contractId);
  NS_ENSURE_TRUE(docLoaderFactory, NS_ERROR_NOT_AVAILABLE);

  nsCOMPtr<nsIContentViewer> viewer;
  nsCOMPtr<nsIStreamListener> listener;
  rv = docLoaderFactory->CreateInstance("external-resource", chan, newLoadGroup,
                                        type.get(), nsnull, nsnull,
                                        getter_AddRefs(listener),
                                        getter_AddRefs(viewer));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDocumentViewer> docViewer = do_QueryInterface(viewer);
  NS_ENSURE_TRUE(docViewer, NS_ERROR_UNEXPECTED);

  nsCOMPtr<nsIParser> parser = do_QueryInterface(listener);
  if (!parser) {
    
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  
  nsIContentSink* sink = parser->GetContentSink();
  nsCOMPtr<nsIXMLContentSink> xmlSink = do_QueryInterface(sink);
  if (!xmlSink) {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  listener.swap(mTargetListener);
  docViewer.swap(*aViewer);
  newLoadGroup.swap(*aLoadGroup);
  return NS_OK;
}

NS_IMETHODIMP
nsExternalResourceMap::PendingLoad::OnDataAvailable(nsIRequest* aRequest,
                                                    nsISupports* aContext,
                                                    nsIInputStream* aStream,
                                                    PRUint32 aOffset,
                                                    PRUint32 aCount)
{
  NS_PRECONDITION(mTargetListener, "Shouldn't be getting called!");
  if (mDisplayDocument->ExternalResourceMap().HaveShutDown()) {
    return NS_BINDING_ABORTED;
  }
  return mTargetListener->OnDataAvailable(aRequest, aContext, aStream, aOffset,
                                          aCount);
}

NS_IMETHODIMP
nsExternalResourceMap::PendingLoad::OnStopRequest(nsIRequest* aRequest,
                                                  nsISupports* aContext,
                                                  nsresult aStatus)
{
  
  if (mTargetListener) {
    nsCOMPtr<nsIStreamListener> listener;
    mTargetListener.swap(listener);
    return listener->OnStopRequest(aRequest, aContext, aStatus);
  }

  return NS_OK;
}

nsresult
nsExternalResourceMap::PendingLoad::StartLoad(nsIURI* aURI,
                                              nsINode* aRequestingNode)
{
  NS_PRECONDITION(aURI, "Must have a URI");
  NS_PRECONDITION(aRequestingNode, "Must have a node");

  

  nsIPrincipal* requestingPrincipal = aRequestingNode->NodePrincipal();

  nsresult rv = nsContentUtils::GetSecurityManager()->
    CheckLoadURIWithPrincipal(requestingPrincipal, aURI,
                              nsIScriptSecurityManager::STANDARD);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = requestingPrincipal->CheckMayLoad(aURI, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt16 shouldLoad = nsIContentPolicy::ACCEPT;
  rv = NS_CheckContentLoadPolicy(nsIContentPolicy::TYPE_OTHER,
                                 aURI,
                                 requestingPrincipal,
                                 aRequestingNode,
                                 EmptyCString(), 
                                 nsnull,         
                                 &shouldLoad,
                                 nsContentUtils::GetContentPolicy(),
                                 nsContentUtils::GetSecurityManager());
  if (NS_FAILED(rv)) return rv;
  if (NS_CP_REJECTED(shouldLoad)) {
    
    return NS_ERROR_CONTENT_BLOCKED;
  }

  nsIDocument* doc = aRequestingNode->GetOwnerDoc();
  if (!doc) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsCOMPtr<nsIInterfaceRequestor> req = nsContentUtils::GetSameOriginChecker();
  NS_ENSURE_TRUE(req, NS_ERROR_OUT_OF_MEMORY);

  nsCOMPtr<nsILoadGroup> loadGroup = doc->GetDocumentLoadGroup();
  nsCOMPtr<nsIChannel> channel;
  rv = NS_NewChannel(getter_AddRefs(channel), aURI, nsnull, loadGroup, req);
  NS_ENSURE_SUCCESS(rv, rv);

  mURI = aURI;

  return channel->AsyncOpen(this, nsnull);
}

NS_IMPL_ISUPPORTS1(nsExternalResourceMap::LoadgroupCallbacks,
                   nsIInterfaceRequestor)

#define IMPL_SHIM(_i) \
  NS_IMPL_ISUPPORTS1(nsExternalResourceMap::LoadgroupCallbacks::_i##Shim, _i)

IMPL_SHIM(nsILoadContext)
IMPL_SHIM(nsIProgressEventSink)
IMPL_SHIM(nsIChannelEventSink)
IMPL_SHIM(nsISecurityEventSink)
IMPL_SHIM(nsIApplicationCacheContainer)

#undef IMPL_SHIM

#define IID_IS(_i) aIID.Equals(NS_GET_IID(_i))

#define TRY_SHIM(_i)                                                       \
  PR_BEGIN_MACRO                                                           \
    if (IID_IS(_i)) {                                                      \
      nsCOMPtr<_i> real = do_GetInterface(mCallbacks);                     \
      if (!real) {                                                         \
        return NS_NOINTERFACE;                                             \
      }                                                                    \
      nsCOMPtr<_i> shim = new _i##Shim(this, real);                        \
      if (!shim) {                                                         \
        return NS_ERROR_OUT_OF_MEMORY;                                     \
      }                                                                    \
      *aSink = shim.forget().get();                                        \
      return NS_OK;                                                        \
    }                                                                      \
  PR_END_MACRO

NS_IMETHODIMP
nsExternalResourceMap::LoadgroupCallbacks::GetInterface(const nsIID & aIID,
                                                        void **aSink)
{
  if (mCallbacks &&
      (IID_IS(nsIPrompt) || IID_IS(nsIAuthPrompt) || IID_IS(nsIAuthPrompt2))) {
    return mCallbacks->GetInterface(aIID, aSink);
  }

  *aSink = nsnull;

  TRY_SHIM(nsILoadContext);
  TRY_SHIM(nsIProgressEventSink);
  TRY_SHIM(nsIChannelEventSink);
  TRY_SHIM(nsISecurityEventSink);
  TRY_SHIM(nsIApplicationCacheContainer);
    
  return NS_NOINTERFACE;
}

#undef TRY_SHIM
#undef IID_IS

nsExternalResourceMap::ExternalResource::~ExternalResource()
{
  if (mViewer) {
    mViewer->Close(nsnull);
    mViewer->Destroy();
  }
}








class nsDOMStyleSheetSetList : public nsIDOMDOMStringList
                          
{
public:
  NS_DECL_ISUPPORTS

  NS_DECL_NSIDOMDOMSTRINGLIST

  nsDOMStyleSheetSetList(nsIDocument* aDocument);

  void Disconnect()
  {
    mDocument = nsnull;
  }

protected:
  
  nsresult GetSets(nsTArray<nsString>& aStyleSets);
  
  nsIDocument* mDocument;  
                           
};

NS_IMPL_ADDREF(nsDOMStyleSheetSetList)
NS_IMPL_RELEASE(nsDOMStyleSheetSetList)
NS_INTERFACE_TABLE_HEAD(nsDOMStyleSheetSetList)
  NS_OFFSET_AND_INTERFACE_TABLE_BEGIN(nsDOMStyleSheetSetList)
    NS_INTERFACE_TABLE_ENTRY(nsDOMStyleSheetSetList, nsIDOMDOMStringList)
  NS_OFFSET_AND_INTERFACE_TABLE_END
  NS_OFFSET_AND_INTERFACE_TABLE_TO_MAP_SEGUE
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(DOMStringList)
NS_INTERFACE_MAP_END

nsDOMStyleSheetSetList::nsDOMStyleSheetSetList(nsIDocument* aDocument)
  : mDocument(aDocument)
{
  NS_ASSERTION(mDocument, "Must have document!");
}

NS_IMETHODIMP
nsDOMStyleSheetSetList::Item(PRUint32 aIndex, nsAString& aResult)
{
  nsTArray<nsString> styleSets;
  nsresult rv = GetSets(styleSets);
  NS_ENSURE_SUCCESS(rv, rv);
  
  if (aIndex >= styleSets.Length()) {
    SetDOMStringToNull(aResult);
  } else {
    aResult = styleSets[aIndex];
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDOMStyleSheetSetList::GetLength(PRUint32 *aLength)
{
  nsTArray<nsString> styleSets;
  nsresult rv = GetSets(styleSets);
  NS_ENSURE_SUCCESS(rv, rv);
  
  *aLength = styleSets.Length();

  return NS_OK;
}

NS_IMETHODIMP
nsDOMStyleSheetSetList::Contains(const nsAString& aString, PRBool *aResult)
{
  nsTArray<nsString> styleSets;
  nsresult rv = GetSets(styleSets);
  NS_ENSURE_SUCCESS(rv, rv);
  
  *aResult = styleSets.Contains(aString);

  return NS_OK;
}

nsresult
nsDOMStyleSheetSetList::GetSets(nsTArray<nsString>& aStyleSets)
{
  if (!mDocument) {
    return NS_OK; 
                  
  }
  
  PRInt32 count = mDocument->GetNumberOfStyleSheets();
  nsAutoString title;
  nsAutoString temp;
  for (PRInt32 index = 0; index < count; index++) {
    nsIStyleSheet* sheet = mDocument->GetStyleSheetAt(index);
    NS_ASSERTION(sheet, "Null sheet in sheet list!");
    sheet->GetTitle(title);
    if (!title.IsEmpty() && !aStyleSets.Contains(title) &&
        !aStyleSets.AppendElement(title)) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  return NS_OK;
}





class nsDOMImplementation : public nsIDOMDOMImplementation,
                            public nsIPrivateDOMImplementation
{
public:
  nsDOMImplementation(nsIScriptGlobalObject* aScriptObject,
                      nsIURI* aDocumentURI,
                      nsIURI* aBaseURI,
                      nsIPrincipal* aPrincipal);
  virtual ~nsDOMImplementation();

  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOMDOMIMPLEMENTATION

  
  NS_IMETHOD Init(nsIURI* aDocumentURI, nsIURI* aBaseURI,
                  nsIPrincipal* aPrincipal);

protected:
  nsWeakPtr mScriptObject;
  nsCOMPtr<nsIURI> mDocumentURI;
  nsCOMPtr<nsIURI> mBaseURI;
  nsCOMPtr<nsIPrincipal> mPrincipal;
};


nsresult
NS_NewDOMImplementation(nsIDOMDOMImplementation** aInstancePtrResult)
{
  *aInstancePtrResult = new nsDOMImplementation(nsnull, nsnull, nsnull, nsnull);
  if (!*aInstancePtrResult) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_ADDREF(*aInstancePtrResult);

  return NS_OK;
}

nsDOMImplementation::nsDOMImplementation(nsIScriptGlobalObject* aScriptObject,
                                         nsIURI* aDocumentURI,
                                         nsIURI* aBaseURI,
                                         nsIPrincipal* aPrincipal)
  : mScriptObject(do_GetWeakReference(aScriptObject)),
    mDocumentURI(aDocumentURI),
    mBaseURI(aBaseURI),
    mPrincipal(aPrincipal)
{
}

nsDOMImplementation::~nsDOMImplementation()
{
}


NS_INTERFACE_MAP_BEGIN(nsDOMImplementation)
  NS_INTERFACE_MAP_ENTRY(nsIDOMDOMImplementation)
  NS_INTERFACE_MAP_ENTRY(nsIPrivateDOMImplementation)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMDOMImplementation)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(DOMImplementation)
NS_INTERFACE_MAP_END


NS_IMPL_ADDREF(nsDOMImplementation)
NS_IMPL_RELEASE(nsDOMImplementation)


NS_IMETHODIMP
nsDOMImplementation::HasFeature(const nsAString& aFeature,
                                const nsAString& aVersion,
                                PRBool* aReturn)
{
  return nsGenericElement::InternalIsSupported(
           static_cast<nsIDOMDOMImplementation*>(this),
           aFeature, aVersion, aReturn);
}

NS_IMETHODIMP
nsDOMImplementation::CreateDocumentType(const nsAString& aQualifiedName,
                                        const nsAString& aPublicId,
                                        const nsAString& aSystemId,
                                        nsIDOMDocumentType** aReturn)
{
  *aReturn = nsnull;

  nsresult rv = nsContentUtils::CheckQName(aQualifiedName);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIAtom> name = do_GetAtom(aQualifiedName);
  NS_ENSURE_TRUE(name, NS_ERROR_OUT_OF_MEMORY);

  
  nsAutoString voidString;
  voidString.SetIsVoid(PR_TRUE);
  return NS_NewDOMDocumentType(aReturn, nsnull, mPrincipal, name, nsnull,
                               nsnull, aPublicId, aSystemId, voidString);
}

NS_IMETHODIMP
nsDOMImplementation::CreateDocument(const nsAString& aNamespaceURI,
                                    const nsAString& aQualifiedName,
                                    nsIDOMDocumentType* aDoctype,
                                    nsIDOMDocument** aReturn)
{
  *aReturn = nsnull;

  nsresult rv;
  if (!aQualifiedName.IsEmpty()) {
    nsIParserService *parserService = nsContentUtils::GetParserService();
    NS_ENSURE_TRUE(parserService, NS_ERROR_FAILURE);

    const nsAFlatString& qName = PromiseFlatString(aQualifiedName);
    const PRUnichar *colon;
    rv = parserService->CheckQName(qName, PR_TRUE, &colon);
    NS_ENSURE_SUCCESS(rv, rv);

    if (colon &&
        (DOMStringIsNull(aNamespaceURI) ||
         (Substring(qName.get(), colon).EqualsLiteral("xml") &&
          !aNamespaceURI.EqualsLiteral("http://www.w3.org/XML/1998/namespace")))) {
      return NS_ERROR_DOM_NAMESPACE_ERR;
    }
  }
  else if (DOMStringIsNull(aQualifiedName) &&
           !DOMStringIsNull(aNamespaceURI)) {
    return NS_ERROR_DOM_NAMESPACE_ERR;
  }

  if (aDoctype) {
    nsCOMPtr<nsIDOMDocument> owner;
    aDoctype->GetOwnerDocument(getter_AddRefs(owner));
    if (owner) {
      return NS_ERROR_DOM_WRONG_DOCUMENT_ERR;
    }
  }

  nsCOMPtr<nsIScriptGlobalObject> scriptHandlingObject =
    do_QueryReferent(mScriptObject);

  return nsContentUtils::CreateDocument(aNamespaceURI, aQualifiedName, aDoctype,
                                        mDocumentURI, mBaseURI, mPrincipal,
                                        scriptHandlingObject, aReturn);
}

NS_IMETHODIMP
nsDOMImplementation::Init(nsIURI* aDocumentURI, nsIURI* aBaseURI,
                          nsIPrincipal* aPrincipal)
{
  
  
  mDocumentURI = aDocumentURI;
  mBaseURI = aBaseURI;
  mPrincipal = aPrincipal;
  return NS_OK;
}





  
  

nsDocument::nsDocument(const char* aContentType)
  : nsIDocument(),
    mVisible(PR_TRUE)
{
  mContentType = aContentType;
  
#ifdef PR_LOGGING
  if (!gDocumentLeakPRLog)
    gDocumentLeakPRLog = PR_NewLogModule("DocumentLeak");

  if (gDocumentLeakPRLog)
    PR_LOG(gDocumentLeakPRLog, PR_LOG_DEBUG,
           ("DOCUMENT %p created", this));
#endif

  
  SetDOMStringToNull(mLastStyleSheetSet);
}

static PLDHashOperator
ClearAllBoxObjects(const void* aKey, nsPIBoxObject* aBoxObject, void* aUserArg)
{
  if (aBoxObject) {
    aBoxObject->Clear();
  }
  return PL_DHASH_NEXT;
}

nsDocument::~nsDocument()
{
#ifdef PR_LOGGING
  if (gDocumentLeakPRLog)
    PR_LOG(gDocumentLeakPRLog, PR_LOG_DEBUG,
           ("DOCUMENT %p destroyed", this));
#endif

#ifdef DEBUG
  nsCycleCollector_DEBUG_wasFreed(static_cast<nsIDocument*>(this));
#endif

  mInDestructor = PR_TRUE;

  
  mObservers.Clear();

  if (mStyleSheetSetList) {
    mStyleSheetSetList->Disconnect();
  }

  mParentDocument = nsnull;

  
  
  if (mSubDocuments) {
    PL_DHashTableDestroy(mSubDocuments);

    mSubDocuments = nsnull;
  }

  
  
  DestroyLinkMap();

  nsAutoScriptBlocker scriptBlocker;

  PRInt32 indx; 
  PRUint32 count = mChildren.ChildCount();
  for (indx = PRInt32(count) - 1; indx >= 0; --indx) {
    mChildren.ChildAt(indx)->UnbindFromTree();
    mChildren.RemoveChildAt(indx);
  }
  mCachedRootContent = nsnull;

  
  indx = mStyleSheets.Count();
  while (--indx >= 0) {
    mStyleSheets[indx]->SetOwningDocument(nsnull);
  }
  indx = mCatalogSheets.Count();
  while (--indx >= 0) {
    mCatalogSheets[indx]->SetOwningDocument(nsnull);
  }
  if (mAttrStyleSheet)
    mAttrStyleSheet->SetOwningDocument(nsnull);
  if (mStyleAttrStyleSheet)
    mStyleAttrStyleSheet->SetOwningDocument(nsnull);

  if (mListenerManager) {
    mListenerManager->Disconnect();
  }

  if (mScriptLoader) {
    mScriptLoader->DropDocumentReference();
  }

  if (mCSSLoader) {
    
    mCSSLoader->DropDocumentReference();
    NS_RELEASE(mCSSLoader);
  }

  
  if (mNodeInfoManager) {
    mNodeInfoManager->DropDocumentReference();
    NS_RELEASE(mNodeInfoManager);
  }

  if (mAttrStyleSheet) {
    mAttrStyleSheet->SetOwningDocument(nsnull);
  }
  
  if (mStyleAttrStyleSheet) {
    mStyleAttrStyleSheet->SetOwningDocument(nsnull);
  }

  delete mHeaderData;

  if (mBoxObjectTable) {
    mBoxObjectTable->EnumerateRead(ClearAllBoxObjects, nsnull);
    delete mBoxObjectTable;
  }

  mPendingTitleChangeEvent.Revoke();
}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsDocument)

NS_INTERFACE_TABLE_HEAD(nsDocument)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_DOCUMENT_INTERFACE_TABLE_BEGIN(nsDocument)
    NS_INTERFACE_TABLE_ENTRY(nsDocument, nsINode)
    NS_INTERFACE_TABLE_ENTRY(nsDocument, nsIDocument)
    NS_INTERFACE_TABLE_ENTRY(nsDocument, nsIDOM3DocumentEvent)
    NS_INTERFACE_TABLE_ENTRY(nsDocument, nsIDOMDocumentStyle)
    NS_INTERFACE_TABLE_ENTRY(nsDocument, nsIDOMNSDocumentStyle)
    NS_INTERFACE_TABLE_ENTRY(nsDocument, nsIDOMDocumentRange)
    NS_INTERFACE_TABLE_ENTRY(nsDocument, nsIDOMDocumentXBL)
    NS_INTERFACE_TABLE_ENTRY(nsDocument, nsIScriptObjectPrincipal)
    NS_INTERFACE_TABLE_ENTRY(nsDocument, nsIDOM3EventTarget)
    NS_INTERFACE_TABLE_ENTRY(nsDocument, nsIDOMNSEventTarget)
    NS_INTERFACE_TABLE_ENTRY(nsDocument, nsPIDOMEventTarget)
    NS_INTERFACE_TABLE_ENTRY(nsDocument, nsISupportsWeakReference)
    NS_INTERFACE_TABLE_ENTRY(nsDocument, nsIRadioGroupContainer)
    NS_INTERFACE_TABLE_ENTRY(nsDocument, nsIMutationObserver)
    NS_INTERFACE_TABLE_ENTRY(nsDocument, nsIDOMNodeSelector)
    NS_INTERFACE_TABLE_ENTRY(nsDocument, nsIApplicationCacheContainer)
    NS_INTERFACE_TABLE_ENTRY(nsDocument, nsIDOMXPathNSResolver)
  NS_OFFSET_AND_INTERFACE_TABLE_END
  NS_OFFSET_AND_INTERFACE_TABLE_TO_MAP_SEGUE
  NS_INTERFACE_MAP_ENTRIES_CYCLE_COLLECTION(nsDocument)
  if (aIID.Equals(NS_GET_IID(nsIDOMXPathEvaluator)) ||
      aIID.Equals(NS_GET_IID(nsIXPathEvaluatorInternal))) {
    if (!mXPathEvaluatorTearoff) {
      nsresult rv;
      mXPathEvaluatorTearoff =
        do_CreateInstance(NS_XPATH_EVALUATOR_CONTRACTID,
                          static_cast<nsIDocument *>(this), &rv);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    return mXPathEvaluatorTearoff->QueryInterface(aIID, aInstancePtr);
  }
  else
NS_INTERFACE_MAP_END


NS_IMPL_CYCLE_COLLECTING_ADDREF_AMBIGUOUS(nsDocument, nsIDocument)
NS_IMPL_CYCLE_COLLECTING_RELEASE_AMBIGUOUS_WITH_DESTROY(nsDocument, 
                                                        nsIDocument,
                                                        nsNodeUtils::LastRelease(this))

NS_IMPL_CYCLE_COLLECTION_ROOT_BEGIN(nsDocument)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_ROOT_END

static PLDHashOperator
SubDocTraverser(PLDHashTable *table, PLDHashEntryHdr *hdr, PRUint32 number,
                void *arg)
{
  SubDocMapEntry *entry = static_cast<SubDocMapEntry*>(hdr);
  nsCycleCollectionTraversalCallback *cb = 
    static_cast<nsCycleCollectionTraversalCallback*>(arg);

  NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(*cb, "mSubDocuments entry->mKey");
  cb->NoteXPCOMChild(entry->mKey);
  NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(*cb, "mSubDocuments entry->mSubDocument");
  cb->NoteXPCOMChild(entry->mSubDocument);

  return PL_DHASH_NEXT;
}

static PLDHashOperator
RadioGroupsTraverser(const nsAString& aKey, nsRadioGroupStruct* aData,
                     void* aClosure)
{
  nsCycleCollectionTraversalCallback *cb = 
    static_cast<nsCycleCollectionTraversalCallback*>(aClosure);

  NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(*cb,
                                   "mRadioGroups entry->mSelectedRadioButton");
  cb->NoteXPCOMChild(aData->mSelectedRadioButton);

  PRUint32 i, count = aData->mRadioButtons.Count();
  for (i = 0; i < count; ++i) {
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(*cb,
                                       "mRadioGroups entry->mRadioButtons[i]");
    cb->NoteXPCOMChild(aData->mRadioButtons[i]);
  }

  return PL_DHASH_NEXT;
}

static PLDHashOperator
BoxObjectTraverser(const void* key, nsPIBoxObject* boxObject, void* userArg)
{
  nsCycleCollectionTraversalCallback *cb = 
    static_cast<nsCycleCollectionTraversalCallback*>(userArg);
 
  NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(*cb, "mBoxObjectTable entry");
  cb->NoteXPCOMChild(boxObject);

  return PL_DHASH_NEXT;
}

class LinkMapTraversalVisitor : public nsUint32ToContentHashEntry::Visitor
{
public:
  nsCycleCollectionTraversalCallback *mCb;
  virtual void Visit(nsIContent* aContent)
  {
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(*mCb, "mLinkMap entry");
    mCb->NoteXPCOMChild(aContent);
  }
};

static PLDHashOperator
LinkMapTraverser(nsUint32ToContentHashEntry* aEntry, void* userArg)
{
  LinkMapTraversalVisitor visitor;
  visitor.mCb = static_cast<nsCycleCollectionTraversalCallback*>(userArg);
  aEntry->VisitContent(&visitor);
  return PL_DHASH_NEXT;
}

static PLDHashOperator
IdentifierMapEntryTraverse(nsIdentifierMapEntry *aEntry, void *aArg)
{
  nsCycleCollectionTraversalCallback *cb =
    static_cast<nsCycleCollectionTraversalCallback*>(aArg);
  aEntry->Traverse(cb);
  return PL_DHASH_NEXT;
}

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsDocument)
  
  
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS

  if (nsCCUncollectableMarker::InGeneration(cb, tmp->GetMarkedCCGeneration())) {
    return NS_SUCCESS_INTERRUPTED_TRAVERSE;
  }

  tmp->mIdentifierMap.EnumerateEntries(IdentifierMapEntryTraverse, &cb);

  tmp->mExternalResourceMap.Traverse(&cb);

  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mNodeInfo)

  
  for (PRInt32 indx = PRInt32(tmp->mChildren.ChildCount()); indx > 0; --indx) {
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mChildren[i]");
    cb.NoteXPCOMChild(tmp->mChildren.ChildAt(indx - 1));
  }

  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_USERDATA

  
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mCachedRootContent)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NATIVE_MEMBER(mNodeInfoManager,
                                                  nsNodeInfoManager)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mSecurityInfo)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mDisplayDocument)

  
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mParser)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mScriptGlobalObject)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mListenerManager)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mDOMStyleSheets)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mScriptLoader)

  tmp->mRadioGroups.EnumerateRead(RadioGroupsTraverser, &cb);

  
  
  if (tmp->mBoxObjectTable) {
    tmp->mBoxObjectTable->EnumerateRead(BoxObjectTraverser, &cb);
  }

  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mChannel)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mStyleAttrStyleSheet)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mScriptEventManager)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mXPathEvaluatorTearoff)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mLayoutHistoryState)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnloadBlocker)

  
  
  if (tmp->mLinkMap.IsInitialized()) {
    tmp->mLinkMap.EnumerateEntries(LinkMapTraverser, &cb);
  }

  
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMARRAY(mStyleSheets)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMARRAY(mCatalogSheets)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMARRAY(mVisitednessChangedURIs)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMARRAY(mPreloadingImages)

#ifdef MOZ_SMIL
  
  if (tmp->mAnimationController) {
    tmp->mAnimationController->Traverse(&cb);
  }
#endif 

  if (tmp->mSubDocuments && tmp->mSubDocuments->ops) {
    PL_DHashTableEnumerate(tmp->mSubDocuments, SubDocTraverser, &cb);
  }
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END


NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(nsDocument)
  NS_IMPL_CYCLE_COLLECTION_TRACE_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_TRACE_END


NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsDocument)
  
  
  
  tmp->DestroyLinkMap();

  
  tmp->mExternalResourceMap.Shutdown();

  nsAutoScriptBlocker scriptBlocker;

  
  for (PRInt32 indx = PRInt32(tmp->mChildren.ChildCount()) - 1; 
       indx >= 0; --indx) {
    tmp->mChildren.ChildAt(indx)->UnbindFromTree();
    tmp->mChildren.RemoveChildAt(indx);
  }

  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mCachedRootContent)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mDisplayDocument)

  NS_IMPL_CYCLE_COLLECTION_UNLINK_USERDATA

  tmp->mParentDocument = nsnull;

  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMARRAY(mPreloadingImages)

  
  
  
  
  
  
NS_IMPL_CYCLE_COLLECTION_UNLINK_END


nsresult
nsDocument::Init()
{
  if (mCSSLoader || mNodeInfoManager || mScriptLoader) {
    return NS_ERROR_ALREADY_INITIALIZED;
  }

  mIdentifierMap.Init();
  mLinkMap.Init();
  mRadioGroups.Init();

  
  nsINode::nsSlots* slots = GetSlots();
  NS_ENSURE_TRUE(slots,NS_ERROR_OUT_OF_MEMORY);

  
  
  
  
  NS_ENSURE_TRUE(slots->mMutationObservers.PrependElementUnlessExists(static_cast<nsIMutationObserver*>(this)),
                 NS_ERROR_OUT_OF_MEMORY);


  mOnloadBlocker = new nsOnloadBlocker();
  NS_ENSURE_TRUE(mOnloadBlocker, NS_ERROR_OUT_OF_MEMORY);
  
  NS_NewCSSLoader(this, &mCSSLoader);
  NS_ENSURE_TRUE(mCSSLoader, NS_ERROR_OUT_OF_MEMORY);
  
  mCSSLoader->SetCaseSensitive(PR_TRUE);
  mCSSLoader->SetCompatibilityMode(eCompatibility_FullStandards);

  mNodeInfoManager = new nsNodeInfoManager();
  NS_ENSURE_TRUE(mNodeInfoManager, NS_ERROR_OUT_OF_MEMORY);

  NS_ADDREF(mNodeInfoManager);

  nsresult  rv = mNodeInfoManager->Init(this);
  NS_ENSURE_SUCCESS(rv, rv);

  mNodeInfo = mNodeInfoManager->GetDocumentNodeInfo();
  NS_ENSURE_TRUE(mNodeInfo, NS_ERROR_OUT_OF_MEMORY);

  NS_ASSERTION(GetOwnerDoc() == this, "Our nodeinfo is busted!");

  mScriptLoader = new nsScriptLoader(this);
  NS_ENSURE_TRUE(mScriptLoader, NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}

nsresult
nsDocument::AddXMLEventsContent(nsIContent *aXMLEventsElement)
{
  if (!mXMLEventsManager) {
    mXMLEventsManager = new nsXMLEventsManager();
    NS_ENSURE_TRUE(mXMLEventsManager, NS_ERROR_OUT_OF_MEMORY);
    AddObserver(mXMLEventsManager);
  }
  mXMLEventsManager->AddXMLEventsContent(aXMLEventsElement);
  return NS_OK;
}

void
nsDocument::Reset(nsIChannel* aChannel, nsILoadGroup* aLoadGroup)
{
  nsCOMPtr<nsIURI> uri;
  nsCOMPtr<nsIPrincipal> principal;
  if (aChannel) {
    
    
    
    NS_GetFinalChannelURI(aChannel, getter_AddRefs(uri));

    nsIScriptSecurityManager *securityManager =
      nsContentUtils::GetSecurityManager();
    if (securityManager) {
      securityManager->GetChannelPrincipal(aChannel,
                                           getter_AddRefs(principal));
    }
  }

  ResetToURI(uri, aLoadGroup, principal);

  nsCOMPtr<nsIPropertyBag2> bag = do_QueryInterface(aChannel);
  if (bag) {
    nsCOMPtr<nsIURI> baseURI;
    bag->GetPropertyAsInterface(NS_LITERAL_STRING("baseURI"),
                                NS_GET_IID(nsIURI), getter_AddRefs(baseURI));
    if (baseURI) {
      mDocumentBaseURI = baseURI;
    }
  }

  mChannel = aChannel;
}

void
nsDocument::ResetToURI(nsIURI *aURI, nsILoadGroup *aLoadGroup,
                       nsIPrincipal* aPrincipal)
{
  NS_PRECONDITION(aURI, "Null URI passed to ResetToURI");

#ifdef PR_LOGGING
  if (gDocumentLeakPRLog && PR_LOG_TEST(gDocumentLeakPRLog, PR_LOG_DEBUG)) {
    nsCAutoString spec;
    aURI->GetSpec(spec);
    PR_LogPrint("DOCUMENT %p ResetToURI %s", this, spec.get());
  }
#endif

  mIdentifierMap.Clear();

  SetPrincipal(nsnull);
  mSecurityInfo = nsnull;

  mDocumentLoadGroup = nsnull;

  
  
  if (mSubDocuments) {
    PL_DHashTableDestroy(mSubDocuments);

    mSubDocuments = nsnull;
  }

  
  
  DestroyLinkMap();

  PRUint32 count = mChildren.ChildCount();
  { 
    MOZ_AUTO_DOC_UPDATE(this, UPDATE_CONTENT_MODEL, PR_TRUE);    
    for (PRInt32 i = PRInt32(count) - 1; i >= 0; i--) {
      nsCOMPtr<nsIContent> content = mChildren.ChildAt(i);

      
      
      
      nsNodeUtils::ContentRemoved(this, content, i);
      content->UnbindFromTree();
      mChildren.RemoveChildAt(i);
    }
  }
  mCachedRootContent = nsnull;

  
  ResetStylesheetsToURI(aURI);
  
  
  if (mListenerManager) {
    mListenerManager->Disconnect();
    mListenerManager = nsnull;
  }

  
  mDOMStyleSheets = nsnull;

  SetDocumentURI(aURI);
  mDocumentBaseURI = mDocumentURI;

  if (aLoadGroup) {
    mDocumentLoadGroup = do_GetWeakReference(aLoadGroup);
    
    
    

    
    
  }

  mLastModified.Truncate();
  
  
  mContentType.Truncate();
  mContentLanguage.Truncate();
  mBaseTarget.Truncate();
  mReferrer.Truncate();

  mXMLDeclarationBits = 0;

  
  if (aPrincipal) {
    SetPrincipal(aPrincipal);
  } else {
    nsIScriptSecurityManager *securityManager =
      nsContentUtils::GetSecurityManager();
    if (securityManager) {
      nsCOMPtr<nsIPrincipal> principal;
      nsresult rv =
        securityManager->GetCodebasePrincipal(mDocumentURI,
                                              getter_AddRefs(principal));
      if (NS_SUCCEEDED(rv)) {
        SetPrincipal(principal);
      }
    }
  }
}

nsresult
nsDocument::ResetStylesheetsToURI(nsIURI* aURI)
{
  NS_PRECONDITION(aURI, "Null URI passed to ResetStylesheetsToURI");

  mozAutoDocUpdate upd(this, UPDATE_STYLE, PR_TRUE);
  
  
  PRInt32 indx = mStyleSheets.Count();
  while (--indx >= 0) {
    nsIStyleSheet* sheet = mStyleSheets[indx];
    sheet->SetOwningDocument(nsnull);

    PRBool applicable;
    sheet->GetApplicable(applicable);
    if (applicable) {
      RemoveStyleSheetFromStyleSets(sheet);
    }

    
  }

  indx = mCatalogSheets.Count();
  while (--indx >= 0) {
    nsIStyleSheet* sheet = mCatalogSheets[indx];
    sheet->SetOwningDocument(nsnull);

    PRBool applicable;
    sheet->GetApplicable(applicable);
    if (applicable) {
      nsPresShellIterator iter(this);
      nsCOMPtr<nsIPresShell> shell;
      while ((shell = iter.GetNextShell())) {
        shell->StyleSet()->RemoveStyleSheet(nsStyleSet::eAgentSheet, sheet);
      }
    }

    
  }


  
  mStyleSheets.Clear();
  
  
  

  
  nsresult rv;
  nsStyleSet::sheetType attrSheetType = GetAttrSheetType();
  if (mAttrStyleSheet) {
    
    nsPresShellIterator iter(this);
    nsCOMPtr<nsIPresShell> shell;
    while ((shell = iter.GetNextShell())) {
      shell->StyleSet()->RemoveStyleSheet(attrSheetType, mAttrStyleSheet);
    }
    rv = mAttrStyleSheet->Reset(aURI);
  } else {
    rv = NS_NewHTMLStyleSheet(getter_AddRefs(mAttrStyleSheet), aURI, this);
  }
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  mAttrStyleSheet->SetOwningDocument(this);
  
  if (mStyleAttrStyleSheet) {
    
    nsPresShellIterator iter(this);
    nsCOMPtr<nsIPresShell> shell;
    while ((shell = iter.GetNextShell())) {
      shell->StyleSet()->
        RemoveStyleSheet(nsStyleSet::eStyleAttrSheet, mStyleAttrStyleSheet);
    }
    rv = mStyleAttrStyleSheet->Reset(aURI);
  } else {
    rv = NS_NewHTMLCSSStyleSheet(getter_AddRefs(mStyleAttrStyleSheet), aURI,
                                                this);
  }
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  mStyleAttrStyleSheet->SetOwningDocument(this);

  
  nsPresShellIterator iter(this);
  nsCOMPtr<nsIPresShell> shell;
  while ((shell = iter.GetNextShell())) {
    FillStyleSet(shell->StyleSet());
  }

  return rv;
}

nsStyleSet::sheetType
nsDocument::GetAttrSheetType()
{
  return nsStyleSet::ePresHintSheet;
}

void
nsDocument::FillStyleSet(nsStyleSet* aStyleSet)
{
  NS_PRECONDITION(aStyleSet, "Must have a style set");
  NS_PRECONDITION(aStyleSet->SheetCount(nsStyleSet::ePresHintSheet) == 0,
                  "Style set already has a preshint sheet?");
  NS_PRECONDITION(aStyleSet->SheetCount(nsStyleSet::eHTMLPresHintSheet) == 0,
                  "Style set already has a HTML preshint sheet?");
  NS_PRECONDITION(aStyleSet->SheetCount(nsStyleSet::eDocSheet) == 0,
                  "Style set already has document sheets?");
  NS_PRECONDITION(aStyleSet->SheetCount(nsStyleSet::eStyleAttrSheet) == 0,
                  "Style set already has style attr sheets?");
  NS_PRECONDITION(mStyleAttrStyleSheet, "No style attr stylesheet?");
  NS_PRECONDITION(mAttrStyleSheet, "No attr stylesheet?");
  
  aStyleSet->AppendStyleSheet(GetAttrSheetType(), mAttrStyleSheet);

  aStyleSet->AppendStyleSheet(nsStyleSet::eStyleAttrSheet,
                              mStyleAttrStyleSheet);

  PRInt32 i;
  for (i = mStyleSheets.Count() - 1; i >= 0; --i) {
    nsIStyleSheet* sheet = mStyleSheets[i];
    PRBool sheetApplicable;
    sheet->GetApplicable(sheetApplicable);
    if (sheetApplicable) {
      aStyleSet->AddDocStyleSheet(sheet, this);
    }
  }

  for (i = mCatalogSheets.Count() - 1; i >= 0; --i) {
    nsIStyleSheet* sheet = mCatalogSheets[i];
    PRBool sheetApplicable;
    sheet->GetApplicable(sheetApplicable);
    if (sheetApplicable) {
      aStyleSet->AppendStyleSheet(nsStyleSet::eAgentSheet, sheet);
    }
  }
}

nsresult
nsDocument::StartDocumentLoad(const char* aCommand, nsIChannel* aChannel,
                              nsILoadGroup* aLoadGroup,
                              nsISupports* aContainer,
                              nsIStreamListener **aDocListener,
                              PRBool aReset, nsIContentSink* aSink)
{
#ifdef PR_LOGGING
  if (gDocumentLeakPRLog && PR_LOG_TEST(gDocumentLeakPRLog, PR_LOG_DEBUG)) {
    nsCOMPtr<nsIURI> uri;
    aChannel->GetURI(getter_AddRefs(uri));
    nsCAutoString spec;
    if (uri)
      uri->GetSpec(spec);
    PR_LogPrint("DOCUMENT %p StartDocumentLoad %s", this, spec.get());
  }
#endif

  SetReadyStateInternal(READYSTATE_LOADING);

  if (nsCRT::strcmp(kLoadAsData, aCommand) == 0) {
    mLoadedAsData = PR_TRUE;
    
    
    

    
    ScriptLoader()->SetEnabled(PR_FALSE);

    
    CSSLoader()->SetEnabled(PR_FALSE); 
  } else if (nsCRT::strcmp("external-resource", aCommand) == 0) {
    
    ScriptLoader()->SetEnabled(PR_FALSE);
  }

  mMayStartLayout = PR_FALSE;

  mHaveInputEncoding = PR_TRUE;

  if (aReset) {
    Reset(aChannel, aLoadGroup);
  }

  nsCAutoString contentType;
  if (NS_SUCCEEDED(aChannel->GetContentType(contentType))) {
    
    nsACString::const_iterator start, end, semicolon;
    contentType.BeginReading(start);
    contentType.EndReading(end);
    semicolon = start;
    FindCharInReadable(';', semicolon, end);
    mContentType = Substring(start, semicolon);
  }

  RetrieveRelevantHeaders(aChannel);

  mChannel = aChannel;

  return NS_OK;
}

void
nsDocument::StopDocumentLoad()
{
  if (mParser) {
    mParser->Terminate();
  }
}

void
nsDocument::SetDocumentURI(nsIURI* aURI)
{
  mDocumentURI = NS_TryToMakeImmutable(aURI);
}

NS_IMETHODIMP
nsDocument::GetLastModified(nsAString& aLastModified)
{
  if (!mLastModified.IsEmpty()) {
    aLastModified.Assign(mLastModified);
  } else {
    
    
    aLastModified.Assign(NS_LITERAL_STRING("01/01/1970 00:00:00"));
  }

  return NS_OK;
}

void
nsDocument::UpdateNameTableEntry(nsIContent *aContent)
{
  if (!mIsRegularHTML)
    return;

  nsIAtom* name = nsContentUtils::IsNamedItem(aContent);
  if (!name)
    return;

  nsIdentifierMapEntry *entry = mIdentifierMap.GetEntry(name);
  if (!entry) {
    
    return;
  }

  entry->AddNameContent(aContent);
}

void
nsDocument::RemoveFromNameTable(nsIContent *aContent)
{
  if (!mIsRegularHTML)
    return;

  nsIAtom* name = nsContentUtils::IsNamedItem(aContent);
  if (!name)
    return;

  nsIdentifierMapEntry *entry = mIdentifierMap.GetEntry(name);
  if (!entry) {
    
    return;
  }

  entry->RemoveNameContent(aContent);
}

void
nsDocument::UpdateIdTableEntry(nsIContent *aContent)
{
  nsIAtom* id = aContent->GetID();
  if (!id)
    return;

  nsIdentifierMapEntry *entry = mIdentifierMap.PutEntry(id);

  if (entry) { 
    entry->AddIdContent(aContent);
  }
}

void
nsDocument::RemoveFromIdTable(nsIContent *aContent)
{
  nsIAtom* id = aContent->GetID();
  if (!id)
    return;

  nsIdentifierMapEntry *entry = mIdentifierMap.GetEntry(id);
  if (!entry) 
    return;

  if (entry->RemoveIdContent(aContent)) {
    mIdentifierMap.RemoveEntry(id);
  }
}

void
nsDocument::UnregisterNamedItems(nsIContent *aContent)
{
  if (!aContent->IsNodeOfType(nsINode::eELEMENT)) {
    
    return;
  }

  RemoveFromNameTable(aContent);
  RemoveFromIdTable(aContent);

  for (nsINode::ChildIterator iter(aContent); !iter.IsDone(); iter.Next()) {
    UnregisterNamedItems(iter);
  }
}

void
nsDocument::RegisterNamedItems(nsIContent *aContent)
{
  if (!aContent->IsNodeOfType(nsINode::eELEMENT)) {
    
    return;
  }

  UpdateNameTableEntry(aContent);
  UpdateIdTableEntry(aContent);

  for (nsINode::ChildIterator iter(aContent); !iter.IsDone(); iter.Next()) {
    RegisterNamedItems(iter);
  }
}

void
nsDocument::ContentAppended(nsIDocument* aDocument,
                            nsIContent* aContainer,
                            PRInt32 aNewIndexInContainer)
{
  NS_ASSERTION(aDocument == this, "unexpected doc");

  for (nsINode::ChildIterator iter(aContainer, aNewIndexInContainer);
       !iter.IsDone();
       iter.Next()) {
    RegisterNamedItems(iter);
  }
}

void
nsDocument::ContentInserted(nsIDocument* aDocument,
                            nsIContent* aContainer,
                            nsIContent* aContent,
                            PRInt32 aIndexInContainer)
{
  NS_ASSERTION(aDocument == this, "unexpected doc");

  NS_ABORT_IF_FALSE(aContent, "Null content!");

  RegisterNamedItems(aContent);
}

void
nsDocument::ContentRemoved(nsIDocument* aDocument,
                           nsIContent* aContainer,
                           nsIContent* aChild,
                           PRInt32 aIndexInContainer)
{
  NS_ASSERTION(aDocument == this, "unexpected doc");

  NS_ABORT_IF_FALSE(aChild, "Null content!");

  UnregisterNamedItems(aChild);
}

void
nsDocument::AttributeWillChange(nsIDocument* aDocument,
                                nsIContent* aContent, PRInt32 aNameSpaceID,
                                nsIAtom* aAttribute, PRInt32 aModType)
{
  NS_ABORT_IF_FALSE(aContent, "Null content!");
  NS_PRECONDITION(aAttribute, "Must have an attribute that's changing!");

  if (aNameSpaceID != kNameSpaceID_None)
    return;
  if (aAttribute == nsGkAtoms::name) {
    RemoveFromNameTable(aContent);
  } else if (aAttribute == aContent->GetIDAttributeName()) {
    RemoveFromIdTable(aContent);
  }
}

void
nsDocument::AttributeChanged(nsIDocument* aDocument,
                             nsIContent* aContent, PRInt32 aNameSpaceID,
                             nsIAtom* aAttribute, PRInt32 aModType,
                             PRUint32 aStateMask)
{
  NS_ASSERTION(aDocument == this, "unexpected doc");

  NS_ABORT_IF_FALSE(aContent, "Null content!");
  NS_PRECONDITION(aAttribute, "Must have an attribute that's changing!");

  if (aNameSpaceID != kNameSpaceID_None)
    return;
  if (aAttribute == nsGkAtoms::name) {
    UpdateNameTableEntry(aContent);
  } else if (aAttribute == aContent->GetIDAttributeName()) {
    UpdateIdTableEntry(aContent);
  }
}

nsIPrincipal*
nsDocument::GetPrincipal()
{
  return NodePrincipal();
}

extern PRBool sDisablePrefetchHTTPSPref;

void
nsDocument::SetPrincipal(nsIPrincipal *aNewPrincipal)
{
  if (aNewPrincipal && mAllowDNSPrefetch && sDisablePrefetchHTTPSPref) {
    nsCOMPtr<nsIURI> uri;
    aNewPrincipal->GetURI(getter_AddRefs(uri));
    PRBool isHTTPS;
    if (!uri || NS_FAILED(uri->SchemeIs("https", &isHTTPS)) ||
        isHTTPS) {
      mAllowDNSPrefetch = PR_FALSE;
    }
  }
  mNodeInfoManager->SetDocumentPrincipal(aNewPrincipal);
}

NS_IMETHODIMP
nsDocument::GetApplicationCache(nsIApplicationCache **aApplicationCache)
{
  NS_IF_ADDREF(*aApplicationCache = mApplicationCache);

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::SetApplicationCache(nsIApplicationCache *aApplicationCache)
{
  mApplicationCache = aApplicationCache;

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetContentType(nsAString& aContentType)
{
  CopyUTF8toUTF16(mContentType, aContentType);

  return NS_OK;
}

void
nsDocument::SetContentType(const nsAString& aContentType)
{
  NS_ASSERTION(mContentType.IsEmpty() ||
               mContentType.Equals(NS_ConvertUTF16toUTF8(aContentType)),
               "Do you really want to change the content-type?");

  CopyUTF16toUTF8(aContentType, mContentType);
}



NS_IMETHODIMP
nsDocument::HasFocus(PRBool* aResult)
{
  *aResult = PR_FALSE;

  nsIFocusManager* fm = nsFocusManager::GetFocusManager();
  if (!fm)
    return NS_ERROR_NOT_AVAILABLE;

  
  nsCOMPtr<nsIDOMWindow> focusedWindow;
  fm->GetFocusedWindow(getter_AddRefs(focusedWindow));
  if (!focusedWindow)
    return NS_OK;

  
  nsCOMPtr<nsIDOMDocument> domDocument;
  focusedWindow->GetDocument(getter_AddRefs(domDocument));
  nsCOMPtr<nsIDocument> document = do_QueryInterface(domDocument);

  for (nsIDocument* currentDoc = document; currentDoc;
       currentDoc = currentDoc->GetParentDocument()) {
    if (currentDoc == this) {
      
      *aResult = PR_TRUE;
      return NS_OK;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetReferrer(nsAString& aReferrer)
{
  CopyUTF8toUTF16(mReferrer, aReferrer);
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetActiveElement(nsIDOMElement **aElement)
{
  *aElement = nsnull;

  
  nsCOMPtr<nsPIDOMWindow> window = GetWindow();
  if (!window) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsIFocusManager* fm = nsFocusManager::GetFocusManager();
  if (!fm)
    return NS_ERROR_NOT_AVAILABLE;

  nsCOMPtr<nsPIDOMWindow> focusedWindow;
  nsIContent* focusedContent =
    nsFocusManager::GetFocusedDescendant(window, PR_FALSE, getter_AddRefs(focusedWindow));

  
  if (focusedContent) {
    
    if (focusedContent->GetOwnerDoc() != this) {
      NS_WARNING("Focused element found from another document");
      return NS_ERROR_FAILURE;
    }

    CallQueryInterface(focusedContent, aElement);
    return NS_OK;
  }

  
  nsCOMPtr<nsIDOMHTMLDocument> htmlDoc =
    do_QueryInterface(static_cast<nsIDocument*>(this));
  if (htmlDoc) {
    nsCOMPtr<nsIDOMHTMLElement> bodyElement;
    htmlDoc->GetBody(getter_AddRefs(bodyElement));
    if (bodyElement) {
      *aElement = bodyElement;
      NS_ADDREF(*aElement);
    }
    
    
    return NS_OK;
  }

  
  return GetDocumentElement(aElement);
}

NS_IMETHODIMP
nsDocument::ElementFromPoint(PRInt32 aX, PRInt32 aY, nsIDOMElement** aReturn)
{
  return ElementFromPointHelper(aX, aY, PR_FALSE, PR_TRUE, aReturn);
}

nsresult
nsDocument::ElementFromPointHelper(PRInt32 aX, PRInt32 aY,
                                   PRBool aIgnoreRootScrollFrame,
                                   PRBool aFlushLayout,
                                   nsIDOMElement** aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);
  *aReturn = nsnull;
  
  if (!aIgnoreRootScrollFrame && (aX < 0 || aY < 0))
    return NS_OK;

  nscoord x = nsPresContext::CSSPixelsToAppUnits(aX);
  nscoord y = nsPresContext::CSSPixelsToAppUnits(aY);
  nsPoint pt(x, y);

  
  
  if (aFlushLayout)
    FlushPendingNotifications(Flush_Layout);

  nsIPresShell *ps = GetPrimaryShell();
  NS_ENSURE_STATE(ps);
  nsIFrame *rootFrame = ps->GetRootFrame();

  
  if (!rootFrame)
    return NS_OK; 

  nsIFrame *ptFrame = nsLayoutUtils::GetFrameForPoint(rootFrame, pt, PR_TRUE,
                                                      aIgnoreRootScrollFrame);
  if (!ptFrame)
    return NS_OK;

  nsIContent* ptContent = ptFrame->GetContent();
  NS_ENSURE_STATE(ptContent);

  
  nsIDocument *currentDoc = ptContent->GetCurrentDoc();
  if (currentDoc && (currentDoc != this)) {
    *aReturn = CheckAncestryAndGetFrame(currentDoc).get();
    return NS_OK;
  }

  
  
  
  while (ptContent &&
         (!ptContent->IsNodeOfType(nsINode::eELEMENT) ||
          ptContent->IsInAnonymousSubtree())) {
    
    ptContent = ptContent->GetParent();
  }
 
  if (ptContent)
    CallQueryInterface(ptContent, aReturn);
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetElementsByClassName(const nsAString& aClasses,
                                   nsIDOMNodeList** aReturn)
{
  return GetElementsByClassNameHelper(this, aClasses, aReturn);
}

struct ClassMatchingInfo {
  nsCOMArray<nsIAtom> mClasses;
  nsCaseTreatment mCaseTreatment;
};


nsresult
nsDocument::GetElementsByClassNameHelper(nsINode* aRootNode,
                                         const nsAString& aClasses,
                                         nsIDOMNodeList** aReturn)
{
  NS_PRECONDITION(aRootNode, "Must have root node");
  
  nsAttrValue attrValue;
  attrValue.ParseAtomArray(aClasses);
  
  ClassMatchingInfo* info = new ClassMatchingInfo;
  NS_ENSURE_TRUE(info, NS_ERROR_OUT_OF_MEMORY);

  if (attrValue.Type() == nsAttrValue::eAtomArray) {
    info->mClasses.AppendObjects(*(attrValue.GetAtomArrayValue()));
  } else if (attrValue.Type() == nsAttrValue::eAtom) {
    info->mClasses.AppendObject(attrValue.GetAtomValue());
  }

  nsBaseContentList* elements;
  if (info->mClasses.Count() > 0) {
    info->mCaseTreatment =
      aRootNode->GetOwnerDoc()->GetCompatibilityMode() ==
        eCompatibility_NavQuirks ?
          eIgnoreCase : eCaseMatters;
  
    elements = new nsContentList(aRootNode, MatchClassNames,
                                 DestroyClassNameArray, info);
  } else {
    delete info;
    info = nsnull;
    elements = new nsBaseContentList();
  }
  if (!elements) {
    delete info;
    return NS_ERROR_OUT_OF_MEMORY;
  }

  *aReturn = elements;
  NS_ADDREF(*aReturn);

  return NS_OK;
}


PRBool
nsDocument::MatchClassNames(nsIContent* aContent,
                            PRInt32 aNamespaceID,
                            nsIAtom* aAtom, void* aData)
{
  
  const nsAttrValue* classAttr = aContent->GetClasses();
  if (!classAttr) {
    return PR_FALSE;
  }
  
  
  ClassMatchingInfo* info = static_cast<ClassMatchingInfo*>(aData);
  PRInt32 length = info->mClasses.Count();
  PRInt32 i;
  for (i = 0; i < length; ++i) {
    if (!classAttr->Contains(info->mClasses.ObjectAt(i),
                             info->mCaseTreatment)) {
      return PR_FALSE;
    }
  }
  
  return PR_TRUE;
}


void
nsDocument::DestroyClassNameArray(void* aData)
{
  ClassMatchingInfo* info = static_cast<ClassMatchingInfo*>(aData);
  delete info;
}

nsresult
nsDocument::SetBaseURI(nsIURI* aURI)
{
  nsresult rv = NS_OK;

  if (aURI) {
    rv = nsContentUtils::GetSecurityManager()->
      CheckLoadURIWithPrincipal(NodePrincipal(), aURI,
                                nsIScriptSecurityManager::STANDARD);
    if (NS_SUCCEEDED(rv)) {
      mDocumentBaseURI = NS_TryToMakeImmutable(aURI);
    }
  } else {
    mDocumentBaseURI = nsnull;
  }

  return rv;
}

void
nsDocument::GetBaseTarget(nsAString &aBaseTarget) const
{
  aBaseTarget.Assign(mBaseTarget);
}

void
nsDocument::SetBaseTarget(const nsAString &aBaseTarget)
{
  mBaseTarget.Assign(aBaseTarget);
}

void
nsDocument::SetDocumentCharacterSet(const nsACString& aCharSetID)
{
  if (!mCharacterSet.Equals(aCharSetID)) {
    mCharacterSet = aCharSetID;

#ifdef DEBUG
    nsCOMPtr<nsICharsetAlias> calias(do_GetService(NS_CHARSETALIAS_CONTRACTID));
    if (calias) {
      nsCAutoString canonicalName;
      calias->GetPreferred(aCharSetID, canonicalName);
      NS_ASSERTION(canonicalName.Equals(aCharSetID),
                   "charset name must be canonical");
    }
#endif

    PRInt32 n = mCharSetObservers.Length();

    for (PRInt32 i = 0; i < n; i++) {
      nsIObserver* observer = mCharSetObservers.ElementAt(i);

      observer->Observe(static_cast<nsIDocument *>(this), "charset",
                        NS_ConvertASCIItoUTF16(aCharSetID).get());
    }
  }
}

nsresult
nsDocument::AddCharSetObserver(nsIObserver* aObserver)
{
  NS_ENSURE_ARG_POINTER(aObserver);

  NS_ENSURE_TRUE(mCharSetObservers.AppendElement(aObserver), NS_ERROR_FAILURE);

  return NS_OK;
}

void
nsDocument::RemoveCharSetObserver(nsIObserver* aObserver)
{
  mCharSetObservers.RemoveElement(aObserver);
}

void
nsDocument::GetHeaderData(nsIAtom* aHeaderField, nsAString& aData) const
{
  aData.Truncate();
  const nsDocHeaderData* data = mHeaderData;
  while (data) {
    if (data->mField == aHeaderField) {
      aData = data->mData;

      break;
    }
    data = data->mNext;
  }
}

void
nsDocument::SetHeaderData(nsIAtom* aHeaderField, const nsAString& aData)
{
  if (!aHeaderField) {
    NS_ERROR("null headerField");
    return;
  }

  if (!mHeaderData) {
    if (!aData.IsEmpty()) { 
      mHeaderData = new nsDocHeaderData(aHeaderField, aData);
    }
  }
  else {
    nsDocHeaderData* data = mHeaderData;
    nsDocHeaderData** lastPtr = &mHeaderData;
    PRBool found = PR_FALSE;
    do {  
      if (data->mField == aHeaderField) {
        if (!aData.IsEmpty()) {
          data->mData.Assign(aData);
        }
        else {  
          *lastPtr = data->mNext;
          data->mNext = nsnull;
          delete data;
        }
        found = PR_TRUE;

        break;
      }
      lastPtr = &(data->mNext);
      data = *lastPtr;
    } while (data);

    if (!aData.IsEmpty() && !found) {
      
      *lastPtr = new nsDocHeaderData(aHeaderField, aData);
    }
  }

  if (aHeaderField == nsGkAtoms::headerContentLanguage) {
    CopyUTF16toUTF8(aData, mContentLanguage);
  }

  
  if (aHeaderField == nsGkAtoms::headerContentScriptType) {
    nsIContent *root = GetRootContent();
    if (root) {
      
      nsresult rv;
      nsCOMPtr<nsIScriptRuntime> runtime;
      rv = NS_GetScriptRuntime(aData, getter_AddRefs(runtime));
      if (NS_FAILED(rv) || runtime == nsnull) {
        NS_WARNING("The script-type is unknown");
      } else {
        root->SetScriptTypeID(runtime->GetScriptTypeID());
      }
    }
  }

  if (aHeaderField == nsGkAtoms::headerDefaultStyle) {
    
    
    if (DOMStringIsNull(mLastStyleSheetSet)) {
      
      
      
      
      
      EnableStyleSheetsForSetInternal(aData, PR_TRUE);
    }
  }

  if (aHeaderField == nsGkAtoms::refresh) {
    
    
    nsCOMPtr<nsIRefreshURI> refresher = do_QueryReferent(mDocumentContainer);
    if (refresher) {
      
      
      
      
      
      refresher->SetupRefreshURIFromHeader(mDocumentURI,
                                           NS_ConvertUTF16toUTF8(aData));
    }
  }

  if (aHeaderField == nsGkAtoms::headerDNSPrefetchControl &&
      mAllowDNSPrefetch) {
    
    mAllowDNSPrefetch = aData.IsEmpty() || aData.LowerCaseEqualsLiteral("on");
  }
}

PRBool
nsDocument::TryChannelCharset(nsIChannel *aChannel,
                              PRInt32& aCharsetSource,
                              nsACString& aCharset)
{
  if(kCharsetFromChannel <= aCharsetSource) {
    return PR_TRUE;
  }

  if (aChannel) {
    nsCAutoString charsetVal;
    nsresult rv = aChannel->GetContentCharset(charsetVal);
    if (NS_SUCCEEDED(rv)) {
      nsCOMPtr<nsICharsetAlias> calias(do_GetService(NS_CHARSETALIAS_CONTRACTID));
      if (calias) {
        nsCAutoString preferred;
        rv = calias->GetPreferred(charsetVal,
                                  preferred);
        if(NS_SUCCEEDED(rv)) {
          aCharset = preferred;
          aCharsetSource = kCharsetFromChannel;
          return PR_TRUE;
        }
      }
    }
  }
  return PR_FALSE;
}

nsresult
nsDocument::CreateShell(nsPresContext* aContext, nsIViewManager* aViewManager,
                        nsStyleSet* aStyleSet,
                        nsIPresShell** aInstancePtrResult)
{
  
  
  
  return doCreateShell(aContext, aViewManager, aStyleSet,
                       eCompatibility_FullStandards, aInstancePtrResult);
}

nsresult
nsDocument::doCreateShell(nsPresContext* aContext,
                          nsIViewManager* aViewManager, nsStyleSet* aStyleSet,
                          nsCompatibility aCompatMode,
                          nsIPresShell** aInstancePtrResult)
{
  *aInstancePtrResult = nsnull;

  NS_ENSURE_FALSE(mShellsAreHidden, NS_ERROR_FAILURE);

  FillStyleSet(aStyleSet);
  
  nsCOMPtr<nsIPresShell> shell;
  nsresult rv = NS_NewPresShell(getter_AddRefs(shell));
  if (NS_FAILED(rv)) {
    return rv;
  }

  rv = shell->Init(this, aContext, aViewManager, aStyleSet, aCompatMode);
  NS_ENSURE_SUCCESS(rv, rv);

  
  NS_ENSURE_TRUE(mPresShells.AppendElementUnlessExists(shell),
                 NS_ERROR_OUT_OF_MEMORY);
  shell.swap(*aInstancePtrResult);

  return NS_OK;
}

PRBool
nsDocument::DeleteShell(nsIPresShell* aShell)
{
  return mPresShells.RemoveElement(aShell);
}


nsIPresShell *
nsDocument::GetPrimaryShell() const
{
  return mShellsAreHidden ? nsnull : mPresShells.SafeElementAt(0, nsnull);
}

static void
SubDocClearEntry(PLDHashTable *table, PLDHashEntryHdr *entry)
{
  SubDocMapEntry *e = static_cast<SubDocMapEntry *>(entry);

  NS_RELEASE(e->mKey);
  if (e->mSubDocument) {
    e->mSubDocument->SetParentDocument(nsnull);
    NS_RELEASE(e->mSubDocument);
  }
}

static PRBool
SubDocInitEntry(PLDHashTable *table, PLDHashEntryHdr *entry, const void *key)
{
  SubDocMapEntry *e =
    const_cast<SubDocMapEntry *>
              (static_cast<const SubDocMapEntry *>(entry));

  e->mKey = const_cast<nsIContent *>
                      (static_cast<const nsIContent *>(key));
  NS_ADDREF(e->mKey);

  e->mSubDocument = nsnull;
  return PR_TRUE;
}

nsresult
nsDocument::SetSubDocumentFor(nsIContent *aContent, nsIDocument* aSubDoc)
{
  NS_ENSURE_TRUE(aContent, NS_ERROR_UNEXPECTED);

  if (!aSubDoc) {
    

    if (mSubDocuments) {
      SubDocMapEntry *entry =
        static_cast<SubDocMapEntry*>
                   (PL_DHashTableOperate(mSubDocuments, aContent,
                                            PL_DHASH_LOOKUP));

      if (PL_DHASH_ENTRY_IS_BUSY(entry)) {
        PL_DHashTableRawRemove(mSubDocuments, entry);
      }
    }
  } else {
    if (!mSubDocuments) {
      

      static PLDHashTableOps hash_table_ops =
      {
        PL_DHashAllocTable,
        PL_DHashFreeTable,
        PL_DHashVoidPtrKeyStub,
        PL_DHashMatchEntryStub,
        PL_DHashMoveEntryStub,
        SubDocClearEntry,
        PL_DHashFinalizeStub,
        SubDocInitEntry
      };

      mSubDocuments = PL_NewDHashTable(&hash_table_ops, nsnull,
                                       sizeof(SubDocMapEntry), 16);
      if (!mSubDocuments) {
        return NS_ERROR_OUT_OF_MEMORY;
      }
    }

    
    SubDocMapEntry *entry =
      static_cast<SubDocMapEntry*>
                 (PL_DHashTableOperate(mSubDocuments, aContent,
                                          PL_DHASH_ADD));

    if (!entry) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    if (entry->mSubDocument) {
      entry->mSubDocument->SetParentDocument(nsnull);

      
      NS_RELEASE(entry->mSubDocument);
    }

    entry->mSubDocument = aSubDoc;
    NS_ADDREF(entry->mSubDocument);

    aSubDoc->SetParentDocument(this);
  }

  return NS_OK;
}

nsIDocument*
nsDocument::GetSubDocumentFor(nsIContent *aContent) const
{
  if (mSubDocuments) {
    SubDocMapEntry *entry =
      static_cast<SubDocMapEntry*>
                 (PL_DHashTableOperate(mSubDocuments, aContent,
                                          PL_DHASH_LOOKUP));

    if (PL_DHASH_ENTRY_IS_BUSY(entry)) {
      return entry->mSubDocument;
    }
  }

  return nsnull;
}

static PLDHashOperator
FindContentEnumerator(PLDHashTable *table, PLDHashEntryHdr *hdr,
                      PRUint32 number, void *arg)
{
  SubDocMapEntry *entry = static_cast<SubDocMapEntry*>(hdr);
  FindContentData *data = static_cast<FindContentData*>(arg);

  if (entry->mSubDocument == data->mSubDocument) {
    data->mResult = entry->mKey;

    return PL_DHASH_STOP;
  }

  return PL_DHASH_NEXT;
}

nsIContent*
nsDocument::FindContentForSubDocument(nsIDocument *aDocument) const
{
  NS_ENSURE_TRUE(aDocument, nsnull);

  if (!mSubDocuments) {
    return nsnull;
  }

  FindContentData data(aDocument);
  PL_DHashTableEnumerate(mSubDocuments, FindContentEnumerator, &data);

  return data.mResult;
}

PRBool
nsDocument::IsNodeOfType(PRUint32 aFlags) const
{
    return !(aFlags & ~eDOCUMENT);
}

nsIContent*
nsDocument::GetRootContentInternal() const
{
  
  
  PRUint32 i;
  for (i = mChildren.ChildCount(); i > 0; --i) {
    nsIContent* child = mChildren.ChildAt(i - 1);
    if (child->IsNodeOfType(nsINode::eELEMENT)) {
      const_cast<nsDocument*>(this)->mCachedRootContent = child;
      return child;
    }
  }
  
  const_cast<nsDocument*>(this)->mCachedRootContent = nsnull;
  return nsnull;
}

nsIContent *
nsDocument::GetChildAt(PRUint32 aIndex) const
{
  return mChildren.GetSafeChildAt(aIndex);
}

PRInt32
nsDocument::IndexOf(nsINode* aPossibleChild) const
{
  return mChildren.IndexOfChild(aPossibleChild);
}

PRUint32
nsDocument::GetChildCount() const
{
  return mChildren.ChildCount();
}

nsIContent * const *
nsDocument::GetChildArray(PRUint32* aChildCount) const
{
  return mChildren.GetChildArray(aChildCount);
}
  

nsresult
nsDocument::InsertChildAt(nsIContent* aKid, PRUint32 aIndex,
                          PRBool aNotify)
{
  if (aKid->IsNodeOfType(nsINode::eELEMENT) &&
      GetRootContent()) {
    NS_ERROR("Inserting element child when we already have one");
    return NS_ERROR_DOM_HIERARCHY_REQUEST_ERR;
  }

  return nsGenericElement::doInsertChildAt(aKid, aIndex, aNotify,
                                           nsnull, this, mChildren);
}

nsresult
nsDocument::AppendChildTo(nsIContent* aKid, PRBool aNotify)
{
  
  
  
  
  
  return nsDocument::InsertChildAt(aKid, GetChildCount(), aNotify);
}

nsresult
nsDocument::RemoveChildAt(PRUint32 aIndex, PRBool aNotify, PRBool aMutationEvent)
{
  NS_ASSERTION(aMutationEvent, "Someone tried to inhibit mutations on document child removal.");
  nsCOMPtr<nsIContent> oldKid = GetChildAt(aIndex);
  if (!oldKid) {
    return NS_OK;
  }

  if (oldKid->IsNodeOfType(nsINode::eELEMENT)) {
    
    DestroyLinkMap();
  }

  nsresult rv = nsGenericElement::doRemoveChildAt(aIndex, aNotify, oldKid,
                                                  nsnull, this, mChildren, 
                                                  aMutationEvent);
  mCachedRootContent = nsnull;
  return rv;
}

PRInt32
nsDocument::GetNumberOfStyleSheets() const
{
  return mStyleSheets.Count();
}

nsIStyleSheet*
nsDocument::GetStyleSheetAt(PRInt32 aIndex) const
{
  NS_ENSURE_TRUE(0 <= aIndex && aIndex < mStyleSheets.Count(), nsnull);
  return mStyleSheets[aIndex];
}

PRInt32
nsDocument::GetIndexOfStyleSheet(nsIStyleSheet* aSheet) const
{
  return mStyleSheets.IndexOf(aSheet);
}

void
nsDocument::AddStyleSheetToStyleSets(nsIStyleSheet* aSheet)
{
  nsPresShellIterator iter(this);
  nsCOMPtr<nsIPresShell> shell;
  while ((shell = iter.GetNextShell())) {
    shell->StyleSet()->AddDocStyleSheet(aSheet, this);
  }
}

void
nsDocument::AddStyleSheet(nsIStyleSheet* aSheet)
{
  NS_PRECONDITION(aSheet, "null arg");
  mStyleSheets.AppendObject(aSheet);
  aSheet->SetOwningDocument(this);

  PRBool applicable;
  aSheet->GetApplicable(applicable);

  if (applicable) {
    AddStyleSheetToStyleSets(aSheet);
  }

  NS_DOCUMENT_NOTIFY_OBSERVERS(StyleSheetAdded, (this, aSheet, PR_TRUE));
}

void
nsDocument::RemoveStyleSheetFromStyleSets(nsIStyleSheet* aSheet)
{
  nsPresShellIterator iter(this);
  nsCOMPtr<nsIPresShell> shell;
  while ((shell = iter.GetNextShell())) {
    shell->StyleSet()->RemoveStyleSheet(nsStyleSet::eDocSheet, aSheet);
  }
}

void
nsDocument::RemoveStyleSheet(nsIStyleSheet* aSheet)
{
  NS_PRECONDITION(aSheet, "null arg");
  nsCOMPtr<nsIStyleSheet> sheet = aSheet; 

  if (!mStyleSheets.RemoveObject(aSheet)) {
    NS_NOTREACHED("stylesheet not found");
    return;
  }

  if (!mIsGoingAway) {
    PRBool applicable = PR_TRUE;
    aSheet->GetApplicable(applicable);
    if (applicable) {
      RemoveStyleSheetFromStyleSets(aSheet);
    }

    NS_DOCUMENT_NOTIFY_OBSERVERS(StyleSheetRemoved, (this, aSheet, PR_TRUE));
  }

  aSheet->SetOwningDocument(nsnull);
}

void
nsDocument::UpdateStyleSheets(nsCOMArray<nsIStyleSheet>& aOldSheets,
                              nsCOMArray<nsIStyleSheet>& aNewSheets)
{
  BeginUpdate(UPDATE_STYLE);

  
  NS_PRECONDITION(aOldSheets.Count() == aNewSheets.Count(),
                  "The lists must be the same length!");
  PRInt32 count = aOldSheets.Count();

  nsCOMPtr<nsIStyleSheet> oldSheet;
  PRInt32 i;
  for (i = 0; i < count; ++i) {
    oldSheet = aOldSheets[i];

    
    NS_ASSERTION(oldSheet, "None of the old sheets should be null");
    PRInt32 oldIndex = mStyleSheets.IndexOf(oldSheet);
    RemoveStyleSheet(oldSheet);  

    
    nsIStyleSheet* newSheet = aNewSheets[i];
    if (newSheet) {
      mStyleSheets.InsertObjectAt(newSheet, oldIndex);
      newSheet->SetOwningDocument(this);
      PRBool applicable = PR_TRUE;
      newSheet->GetApplicable(applicable);
      if (applicable) {
        AddStyleSheetToStyleSets(newSheet);
      }

      NS_DOCUMENT_NOTIFY_OBSERVERS(StyleSheetAdded, (this, newSheet, PR_TRUE));
    }
  }

  EndUpdate(UPDATE_STYLE);
}

void
nsDocument::InsertStyleSheetAt(nsIStyleSheet* aSheet, PRInt32 aIndex)
{
  NS_PRECONDITION(aSheet, "null ptr");
  mStyleSheets.InsertObjectAt(aSheet, aIndex);

  aSheet->SetOwningDocument(this);

  PRBool applicable;
  aSheet->GetApplicable(applicable);

  if (applicable) {
    AddStyleSheetToStyleSets(aSheet);
  }

  NS_DOCUMENT_NOTIFY_OBSERVERS(StyleSheetAdded, (this, aSheet, PR_TRUE));
}


void
nsDocument::SetStyleSheetApplicableState(nsIStyleSheet* aSheet,
                                         PRBool aApplicable)
{
  NS_PRECONDITION(aSheet, "null arg");

  
  if (-1 != mStyleSheets.IndexOf(aSheet)) {
    if (aApplicable) {
      AddStyleSheetToStyleSets(aSheet);
    } else {
      RemoveStyleSheetFromStyleSets(aSheet);
    }
  }

  
  
  

  NS_DOCUMENT_NOTIFY_OBSERVERS(StyleSheetApplicableStateChanged,
                               (this, aSheet, aApplicable));
}




PRInt32
nsDocument::GetNumberOfCatalogStyleSheets() const
{
  return mCatalogSheets.Count();
}

nsIStyleSheet*
nsDocument::GetCatalogStyleSheetAt(PRInt32 aIndex) const
{
  NS_ENSURE_TRUE(0 <= aIndex && aIndex < mCatalogSheets.Count(), nsnull);
  return mCatalogSheets[aIndex];
}

void
nsDocument::AddCatalogStyleSheet(nsIStyleSheet* aSheet)
{
  mCatalogSheets.AppendObject(aSheet);
  aSheet->SetOwningDocument(this);

  PRBool applicable;
  aSheet->GetApplicable(applicable);
                                                                                
  if (applicable) {
    
    nsPresShellIterator iter(this);
    nsCOMPtr<nsIPresShell> shell;
    while ((shell = iter.GetNextShell())) {
      shell->StyleSet()->AppendStyleSheet(nsStyleSet::eAgentSheet, aSheet);
    }
  }
                                                                                
  NS_DOCUMENT_NOTIFY_OBSERVERS(StyleSheetAdded, (this, aSheet, PR_FALSE));
}

void
nsDocument::EnsureCatalogStyleSheet(const char *aStyleSheetURI)
{
  nsICSSLoader* cssLoader = CSSLoader();
  PRBool enabled;
  if (NS_SUCCEEDED(cssLoader->GetEnabled(&enabled)) && enabled) {
    PRInt32 sheetCount = GetNumberOfCatalogStyleSheets();
    for (PRInt32 i = 0; i < sheetCount; i++) {
      nsIStyleSheet* sheet = GetCatalogStyleSheetAt(i);
      NS_ASSERTION(sheet, "unexpected null stylesheet in the document");
      if (sheet) {
        nsCOMPtr<nsIURI> uri;
        sheet->GetSheetURI(getter_AddRefs(uri));
        nsCAutoString uriStr;
        uri->GetSpec(uriStr);
        if (uriStr.Equals(aStyleSheetURI))
          return;
      }
    }

    nsCOMPtr<nsIURI> uri;
    NS_NewURI(getter_AddRefs(uri), aStyleSheetURI);
    if (uri) {
      nsCOMPtr<nsICSSStyleSheet> sheet;
      cssLoader->LoadSheetSync(uri, PR_TRUE, PR_TRUE, getter_AddRefs(sheet));
      if (sheet) {
        BeginUpdate(UPDATE_STYLE);
        AddCatalogStyleSheet(sheet);
        EndUpdate(UPDATE_STYLE);
      }
    }
  }
}

nsIScriptGlobalObject*
nsDocument::GetScriptGlobalObject() const
{
   
   
   

   
   
   
   if (mRemovedFromDocShell) {
     nsCOMPtr<nsIInterfaceRequestor> requestor =
       do_QueryReferent(mDocumentContainer);
     if (requestor) {
       nsCOMPtr<nsIScriptGlobalObject> globalObject = do_GetInterface(requestor);
       return globalObject;
     }
   }

   return mScriptGlobalObject;
}

nsIScriptGlobalObject*
nsDocument::GetScopeObject()
{
  nsCOMPtr<nsIScriptGlobalObject> scope(do_QueryReferent(mScopeObject));
  return scope;
}

void
nsDocument::SetScriptGlobalObject(nsIScriptGlobalObject *aScriptGlobalObject)
{
#ifdef DEBUG
  {
    nsCOMPtr<nsPIDOMWindow> win(do_QueryInterface(aScriptGlobalObject));

    NS_ASSERTION(!win || win->IsInnerWindow(),
                 "Script global object must be an inner window!");
  }
#endif

  if (mScriptGlobalObject && !aScriptGlobalObject) {
    
    
    mLayoutHistoryState = GetLayoutHistoryState();

    
    if (mOnloadBlockCount != 0) {
      nsCOMPtr<nsILoadGroup> loadGroup = GetDocumentLoadGroup();
      if (loadGroup) {
        loadGroup->RemoveRequest(mOnloadBlocker, nsnull, NS_OK);
      }
    }
  }

  mScriptGlobalObject = aScriptGlobalObject;

  if (aScriptGlobalObject) {
    mScriptObject = nsnull;
    mHasHadScriptHandlingObject = PR_TRUE;
    
    mLayoutHistoryState = nsnull;
    mScopeObject = do_GetWeakReference(aScriptGlobalObject);

#ifdef DEBUG
    
    
    JSObject *obj = GetWrapper();
    if (obj) {
      JSObject *newScope = aScriptGlobalObject->GetGlobalJSObject();
      nsIScriptContext *scx = aScriptGlobalObject->GetContext();
      JSContext *cx = scx ? (JSContext *)scx->GetNativeContext() : nsnull;
      if (!cx) {
        nsContentUtils::ThreadJSContextStack()->Peek(&cx);
        if (!cx) {
          nsContentUtils::ThreadJSContextStack()->GetSafeJSContext(&cx);
          NS_ASSERTION(cx, "Uhoh, no context, this is bad!");
        }
      }
      if (cx) {
        NS_ASSERTION(JS_GetGlobalForObject(cx, obj) == newScope,
                     "Wrong scope, this is really bad!");
      }
    }
#endif

    if (mAllowDNSPrefetch) {
      nsCOMPtr<nsIDocShell> docShell = do_QueryReferent(mDocumentContainer);
      if (docShell) {
#ifdef DEBUG
        nsCOMPtr<nsIWebNavigation> webNav =
          do_GetInterface(aScriptGlobalObject);
        NS_ASSERTION(SameCOMIdentity(webNav, docShell),
                     "Unexpected container or script global?");
#endif
        PRBool allowDNSPrefetch;
        docShell->GetAllowDNSPrefetch(&allowDNSPrefetch);
        mAllowDNSPrefetch = allowDNSPrefetch;
      }
    }
  }

  
  
  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(mScriptGlobalObject);
  mWindow = window;
}

nsIScriptGlobalObject*
nsDocument::GetScriptHandlingObject(PRBool& aHasHadScriptHandlingObject) const
{
  aHasHadScriptHandlingObject = mHasHadScriptHandlingObject;
  if (mScriptGlobalObject) {
    return mScriptGlobalObject;
  }

  nsCOMPtr<nsIScriptGlobalObject> scriptHandlingObject =
    do_QueryReferent(mScriptObject);
  nsCOMPtr<nsPIDOMWindow> win = do_QueryInterface(scriptHandlingObject);
  if (win) {
    NS_ASSERTION(win->IsInnerWindow(), "Should have inner window here!");
    nsPIDOMWindow* outer = win->GetOuterWindow();
    if (!outer || outer->GetCurrentInnerWindow() != win) {
      NS_WARNING("Wrong inner/outer window combination!");
      return nsnull;
    }
  }
  return scriptHandlingObject;
}
void
nsDocument::SetScriptHandlingObject(nsIScriptGlobalObject* aScriptObject)
{
  NS_ASSERTION(!mScriptGlobalObject ||
               mScriptGlobalObject == aScriptObject,
               "Wrong script object!");
  nsCOMPtr<nsPIDOMWindow> win = do_QueryInterface(aScriptObject);
  NS_ASSERTION(!win || win->IsInnerWindow(), "Should have inner window here!");
  mScopeObject = mScriptObject = do_GetWeakReference(aScriptObject);
  if (aScriptObject) {
    mHasHadScriptHandlingObject = PR_TRUE;
  }
}

nsPIDOMWindow *
nsDocument::GetWindow()
{
  if (mWindow) {
    return mWindow->GetOuterWindow();
  }

  nsCOMPtr<nsPIDOMWindow> win(do_QueryInterface(GetScriptGlobalObject()));

  if (!win) {
    return nsnull;
  }

  return win->GetOuterWindow();
}

nsPIDOMWindow *
nsDocument::GetInnerWindow()
{
  if (!mRemovedFromDocShell) {
    return mWindow;
  }

  nsCOMPtr<nsPIDOMWindow> win(do_QueryInterface(GetScriptGlobalObject()));

  return win;
}

nsScriptLoader*
nsDocument::ScriptLoader()
{
  return mScriptLoader;
}



void
nsDocument::AddObserver(nsIDocumentObserver* aObserver)
{
  
  mObservers.AppendElementUnlessExists(aObserver);
  AddMutationObserver(aObserver);
}

PRBool
nsDocument::RemoveObserver(nsIDocumentObserver* aObserver)
{
  
  
  
  
  if (!mInDestructor) {
    RemoveMutationObserver(aObserver);
    return mObservers.RemoveElement(aObserver);
  }

  return mObservers.Contains(aObserver);
}

void
nsDocument::MaybeEndOutermostXBLUpdate()
{
  
  
  if (mUpdateNestLevel == 0 && mInXBLUpdate) {
    if (nsContentUtils::IsSafeToRunScript()) {
      mInXBLUpdate = PR_FALSE;
      BindingManager()->EndOutermostUpdate();
    } else if (!mInDestructor) {
      nsContentUtils::AddScriptRunner(
        NS_NEW_RUNNABLE_METHOD(nsDocument, this, MaybeEndOutermostXBLUpdate));
    }
  }
}

void
nsDocument::BeginUpdate(nsUpdateType aUpdateType)
{
  if (mUpdateNestLevel == 0 && !mInXBLUpdate) {
    mInXBLUpdate = PR_TRUE;
    BindingManager()->BeginOutermostUpdate();
  }
  
  ++mUpdateNestLevel;
  if (aUpdateType == UPDATE_CONTENT_MODEL) {
    nsContentUtils::AddRemovableScriptBlocker();
  }
  else {
    nsContentUtils::AddScriptBlocker();
  }
  NS_DOCUMENT_NOTIFY_OBSERVERS(BeginUpdate, (this, aUpdateType));
}

void
nsDocument::EndUpdate(nsUpdateType aUpdateType)
{
  NS_DOCUMENT_NOTIFY_OBSERVERS(EndUpdate, (this, aUpdateType));

  if (aUpdateType == UPDATE_CONTENT_MODEL) {
    nsContentUtils::RemoveRemovableScriptBlocker();
  }
  else {
    nsContentUtils::RemoveScriptBlocker();
  }

  --mUpdateNestLevel;

  
  
  MaybeEndOutermostXBLUpdate();

  MaybeInitializeFinalizeFrameLoaders();
}

void
nsDocument::BeginLoad()
{
  
  
  BlockOnload();

  if (mScriptLoader) {
    mScriptLoader->BeginDeferringScripts();
  }

  NS_DOCUMENT_NOTIFY_OBSERVERS(BeginLoad, (this));
}

PRBool
nsDocument::CheckGetElementByIdArg(const nsIAtom* aId)
{
  if (aId == nsGkAtoms::_empty) {
    nsContentUtils::ReportToConsole(
        nsContentUtils::eDOM_PROPERTIES,
        "EmptyGetElementByIdParam",
        nsnull, 0,
        nsnull,
        EmptyString(), 0, 0,
        nsIScriptError::warningFlag,
        "DOM");
    return PR_FALSE;
  }
  return PR_TRUE;
}

nsIdentifierMapEntry*
nsDocument::GetElementByIdInternal(nsIAtom* aID)
{
  
  
  
  
  nsIdentifierMapEntry *entry = mIdentifierMap.PutEntry(aID);
  NS_ENSURE_TRUE(entry, nsnull);

  if (entry->GetIdContent())
    return entry;

  
  
  

  
  
  PRUint32 generation = mIdentifierMap.GetGeneration();
  
  FlushPendingNotifications(Flush_ContentAndNotify);

  if (generation != mIdentifierMap.GetGeneration()) {
    
    
    
    entry = mIdentifierMap.PutEntry(aID);
  }
  
  return entry;
}

NS_IMETHODIMP
nsDocument::GetElementById(const nsAString& aElementId,
                           nsIDOMElement** aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);
  *aReturn = nsnull;

  nsCOMPtr<nsIAtom> idAtom(do_GetAtom(aElementId));
  NS_ENSURE_TRUE(idAtom, NS_ERROR_OUT_OF_MEMORY);
  if (!CheckGetElementByIdArg(idAtom))
    return NS_OK;

  nsIdentifierMapEntry *entry = GetElementByIdInternal(idAtom);
  NS_ENSURE_TRUE(entry, NS_ERROR_OUT_OF_MEMORY);

  nsIContent *e = entry->GetIdContent();
  if (!e)
    return NS_OK;

  return CallQueryInterface(e, aReturn);
}

nsIContent*
nsDocument::AddIDTargetObserver(nsIAtom* aID, IDTargetObserver aObserver,
                                void* aData)
{
  if (!CheckGetElementByIdArg(aID))
    return nsnull;

  nsIdentifierMapEntry *entry = GetElementByIdInternal(aID);
  NS_ENSURE_TRUE(entry, nsnull);

  entry->AddContentChangeCallback(aObserver, aData);
  return entry->GetIdContent();
}

void
nsDocument::RemoveIDTargetObserver(nsIAtom* aID,
                                   IDTargetObserver aObserver, void* aData)
{
  if (!CheckGetElementByIdArg(aID))
    return;

  nsIdentifierMapEntry *entry = mIdentifierMap.GetEntry(aID);
  if (!entry) {
    
    
    
    return;
  }

  entry->RemoveContentChangeCallback(aObserver, aData);
}

void
nsDocument::DispatchContentLoadedEvents()
{
  
  
  
  
  mPreloadingImages.Clear();
    
  
  
  
  nsContentUtils::DispatchTrustedEvent(this, static_cast<nsIDocument*>(this),
                                       NS_LITERAL_STRING("DOMContentLoaded"),
                                       PR_TRUE, PR_TRUE);

  
  
  
  

  
  
  
  nsCOMPtr<nsIDOMEventTarget> target_frame;

  if (mParentDocument) {
    target_frame =
      do_QueryInterface(mParentDocument->FindContentForSubDocument(this));
  }

  if (target_frame) {
    nsCOMPtr<nsIDocument> parent = mParentDocument;
    do {
      nsCOMPtr<nsIDOMDocumentEvent> document_event =
        do_QueryInterface(parent);

      nsCOMPtr<nsIDOMEvent> event;
      nsCOMPtr<nsIPrivateDOMEvent> privateEvent;
      if (document_event) {
        document_event->CreateEvent(NS_LITERAL_STRING("Events"),
                                    getter_AddRefs(event));

        privateEvent = do_QueryInterface(event);
      }

      if (event && privateEvent) {
        event->InitEvent(NS_LITERAL_STRING("DOMFrameContentLoaded"), PR_TRUE,
                         PR_TRUE);

        privateEvent->SetTarget(target_frame);
        privateEvent->SetTrusted(PR_TRUE);

        
        
        
        
        

        nsEvent* innerEvent = privateEvent->GetInternalNSEvent();
        if (innerEvent) {
          nsEventStatus status = nsEventStatus_eIgnore;

          nsIPresShell *shell = parent->GetPrimaryShell();
          if (shell) {
            nsCOMPtr<nsPresContext> context = shell->GetPresContext();

            if (context) {
              nsEventDispatcher::Dispatch(parent, context, innerEvent, event,
                                          &status);
            }
          }
        }
      }
      
      parent = parent->GetParentDocument();
    } while (parent);
  }

  
  
  nsIContent* root = GetRootContent();
  if (root && root->HasAttr(kNameSpaceID_None, nsGkAtoms::manifest)) {
    nsContentUtils::DispatchChromeEvent(this, static_cast<nsIDocument*>(this),
                                        NS_LITERAL_STRING("MozApplicationManifest"),
                                        PR_TRUE, PR_TRUE);
  }

  UnblockOnload(PR_TRUE);
}

void
nsDocument::EndLoad()
{
  
  
  
  if (mParser) {
    mWeakSink = do_GetWeakReference(mParser->GetContentSink());
    mParser = nsnull;
  }
  
  NS_DOCUMENT_NOTIFY_OBSERVERS(EndLoad, (this));
  
  if (!mSynchronousDOMContentLoaded) {
    nsRefPtr<nsIRunnable> ev =
      new nsRunnableMethod<nsDocument>(this,
                                       &nsDocument::DispatchContentLoadedEvents);
    NS_DispatchToCurrentThread(ev);
  } else {
    DispatchContentLoadedEvents();
  }
}

void
nsDocument::ContentStatesChanged(nsIContent* aContent1, nsIContent* aContent2,
                                 PRInt32 aStateMask)
{
  NS_DOCUMENT_NOTIFY_OBSERVERS(ContentStatesChanged,
                               (this, aContent1, aContent2, aStateMask));
}

void
nsDocument::StyleRuleChanged(nsIStyleSheet* aStyleSheet,
                             nsIStyleRule* aOldStyleRule,
                             nsIStyleRule* aNewStyleRule)
{
  NS_DOCUMENT_NOTIFY_OBSERVERS(StyleRuleChanged,
                               (this, aStyleSheet,
                                aOldStyleRule, aNewStyleRule));
}

void
nsDocument::StyleRuleAdded(nsIStyleSheet* aStyleSheet,
                           nsIStyleRule* aStyleRule)
{
  NS_DOCUMENT_NOTIFY_OBSERVERS(StyleRuleAdded,
                               (this, aStyleSheet, aStyleRule));
}

void
nsDocument::StyleRuleRemoved(nsIStyleSheet* aStyleSheet,
                             nsIStyleRule* aStyleRule)
{
  NS_DOCUMENT_NOTIFY_OBSERVERS(StyleRuleRemoved,
                               (this, aStyleSheet, aStyleRule));
}





NS_IMETHODIMP
nsDocument::GetDoctype(nsIDOMDocumentType** aDoctype)
{
  NS_ENSURE_ARG_POINTER(aDoctype);

  *aDoctype = nsnull;
  PRInt32 i, count;
  count = mChildren.ChildCount();
  for (i = 0; i < count; i++) {
    CallQueryInterface(mChildren.ChildAt(i), aDoctype);

    if (*aDoctype) {
      return NS_OK;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetImplementation(nsIDOMDOMImplementation** aImplementation)
{
  
  
  nsCOMPtr<nsIURI> uri;
  NS_NewURI(getter_AddRefs(uri), "about:blank");
  NS_ENSURE_TRUE(uri, NS_ERROR_OUT_OF_MEMORY);
  PRBool hasHadScriptObject = PR_TRUE;
  nsIScriptGlobalObject* scriptObject =
    GetScriptHandlingObject(hasHadScriptObject);
  NS_ENSURE_STATE(scriptObject || !hasHadScriptObject);
  *aImplementation = new nsDOMImplementation(scriptObject, uri, uri,
                                             NodePrincipal());
  if (!*aImplementation) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_ADDREF(*aImplementation);

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetDocumentElement(nsIDOMElement** aDocumentElement)
{
  NS_ENSURE_ARG_POINTER(aDocumentElement);

  nsIContent* root = GetRootContent();
  if (root) {
    return CallQueryInterface(root, aDocumentElement);
  }

  *aDocumentElement = nsnull;

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::CreateElement(const nsAString& aTagName,
                          nsIDOMElement** aReturn)
{
  *aReturn = nsnull;

  nsresult rv = nsContentUtils::CheckQName(aTagName, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ASSERTION(IsCaseSensitive(),
               "nsDocument::CreateElement() called on document that is not "
               "case sensitive. Fix caller, or fix "
               "nsDocument::CreateElement()!");

  nsCOMPtr<nsIAtom> name = do_GetAtom(aTagName);

  nsCOMPtr<nsIContent> content;
  rv = CreateElem(name, nsnull, GetDefaultNamespaceID(), PR_TRUE,
                  getter_AddRefs(content));
  NS_ENSURE_SUCCESS(rv, rv);

  return CallQueryInterface(content, aReturn);
}

NS_IMETHODIMP
nsDocument::CreateElementNS(const nsAString& aNamespaceURI,
                            const nsAString& aQualifiedName,
                            nsIDOMElement** aReturn)
{
  *aReturn = nsnull;

  nsCOMPtr<nsINodeInfo> nodeInfo;
  nsresult rv = nsContentUtils::GetNodeInfoFromQName(aNamespaceURI,
                                                     aQualifiedName,
                                                     mNodeInfoManager,
                                                     getter_AddRefs(nodeInfo));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIContent> content;
  NS_NewElement(getter_AddRefs(content), nodeInfo->NamespaceID(), nodeInfo,
                PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  return CallQueryInterface(content, aReturn);
}

NS_IMETHODIMP
nsDocument::CreateTextNode(const nsAString& aData, nsIDOMText** aReturn)
{
  *aReturn = nsnull;

  nsCOMPtr<nsIContent> text;
  nsresult rv = NS_NewTextNode(getter_AddRefs(text), mNodeInfoManager);

  if (NS_SUCCEEDED(rv)) {
    
    text->SetText(aData, PR_FALSE);

    rv = CallQueryInterface(text, aReturn);
  }

  return rv;
}

NS_IMETHODIMP
nsDocument::CreateDocumentFragment(nsIDOMDocumentFragment** aReturn)
{
  return NS_NewDocumentFragment(aReturn, mNodeInfoManager);
}

NS_IMETHODIMP
nsDocument::CreateComment(const nsAString& aData, nsIDOMComment** aReturn)
{
  *aReturn = nsnull;

  
  
  if (FindInReadable(NS_LITERAL_STRING("--"), aData)) {
    return NS_ERROR_DOM_INVALID_CHARACTER_ERR;
  }

  nsCOMPtr<nsIContent> comment;
  nsresult rv = NS_NewCommentNode(getter_AddRefs(comment), mNodeInfoManager);

  if (NS_SUCCEEDED(rv)) {
    
    comment->SetText(aData, PR_FALSE);

    rv = CallQueryInterface(comment, aReturn);
  }

  return rv;
}

NS_IMETHODIMP
nsDocument::CreateCDATASection(const nsAString& aData,
                               nsIDOMCDATASection** aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);
  *aReturn = nsnull;

  if (FindInReadable(NS_LITERAL_STRING("]]>"), aData))
    return NS_ERROR_DOM_INVALID_CHARACTER_ERR;

  nsCOMPtr<nsIContent> content;
  nsresult rv = NS_NewXMLCDATASection(getter_AddRefs(content),
                                      mNodeInfoManager);

  if (NS_SUCCEEDED(rv)) {
    
    content->SetText(aData, PR_FALSE);

    rv = CallQueryInterface(content, aReturn);
  }

  return rv;
}

NS_IMETHODIMP
nsDocument::CreateProcessingInstruction(const nsAString& aTarget,
                                        const nsAString& aData,
                                        nsIDOMProcessingInstruction** aReturn)
{
  *aReturn = nsnull;

  nsresult rv = nsContentUtils::CheckQName(aTarget, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  if (FindInReadable(NS_LITERAL_STRING("?>"), aData)) {
    return NS_ERROR_DOM_INVALID_CHARACTER_ERR;
  }

  nsCOMPtr<nsIContent> content;
  rv = NS_NewXMLProcessingInstruction(getter_AddRefs(content),
                                      mNodeInfoManager, aTarget, aData);
  if (NS_FAILED(rv)) {
    return rv;
  }

  return CallQueryInterface(content, aReturn);
}

NS_IMETHODIMP
nsDocument::CreateAttribute(const nsAString& aName,
                            nsIDOMAttr** aReturn)
{
  *aReturn = nsnull;
  NS_ENSURE_TRUE(mNodeInfoManager, NS_ERROR_NOT_INITIALIZED);

  nsresult rv = nsContentUtils::CheckQName(aName, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString value;
  nsDOMAttribute* attribute;

  nsCOMPtr<nsINodeInfo> nodeInfo;
  rv = mNodeInfoManager->GetNodeInfo(aName, nsnull, kNameSpaceID_None,
                                     getter_AddRefs(nodeInfo));
  NS_ENSURE_SUCCESS(rv, rv);

  attribute = new nsDOMAttribute(nsnull, nodeInfo, value);
  NS_ENSURE_TRUE(attribute, NS_ERROR_OUT_OF_MEMORY);

  return CallQueryInterface(attribute, aReturn);
}

NS_IMETHODIMP
nsDocument::CreateAttributeNS(const nsAString & aNamespaceURI,
                              const nsAString & aQualifiedName,
                              nsIDOMAttr **aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = nsnull;

  nsCOMPtr<nsINodeInfo> nodeInfo;
  nsresult rv = nsContentUtils::GetNodeInfoFromQName(aNamespaceURI,
                                                     aQualifiedName,
                                                     mNodeInfoManager,
                                                     getter_AddRefs(nodeInfo));
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString value;
  nsDOMAttribute* attribute = new nsDOMAttribute(nsnull, nodeInfo, value);
  NS_ENSURE_TRUE(attribute, NS_ERROR_OUT_OF_MEMORY);

  return CallQueryInterface(attribute, aResult);
}

NS_IMETHODIMP
nsDocument::CreateEntityReference(const nsAString& aName,
                                  nsIDOMEntityReference** aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);

  *aReturn = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetElementsByTagName(const nsAString& aTagname,
                                 nsIDOMNodeList** aReturn)
{
  nsCOMPtr<nsIAtom> nameAtom = do_GetAtom(aTagname);
  NS_ENSURE_TRUE(nameAtom, NS_ERROR_OUT_OF_MEMORY);

  nsContentList *list = NS_GetContentList(this, nameAtom, kNameSpaceID_Unknown).get();
  NS_ENSURE_TRUE(list, NS_ERROR_OUT_OF_MEMORY);

  
  *aReturn = list;
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetElementsByTagNameNS(const nsAString& aNamespaceURI,
                                   const nsAString& aLocalName,
                                   nsIDOMNodeList** aReturn)
{
  PRInt32 nameSpaceId = kNameSpaceID_Wildcard;

  if (!aNamespaceURI.EqualsLiteral("*")) {
    nsresult rv =
      nsContentUtils::NameSpaceManager()->RegisterNameSpace(aNamespaceURI,
                                                            nameSpaceId);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<nsIAtom> nameAtom = do_GetAtom(aLocalName);
  NS_ENSURE_TRUE(nameAtom, NS_ERROR_OUT_OF_MEMORY);

  nsContentList *list = NS_GetContentList(this, nameAtom, nameSpaceId).get();
  NS_ENSURE_TRUE(list, NS_ERROR_OUT_OF_MEMORY);

  
  *aReturn = list;
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetAsync(PRBool *aAsync)
{
  NS_ERROR("nsDocument::GetAsync() should be overriden by subclass!");

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDocument::SetAsync(PRBool aAsync)
{
  NS_ERROR("nsDocument::SetAsync() should be overriden by subclass!");

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDocument::Load(const nsAString& aUrl, PRBool *aReturn)
{
  NS_ERROR("nsDocument::Load() should be overriden by subclass!");

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDocument::EvaluateFIXptr(const nsAString& aExpression, nsIDOMRange **aRange)
{
  NS_ERROR("nsDocument::EvaluateFIXptr() should be overriden by subclass!");

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDocument::EvaluateXPointer(const nsAString& aExpression,
                             nsIXPointerResult **aResult)
{
  NS_ERROR("nsDocument::EvaluateXPointer() should be overriden by subclass!");

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDocument::GetStyleSheets(nsIDOMStyleSheetList** aStyleSheets)
{
  if (!mDOMStyleSheets) {
    mDOMStyleSheets = new nsDOMStyleSheetList(this);
    if (!mDOMStyleSheets) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  *aStyleSheets = mDOMStyleSheets;
  NS_ADDREF(*aStyleSheets);

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetSelectedStyleSheetSet(nsAString& aSheetSet)
{
  aSheetSet.Truncate();
  
  
  PRInt32 count = GetNumberOfStyleSheets();
  nsAutoString title;
  for (PRInt32 index = 0; index < count; index++) {
    nsIStyleSheet* sheet = GetStyleSheetAt(index);
    NS_ASSERTION(sheet, "Null sheet in sheet list!");

    nsCOMPtr<nsIDOMStyleSheet> domSheet = do_QueryInterface(sheet);
    NS_ASSERTION(domSheet, "Sheet must QI to nsIDOMStyleSheet");
    PRBool disabled;
    domSheet->GetDisabled(&disabled);
    if (disabled) {
      
      continue;
    }
    
    sheet->GetTitle(title);

    if (aSheetSet.IsEmpty()) {
      aSheetSet = title;
    } else if (!title.IsEmpty() && !aSheetSet.Equals(title)) {
      
      SetDOMStringToNull(aSheetSet);
      break;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::SetSelectedStyleSheetSet(const nsAString& aSheetSet)
{
  if (DOMStringIsNull(aSheetSet)) {
    return NS_OK;
  }

  
  
  mLastStyleSheetSet = aSheetSet;
  EnableStyleSheetsForSetInternal(aSheetSet, PR_TRUE);
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetLastStyleSheetSet(nsAString& aSheetSet)
{
  aSheetSet = mLastStyleSheetSet;
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetPreferredStyleSheetSet(nsAString& aSheetSet)
{
  GetHeaderData(nsGkAtoms::headerDefaultStyle, aSheetSet);
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetStyleSheetSets(nsIDOMDOMStringList** aList)
{
  if (!mStyleSheetSetList) {
    mStyleSheetSetList = new nsDOMStyleSheetSetList(this);
    if (!mStyleSheetSetList) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  NS_ADDREF(*aList = mStyleSheetSetList);
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::EnableStyleSheetsForSet(const nsAString& aSheetSet)
{
  
  if (!DOMStringIsNull(aSheetSet)) {
    
    
    
    
    EnableStyleSheetsForSetInternal(aSheetSet, PR_FALSE);
  }

  return NS_OK;
}

void
nsDocument::EnableStyleSheetsForSetInternal(const nsAString& aSheetSet,
                                            PRBool aUpdateCSSLoader)
{
  BeginUpdate(UPDATE_STYLE);
  PRInt32 count = GetNumberOfStyleSheets();
  nsAutoString title;
  for (PRInt32 index = 0; index < count; index++) {
    nsIStyleSheet* sheet = GetStyleSheetAt(index);
    NS_ASSERTION(sheet, "Null sheet in sheet list!");
    sheet->GetTitle(title);
    if (!title.IsEmpty()) {
      sheet->SetEnabled(title.Equals(aSheetSet));
    }
  }
  if (aUpdateCSSLoader) {
    CSSLoader()->SetPreferredSheet(aSheetSet);
  }
  EndUpdate(UPDATE_STYLE);
}

NS_IMETHODIMP
nsDocument::GetCharacterSet(nsAString& aCharacterSet)
{
  CopyASCIItoUTF16(GetDocumentCharacterSet(), aCharacterSet);
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::ImportNode(nsIDOMNode* aImportedNode,
                       PRBool aDeep,
                       nsIDOMNode** aResult)
{
  NS_ENSURE_ARG(aImportedNode);

  *aResult = nsnull;

  nsresult rv = nsContentUtils::CheckSameOrigin(this, aImportedNode);
  if (NS_FAILED(rv)) {
    return rv;
  }

  PRUint16 nodeType;
  aImportedNode->GetNodeType(&nodeType);
  switch (nodeType) {
    case nsIDOMNode::ATTRIBUTE_NODE:
    case nsIDOMNode::DOCUMENT_FRAGMENT_NODE:
    case nsIDOMNode::ELEMENT_NODE:
    case nsIDOMNode::PROCESSING_INSTRUCTION_NODE:
    case nsIDOMNode::TEXT_NODE:
    case nsIDOMNode::CDATA_SECTION_NODE:
    case nsIDOMNode::COMMENT_NODE:
    {
      nsCOMPtr<nsINode> imported = do_QueryInterface(aImportedNode);
      NS_ENSURE_TRUE(imported, NS_ERROR_FAILURE);

      nsCOMPtr<nsIDOMNode> newNode;
      nsCOMArray<nsINode> nodesWithProperties;
      rv = nsNodeUtils::Clone(imported, aDeep, mNodeInfoManager,
                              nodesWithProperties, getter_AddRefs(newNode));
      NS_ENSURE_SUCCESS(rv, rv);

      nsIDocument *ownerDoc = imported->GetOwnerDoc();
      if (ownerDoc) {
        rv = nsNodeUtils::CallUserDataHandlers(nodesWithProperties, ownerDoc,
                                               nsIDOMUserDataHandler::NODE_IMPORTED,
                                               PR_TRUE);
        NS_ENSURE_SUCCESS(rv, rv);
      }

      newNode.swap(*aResult);

      return NS_OK;
    }
    case nsIDOMNode::ENTITY_NODE:
    case nsIDOMNode::ENTITY_REFERENCE_NODE:
    case nsIDOMNode::NOTATION_NODE:
    {
      return NS_ERROR_NOT_IMPLEMENTED;
    }
    default:
    {
      NS_WARNING("Don't know how to clone this nodetype for importNode.");

      return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
    }
  }
}

NS_IMETHODIMP
nsDocument::AddBinding(nsIDOMElement* aContent, const nsAString& aURI)
{
  NS_ENSURE_ARG(aContent);
  
  nsresult rv = nsContentUtils::CheckSameOrigin(this, aContent);
  if (NS_FAILED(rv)) {
    return rv;
  }

  nsCOMPtr<nsIContent> content(do_QueryInterface(aContent));

  nsCOMPtr<nsIURI> uri;
  rv = NS_NewURI(getter_AddRefs(uri), aURI);
  if (NS_FAILED(rv)) {
    return rv;
  }

  
  nsCOMPtr<nsIPrincipal> subject;
  nsIScriptSecurityManager* secMan = nsContentUtils::GetSecurityManager();
  if (secMan) {
    rv = secMan->GetSubjectPrincipal(getter_AddRefs(subject));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (!subject) {
    
    
    subject = NodePrincipal();
  }
  
  return BindingManager()->AddLayeredBinding(content, uri, subject);
}

NS_IMETHODIMP
nsDocument::RemoveBinding(nsIDOMElement* aContent, const nsAString& aURI)
{
  NS_ENSURE_ARG(aContent);

  nsresult rv = nsContentUtils::CheckSameOrigin(this, aContent);
  if (NS_FAILED(rv)) {
    return rv;
  }

  nsCOMPtr<nsIURI> uri;
  rv = NS_NewURI(getter_AddRefs(uri), aURI);
  if (NS_FAILED(rv)) {
    return rv;
  }

  nsCOMPtr<nsIContent> content(do_QueryInterface(aContent));
  return BindingManager()->RemoveLayeredBinding(content, uri);
}

NS_IMETHODIMP
nsDocument::LoadBindingDocument(const nsAString& aURI)
{
  nsCOMPtr<nsIURI> uri;
  nsresult rv = NS_NewURI(getter_AddRefs(uri), aURI,
                          mCharacterSet.get(),
                          static_cast<nsIDocument *>(this)->GetBaseURI());
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIPrincipal> subject;
  nsIScriptSecurityManager* secMan = nsContentUtils::GetSecurityManager();
  if (secMan) {
    rv = secMan->GetSubjectPrincipal(getter_AddRefs(subject));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (!subject) {
    
    
    subject = NodePrincipal();
  }
  
  BindingManager()->LoadBindingDocument(this, uri, subject);

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetBindingParent(nsIDOMNode* aNode, nsIDOMElement** aResult)
{
  *aResult = nsnull;
  nsCOMPtr<nsIContent> content(do_QueryInterface(aNode));
  if (!content)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMElement> elt(do_QueryInterface(content->GetBindingParent()));
  NS_IF_ADDREF(*aResult = elt);
  return NS_OK;
}

static nsresult
GetElementByAttribute(nsIContent* aContent, nsIAtom* aAttrName,
                      const nsAString& aAttrValue, PRBool aUniversalMatch,
                      nsIDOMElement** aResult)
{
  if (aUniversalMatch ? aContent->HasAttr(kNameSpaceID_None, aAttrName) :
                        aContent->AttrValueIs(kNameSpaceID_None, aAttrName,
                                              aAttrValue, eCaseMatters)) {
    return CallQueryInterface(aContent, aResult);
  }

  PRUint32 childCount = aContent->GetChildCount();

  for (PRUint32 i = 0; i < childCount; ++i) {
    nsIContent *current = aContent->GetChildAt(i);

    GetElementByAttribute(current, aAttrName, aAttrValue, aUniversalMatch,
                          aResult);

    if (*aResult)
      return NS_OK;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetAnonymousElementByAttribute(nsIDOMElement* aElement,
                                           const nsAString& aAttrName,
                                           const nsAString& aAttrValue,
                                           nsIDOMElement** aResult)
{
  *aResult = nsnull;

  nsCOMPtr<nsIDOMNodeList> nodeList;
  GetAnonymousNodes(aElement, getter_AddRefs(nodeList));

  if (!nodeList)
    return NS_OK;

  nsCOMPtr<nsIAtom> attribute = do_GetAtom(aAttrName);

  PRUint32 length;
  nodeList->GetLength(&length);

  PRBool universalMatch = aAttrValue.EqualsLiteral("*");

  for (PRUint32 i = 0; i < length; ++i) {
    nsCOMPtr<nsIDOMNode> current;
    nodeList->Item(i, getter_AddRefs(current));

    nsCOMPtr<nsIContent> content(do_QueryInterface(current));

    GetElementByAttribute(content, attribute, aAttrValue, universalMatch,
                          aResult);
    if (*aResult)
      return NS_OK;
  }

  return NS_OK;
}


NS_IMETHODIMP
nsDocument::GetAnonymousNodes(nsIDOMElement* aElement,
                              nsIDOMNodeList** aResult)
{
  *aResult = nsnull;

  nsCOMPtr<nsIContent> content(do_QueryInterface(aElement));
  return BindingManager()->GetAnonymousNodesFor(content, aResult);
}

NS_IMETHODIMP
nsDocument::CreateRange(nsIDOMRange** aReturn)
{
  nsresult rv = NS_NewRange(aReturn);

  if (NS_SUCCEEDED(rv)) {
    (*aReturn)->SetStart(this, 0);
    (*aReturn)->SetEnd(this, 0);
  }

  return rv;
}

NS_IMETHODIMP
nsDocument::CreateNodeIterator(nsIDOMNode *aRoot,
                               PRUint32 aWhatToShow,
                               nsIDOMNodeFilter *aFilter,
                               PRBool aEntityReferenceExpansion,
                               nsIDOMNodeIterator **_retval)
{
  *_retval = nsnull;

  if (!aRoot)
    return NS_ERROR_DOM_NOT_SUPPORTED_ERR;

  nsresult rv = nsContentUtils::CheckSameOrigin(this, aRoot);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ENSURE_ARG_POINTER(_retval);

  nsCOMPtr<nsINode> root = do_QueryInterface(aRoot);
  NS_ENSURE_TRUE(root, NS_ERROR_DOM_NOT_SUPPORTED_ERR);

  nsNodeIterator *iterator = new nsNodeIterator(root,
                                                aWhatToShow,
                                                aFilter,
                                                aEntityReferenceExpansion);
  NS_ENSURE_TRUE(iterator, NS_ERROR_OUT_OF_MEMORY);

  NS_ADDREF(*_retval = iterator);

  return NS_OK; 
}

NS_IMETHODIMP
nsDocument::CreateTreeWalker(nsIDOMNode *aRoot,
                             PRUint32 aWhatToShow,
                             nsIDOMNodeFilter *aFilter,
                             PRBool aEntityReferenceExpansion,
                             nsIDOMTreeWalker **_retval)
{
  *_retval = nsnull;

  if (!aRoot)
    return NS_ERROR_DOM_NOT_SUPPORTED_ERR;

  nsresult rv = nsContentUtils::CheckSameOrigin(this, aRoot);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ENSURE_ARG_POINTER(_retval);

  nsCOMPtr<nsINode> root = do_QueryInterface(aRoot);
  NS_ENSURE_TRUE(root, NS_ERROR_DOM_NOT_SUPPORTED_ERR);

  nsTreeWalker* walker = new nsTreeWalker(root,
                                          aWhatToShow,
                                          aFilter,
                                          aEntityReferenceExpansion);
  NS_ENSURE_TRUE(walker, NS_ERROR_OUT_OF_MEMORY);

  NS_ADDREF(*_retval = walker);

  return NS_OK;
}


NS_IMETHODIMP
nsDocument::GetDefaultView(nsIDOMAbstractView** aDefaultView)
{
  nsPIDOMWindow* win = GetWindow();
  if (win) {
    return CallQueryInterface(win, aDefaultView);
  }

  *aDefaultView = nsnull;

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetLocation(nsIDOMLocation **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = nsnull;

  nsCOMPtr<nsIDOMWindowInternal> w(do_QueryInterface(mScriptGlobalObject));

  if (!w) {
    return NS_OK;
  }

  return w->GetLocation(_retval);
}

nsIContent*
nsDocument::GetHtmlContent()
{
  nsIContent* rootContent = GetRootContent();
  if (rootContent && rootContent->Tag() == nsGkAtoms::html &&
      rootContent->IsNodeOfType(nsINode::eHTML))
    return rootContent;
  return nsnull;
}

nsIContent*
nsDocument::GetHtmlChildContent(nsIAtom* aTag)
{
  nsIContent* html = GetHtmlContent();
  if (!html)
    return nsnull;

  
  
  for (PRUint32 i = 0; i < html->GetChildCount(); ++i) {
    nsIContent* result = html->GetChildAt(i);
    if (result->Tag() == aTag && result->IsNodeOfType(nsINode::eHTML))
      return result;
  }
  return nsnull;
}

nsIContent*
nsDocument::GetTitleContent(PRUint32 aNodeType)
{
  
  
  
  
  
  if (!mMayHaveTitleElement)
    return nsnull;

  nsRefPtr<nsContentList> list =
    NS_GetContentList(this, nsGkAtoms::title, kNameSpaceID_Unknown);
  if (!list)
    return nsnull;

  for (PRUint32 i = 0; ; ++i) {
    
    
    
    nsIContent* elem = list->Item(i, PR_FALSE);
    if (!elem)
      return nsnull;
    if (elem->IsNodeOfType(aNodeType))
      return elem;
  }
}

void
nsDocument::GetTitleFromElement(PRUint32 aNodeType, nsAString& aTitle)
{
  nsIContent* title = GetTitleContent(aNodeType);
  if (!title)
    return;
  nsContentUtils::GetNodeTextContent(title, PR_FALSE, aTitle);
}

NS_IMETHODIMP
nsDocument::GetTitle(nsAString& aTitle)
{
  aTitle.Truncate();

  nsIContent *rootContent = GetRootContent();
  if (!rootContent)
    return NS_OK;

  nsAutoString tmp;

  switch (rootContent->GetNameSpaceID()) {
#ifdef MOZ_XUL
    case kNameSpaceID_XUL:
      rootContent->GetAttr(kNameSpaceID_None, nsGkAtoms::title, tmp);
      break;
#endif
#ifdef MOZ_SVG
    case kNameSpaceID_SVG:
      if (rootContent->Tag() == nsGkAtoms::svg) {
        GetTitleFromElement(nsINode::eSVG, tmp);
        break;
      } 
#endif
    default:
      GetTitleFromElement(nsINode::eHTML, tmp);
      break;
  }

  tmp.CompressWhitespace();
  aTitle = tmp;
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::SetTitle(const nsAString& aTitle)
{
  nsIContent *rootContent = GetRootContent();
  if (!rootContent)
    return NS_OK;

  switch (rootContent->GetNameSpaceID()) {
#ifdef MOZ_SVG
    case kNameSpaceID_SVG:
      return NS_OK; 
#endif
#ifdef MOZ_XUL
    case kNameSpaceID_XUL:
      return rootContent->SetAttr(kNameSpaceID_None, nsGkAtoms::title,
                                  aTitle, PR_TRUE);
#endif
  }

  
  
  mozAutoDocUpdate updateBatch(this, UPDATE_CONTENT_MODEL, PR_TRUE);

  nsIContent* title = GetTitleContent(nsINode::eHTML);
  if (!title) {
    nsIContent *head = GetHeadContent();
    if (!head)
      return NS_OK;

    {
      nsCOMPtr<nsINodeInfo> titleInfo;
      titleInfo = mNodeInfoManager->GetNodeInfo(nsGkAtoms::title, nsnull,
                                                kNameSpaceID_None);
      if (!titleInfo)
        return NS_OK;
      title = NS_NewHTMLTitleElement(titleInfo);
      if (!title)
        return NS_OK;
    }

    head->AppendChildTo(title, PR_TRUE);
  }

  return nsContentUtils::SetNodeTextContent(title, aTitle, PR_FALSE);
}

void
nsDocument::NotifyPossibleTitleChange(PRBool aBoundTitleElement)
{
  if (aBoundTitleElement) {
    mMayHaveTitleElement = PR_TRUE;
  }
  if (mPendingTitleChangeEvent.IsPending())
    return;

  nsRefPtr<nsNonOwningRunnableMethod<nsDocument> > event =
      new nsNonOwningRunnableMethod<nsDocument>(this,
            &nsDocument::DoNotifyPossibleTitleChange);
  nsresult rv = NS_DispatchToCurrentThread(event);
  if (NS_SUCCEEDED(rv)) {
    mPendingTitleChangeEvent = event;
  }
}

void
nsDocument::DoNotifyPossibleTitleChange()
{
  mPendingTitleChangeEvent.Forget();
  mHaveFiredTitleChange = PR_TRUE;

  nsAutoString title;
  GetTitle(title);

  nsPresShellIterator iter(this);
  nsCOMPtr<nsIPresShell> shell;
  while ((shell = iter.GetNextShell())) {
    nsCOMPtr<nsISupports> container = shell->GetPresContext()->GetContainer();
    if (!container)
      continue;

    nsCOMPtr<nsIBaseWindow> docShellWin = do_QueryInterface(container);
    if (!docShellWin)
      continue;

    docShellWin->SetTitle(PromiseFlatString(title).get());
  }

  
  nsContentUtils::DispatchChromeEvent(this, static_cast<nsIDocument*>(this),
                                      NS_LITERAL_STRING("DOMTitleChanged"),
                                      PR_TRUE, PR_TRUE);
}

NS_IMETHODIMP
nsDocument::GetBoxObjectFor(nsIDOMElement* aElement, nsIBoxObject** aResult)
{
  nsCOMPtr<nsIContent> content(do_QueryInterface(aElement));
  NS_ENSURE_TRUE(content, NS_ERROR_UNEXPECTED);

  nsIDocument* doc = content->GetOwnerDoc();
  NS_ENSURE_TRUE(doc == this, NS_ERROR_DOM_WRONG_DOCUMENT_ERR);

  if (!mHasWarnedAboutBoxObjects && !content->IsNodeOfType(eXUL)) {
    mHasWarnedAboutBoxObjects = PR_TRUE;
    nsContentUtils::ReportToConsole(nsContentUtils::eDOM_PROPERTIES,
                                    "UseOfGetBoxObjectForWarning",
                                    nsnull, 0,
                                    static_cast<nsIDocument*>(this)->
                                      GetDocumentURI(),
                                    EmptyString(), 0, 0,
                                    nsIScriptError::warningFlag,
                                    "BoxObjects");
  }

  *aResult = nsnull;

  if (!mBoxObjectTable) {
    mBoxObjectTable = new nsInterfaceHashtable<nsVoidPtrHashKey, nsPIBoxObject>;
    if (mBoxObjectTable && !mBoxObjectTable->Init(12)) {
      mBoxObjectTable = nsnull;
    }
  } else {
    
    *aResult = mBoxObjectTable->GetWeak(content);
    if (*aResult) {
      NS_ADDREF(*aResult);
      return NS_OK;
    }
  }

  PRInt32 namespaceID;
  nsCOMPtr<nsIAtom> tag = BindingManager()->ResolveTag(content, &namespaceID);

  nsCAutoString contractID("@mozilla.org/layout/xul-boxobject");
  if (namespaceID == kNameSpaceID_XUL) {
    if (tag == nsGkAtoms::browser ||
        tag == nsGkAtoms::editor ||
        tag == nsGkAtoms::iframe)
      contractID += "-container";
    else if (tag == nsGkAtoms::menu)
      contractID += "-menu";
    else if (tag == nsGkAtoms::popup ||
             tag == nsGkAtoms::menupopup ||
             tag == nsGkAtoms::panel ||
             tag == nsGkAtoms::tooltip)
      contractID += "-popup";
    else if (tag == nsGkAtoms::tree)
      contractID += "-tree";
    else if (tag == nsGkAtoms::listbox)
      contractID += "-listbox";
    else if (tag == nsGkAtoms::scrollbox)
      contractID += "-scrollbox";
  }
  contractID += ";1";

  nsCOMPtr<nsPIBoxObject> boxObject(do_CreateInstance(contractID.get()));
  if (!boxObject)
    return NS_ERROR_FAILURE;

  boxObject->Init(content);

  if (mBoxObjectTable) {
    mBoxObjectTable->Put(content, boxObject.get());
  }

  *aResult = boxObject;
  NS_ADDREF(*aResult);

  return NS_OK;
}

void
nsDocument::ClearBoxObjectFor(nsIContent* aContent)
{
  if (mBoxObjectTable) {
    nsPIBoxObject *boxObject = mBoxObjectTable->GetWeak(aContent);
    if (boxObject) {
      boxObject->Clear();
      mBoxObjectTable->Remove(aContent);
    }
  }
}

nsresult
nsDocument::GetXBLChildNodesFor(nsIContent* aContent, nsIDOMNodeList** aResult)
{
  return BindingManager()->GetXBLChildNodesFor(aContent, aResult);
}

nsresult
nsDocument::GetContentListFor(nsIContent* aContent, nsIDOMNodeList** aResult)
{
  return BindingManager()->GetContentListFor(aContent, aResult);
}

void
nsDocument::FlushSkinBindings()
{
  BindingManager()->FlushSkinBindings();
}

nsresult
nsDocument::InitializeFrameLoader(nsFrameLoader* aLoader)
{
  mInitializableFrameLoaders.RemoveElement(aLoader);
  
  if (mInDestructor) {
    NS_WARNING("Trying to initialize a frame loader while"
               "document is being deleted");
    return NS_ERROR_FAILURE;
  }

  mInitializableFrameLoaders.AppendElement(aLoader);
  if (!mFrameLoaderRunner) {
    mFrameLoaderRunner =
      NS_NEW_RUNNABLE_METHOD(nsDocument, this,
                             MaybeInitializeFinalizeFrameLoaders);
    NS_ENSURE_TRUE(mFrameLoaderRunner, NS_ERROR_OUT_OF_MEMORY);
    nsContentUtils::AddScriptRunner(mFrameLoaderRunner);
  }
  return NS_OK;
}

nsresult
nsDocument::FinalizeFrameLoader(nsFrameLoader* aLoader)
{
  mInitializableFrameLoaders.RemoveElement(aLoader);
  if (mInDestructor) {
    return NS_ERROR_FAILURE;
  }

  mFinalizableFrameLoaders.AppendElement(aLoader);
  if (!mFrameLoaderRunner) {
    mFrameLoaderRunner =
      NS_NEW_RUNNABLE_METHOD(nsDocument, this,
                             MaybeInitializeFinalizeFrameLoaders);
    NS_ENSURE_TRUE(mFrameLoaderRunner, NS_ERROR_OUT_OF_MEMORY);
    nsContentUtils::AddScriptRunner(mFrameLoaderRunner);
  }
  return NS_OK;
}

void
nsDocument::MaybeInitializeFinalizeFrameLoaders()
{
  if (mDelayFrameLoaderInitialization || mUpdateNestLevel != 0) {
    
    
    mFrameLoaderRunner = nsnull;
    return;
  }

  
  
  if (!nsContentUtils::IsSafeToRunScript()) {
    if (!mInDestructor && !mFrameLoaderRunner &&
        (mInitializableFrameLoaders.Length() ||
         mFinalizableFrameLoaders.Length())) {
      mFrameLoaderRunner =
        NS_NEW_RUNNABLE_METHOD(nsDocument, this,
                               MaybeInitializeFinalizeFrameLoaders);
      nsContentUtils::AddScriptRunner(mFrameLoaderRunner);
    }
    return;
  }
  mFrameLoaderRunner = nsnull;

  
  
  
  while (mInitializableFrameLoaders.Length()) {
    nsRefPtr<nsFrameLoader> loader = mInitializableFrameLoaders[0];
    mInitializableFrameLoaders.RemoveElementAt(0);
    NS_ASSERTION(loader, "null frameloader in the array?");
    loader->ReallyStartLoading();
  }

  PRUint32 length = mFinalizableFrameLoaders.Length();
  if (length > 0) {
    nsTArray<nsRefPtr<nsFrameLoader> > loaders;
    mFinalizableFrameLoaders.SwapElements(loaders);
    for (PRUint32 i = 0; i < length; ++i) {
      loaders[i]->Finalize();
    }
  }
}

void
nsDocument::TryCancelFrameLoaderInitialization(nsIDocShell* aShell)
{
  PRUint32 length = mInitializableFrameLoaders.Length();
  for (PRUint32 i = 0; i < length; ++i) {
    if (mInitializableFrameLoaders[i]->GetExistingDocShell() == aShell) {
      mInitializableFrameLoaders.RemoveElementAt(i);
      return;
    }
  }
}

PRBool
nsDocument::FrameLoaderScheduledToBeFinalized(nsIDocShell* aShell)
{
  if (aShell) {
    PRUint32 length = mFinalizableFrameLoaders.Length();
    for (PRUint32 i = 0; i < length; ++i) {
      if (mFinalizableFrameLoaders[i]->GetExistingDocShell() == aShell) {
        return PR_TRUE;
      }
    }
  }
  return PR_FALSE;
}

nsIDocument*
nsDocument::RequestExternalResource(nsIURI* aURI,
                                    nsINode* aRequestingNode,
                                    ExternalResourceLoad** aPendingLoad)
{
  NS_PRECONDITION(aURI, "Must have a URI");
  NS_PRECONDITION(aRequestingNode, "Must have a node");
  if (mDisplayDocument) {
    return mDisplayDocument->RequestExternalResource(aURI,
                                                     aRequestingNode,
                                                     aPendingLoad);
  }

  return mExternalResourceMap.RequestResource(aURI, aRequestingNode,
                                              this, aPendingLoad);
}

void
nsDocument::EnumerateExternalResources(nsSubDocEnumFunc aCallback, void* aData)
{
  mExternalResourceMap.EnumerateResources(aCallback, aData);
}

#ifdef MOZ_SMIL
nsSMILAnimationController*
nsDocument::GetAnimationController()
{
  
  
  if (mAnimationController)
    return mAnimationController;
  
  if (!NS_SMILEnabled())
    return nsnull;

  mAnimationController = NS_NewSMILAnimationController(this);
  
  
  
  nsIPresShell *shell = GetPrimaryShell();
  if (mAnimationController && shell) {
    nsPresContext *context = shell->GetPresContext();
    if (context &&
        context->ImageAnimationMode() == imgIContainer::kDontAnimMode) {
      mAnimationController->Pause(nsSMILTimeContainer::PAUSE_USERPREF);
    }
  }

  return mAnimationController;
}
#endif 

struct DirTable {
  const char* mName;
  PRUint8     mValue;
};

static const DirTable dirAttributes[] = {
  {"ltr", IBMBIDI_TEXTDIRECTION_LTR},
  {"rtl", IBMBIDI_TEXTDIRECTION_RTL},
  {0}
};






NS_IMETHODIMP
nsDocument::GetDir(nsAString& aDirection)
{
  PRUint32 options = GetBidiOptions();
  for (const DirTable* elt = dirAttributes; elt->mName; elt++) {
    if (GET_BIDI_OPTION_DIRECTION(options) == elt->mValue) {
      CopyASCIItoUTF16(elt->mName, aDirection);
      break;
    }
  }

  return NS_OK;
}






NS_IMETHODIMP
nsDocument::SetDir(const nsAString& aDirection)
{
  PRUint32 options = GetBidiOptions();

  for (const DirTable* elt = dirAttributes; elt->mName; elt++) {
    if (aDirection == NS_ConvertASCIItoUTF16(elt->mName)) {
      if (GET_BIDI_OPTION_DIRECTION(options) != elt->mValue) {
        SET_BIDI_OPTION_DIRECTION(options, elt->mValue);
        nsIPresShell *shell = GetPrimaryShell();
        if (shell) {
          nsPresContext *context = shell->GetPresContext();
          NS_ENSURE_TRUE(context, NS_ERROR_UNEXPECTED);
          context->SetBidi(options, PR_TRUE);
        } else {
          
          SetBidiOptions(options);
        }
      }

      break;
    }
  }

  return NS_OK;
}





NS_IMETHODIMP
nsDocument::GetNodeName(nsAString& aNodeName)
{
  aNodeName.AssignLiteral("#document");

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetNodeValue(nsAString& aNodeValue)
{
  SetDOMStringToNull(aNodeValue);

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::SetNodeValue(const nsAString& aNodeValue)
{
  
  
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetNodeType(PRUint16* aNodeType)
{
  *aNodeType = nsIDOMNode::DOCUMENT_NODE;

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetParentNode(nsIDOMNode** aParentNode)
{
  *aParentNode = nsnull;

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetChildNodes(nsIDOMNodeList** aChildNodes)
{
  return nsINode::GetChildNodes(aChildNodes);
}

NS_IMETHODIMP
nsDocument::HasChildNodes(PRBool* aHasChildNodes)
{
  NS_ENSURE_ARG(aHasChildNodes);

  *aHasChildNodes = (mChildren.ChildCount() != 0);

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::HasAttributes(PRBool* aHasAttributes)
{
  NS_ENSURE_ARG(aHasAttributes);

  *aHasAttributes = PR_FALSE;

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetFirstChild(nsIDOMNode** aFirstChild)
{
  return nsINode::GetFirstChild(aFirstChild);
}

NS_IMETHODIMP
nsDocument::GetLastChild(nsIDOMNode** aLastChild)
{
  return nsINode::GetLastChild(aLastChild);
}

NS_IMETHODIMP
nsDocument::GetPreviousSibling(nsIDOMNode** aPreviousSibling)
{
  *aPreviousSibling = nsnull;

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetNextSibling(nsIDOMNode** aNextSibling)
{
  *aNextSibling = nsnull;

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetAttributes(nsIDOMNamedNodeMap** aAttributes)
{
  *aAttributes = nsnull;

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetNamespaceURI(nsAString& aNamespaceURI)
{
  SetDOMStringToNull(aNamespaceURI);

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetPrefix(nsAString& aPrefix)
{
  SetDOMStringToNull(aPrefix);

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::SetPrefix(const nsAString& aPrefix)
{
  return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR;
}

NS_IMETHODIMP
nsDocument::GetLocalName(nsAString& aLocalName)
{
  SetDOMStringToNull(aLocalName);

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::InsertBefore(nsIDOMNode* aNewChild, nsIDOMNode* aRefChild,
                         nsIDOMNode** aReturn)
{
  return nsGenericElement::doReplaceOrInsertBefore(PR_FALSE, aNewChild, aRefChild, nsnull, this,
                                          aReturn);
}

NS_IMETHODIMP
nsDocument::ReplaceChild(nsIDOMNode* aNewChild, nsIDOMNode* aOldChild,
                         nsIDOMNode** aReturn)
{
  return nsGenericElement::doReplaceOrInsertBefore(PR_TRUE, aNewChild, aOldChild, nsnull, this,
                                          aReturn);
}

NS_IMETHODIMP
nsDocument::RemoveChild(nsIDOMNode* aOldChild, nsIDOMNode** aReturn)
{
  return nsGenericElement::doRemoveChild(aOldChild, nsnull, this, aReturn);
}

NS_IMETHODIMP
nsDocument::AppendChild(nsIDOMNode* aNewChild, nsIDOMNode** aReturn)
{
  return nsDocument::InsertBefore(aNewChild, nsnull, aReturn);
}

NS_IMETHODIMP
nsDocument::CloneNode(PRBool aDeep, nsIDOMNode** aReturn)
{
  return nsNodeUtils::CloneNodeImpl(this, aDeep, aReturn);
}

NS_IMETHODIMP
nsDocument::Normalize()
{
  PRInt32 count = mChildren.ChildCount();
  for (PRInt32 i = 0; i < count; ++i) {
    nsCOMPtr<nsIDOMNode> node(do_QueryInterface(mChildren.ChildAt(i)));

    if (node) {
      node->Normalize();
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::IsSupported(const nsAString& aFeature, const nsAString& aVersion,
                        PRBool* aReturn)
{
  return nsGenericElement::InternalIsSupported(static_cast<nsIDOMDocument*>(this),
                                               aFeature, aVersion, aReturn);
}

NS_IMETHODIMP
nsDocument::GetBaseURI(nsAString &aURI)
{
  nsCAutoString spec;
  if (mDocumentBaseURI) {
    mDocumentBaseURI->GetSpec(spec);
  }

  CopyUTF8toUTF16(spec, aURI);

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetTextContent(nsAString &aTextContent)
{
  SetDOMStringToNull(aTextContent);

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::SetTextContent(const nsAString& aTextContent)
{
  return NS_OK;
}


NS_IMETHODIMP
nsDocument::CompareDocumentPosition(nsIDOMNode* aOther, PRUint16* aReturn)
{
  NS_ENSURE_ARG_POINTER(aOther);

  
  
  
  

  nsCOMPtr<nsINode> other = do_QueryInterface(aOther);
  NS_ENSURE_TRUE(other, NS_ERROR_DOM_NOT_SUPPORTED_ERR);

  *aReturn = nsContentUtils::ComparePosition(other, this);
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::IsSameNode(nsIDOMNode* aOther, PRBool* aReturn)
{
  PRBool sameNode = PR_FALSE;

  if (this == aOther) {
    sameNode = PR_TRUE;
  }

  *aReturn = sameNode;

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::IsEqualNode(nsIDOMNode* aOther, PRBool* aReturn)
{
  NS_ENSURE_ARG_POINTER(aOther);

  *aReturn = PR_FALSE;

  
  nsCOMPtr<nsIDocument> aOtherDoc = do_QueryInterface(aOther);
  if (!aOtherDoc) {
    return NS_OK;
  }

  
  PRUint32 childCount = GetChildCount();
  if (childCount != aOtherDoc->GetChildCount()) {
    return NS_OK;
  }

  for (PRUint32 i = 0; i < childCount; i++) {
    nsIContent* aChild1 = GetChildAt(i);
    nsIContent* aChild2 = aOtherDoc->GetChildAt(i);
    if (!nsNode3Tearoff::AreNodesEqual(aChild1, aChild2)) {
      return NS_OK;
    }
  }

  



  *aReturn = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::IsDefaultNamespace(const nsAString& aNamespaceURI,
                               PRBool* aReturn)
{
  nsAutoString defaultNamespace;
  LookupNamespaceURI(EmptyString(), defaultNamespace);
  *aReturn = aNamespaceURI.Equals(defaultNamespace);
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetFeature(const nsAString& aFeature,
                       const nsAString& aVersion,
                       nsISupports** aReturn)
{
  return nsGenericElement::InternalGetFeature(static_cast<nsIDOMDocument*>(this),
                                              aFeature, aVersion, aReturn);
}

NS_IMETHODIMP
nsDocument::SetUserData(const nsAString &aKey,
                        nsIVariant *aData,
                        nsIDOMUserDataHandler *aHandler,
                        nsIVariant **aResult)
{
  return nsNodeUtils::SetUserData(this, aKey, aData, aHandler, aResult);
}

NS_IMETHODIMP
nsDocument::GetUserData(const nsAString &aKey,
                        nsIVariant **aResult)
{
  return nsNodeUtils::GetUserData(this, aKey, aResult);
}

NS_IMETHODIMP
nsDocument::LookupPrefix(const nsAString& aNamespaceURI,
                         nsAString& aPrefix)
{
  nsCOMPtr<nsIDOM3Node> root(do_QueryInterface(GetRootContent()));
  if (root) {
    return root->LookupPrefix(aNamespaceURI, aPrefix);
  }

  SetDOMStringToNull(aPrefix);
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::LookupNamespaceURI(const nsAString& aNamespacePrefix,
                               nsAString& aNamespaceURI)
{
  if (NS_FAILED(nsContentUtils::LookupNamespaceURI(GetRootContent(),
                                                   aNamespacePrefix,
                                                   aNamespaceURI))) {
    SetDOMStringToNull(aNamespaceURI);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetInputEncoding(nsAString& aInputEncoding)
{
  if (mHaveInputEncoding) {
    return GetCharacterSet(aInputEncoding);
  }

  SetDOMStringToNull(aInputEncoding);
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetXmlEncoding(nsAString& aXmlEncoding)
{
  if (mXMLDeclarationBits & XML_DECLARATION_BITS_DECLARATION_EXISTS &&
      mXMLDeclarationBits & XML_DECLARATION_BITS_ENCODING_EXISTS) {
    
    
    GetInputEncoding(aXmlEncoding);
  } else {
    SetDOMStringToNull(aXmlEncoding);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetXmlStandalone(PRBool *aXmlStandalone)
{
  *aXmlStandalone = 
    mXMLDeclarationBits & XML_DECLARATION_BITS_DECLARATION_EXISTS &&
    mXMLDeclarationBits & XML_DECLARATION_BITS_STANDALONE_EXISTS &&
    mXMLDeclarationBits & XML_DECLARATION_BITS_STANDALONE_YES;

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::SetXmlStandalone(PRBool aXmlStandalone)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDocument::GetXmlVersion(nsAString& aXmlVersion)
{
  

  
  aXmlVersion.AssignLiteral("1.0");

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::SetXmlVersion(const nsAString& aXmlVersion)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDocument::GetStrictErrorChecking(PRBool *aStrictErrorChecking)
{
  
  *aStrictErrorChecking = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::SetStrictErrorChecking(PRBool aStrictErrorChecking)
{
  
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetDocumentURI(nsAString& aDocumentURI)
{
  if (mDocumentURI) {
    nsCAutoString uri;
    mDocumentURI->GetSpec(uri);
    CopyUTF8toUTF16(uri, aDocumentURI);
  } else {
    SetDOMStringToNull(aDocumentURI);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::SetDocumentURI(const nsAString& aDocumentURI)
{
  
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

static void BlastSubtreeToPieces(nsINode *aNode);

PLDHashOperator
BlastFunc(nsAttrHashKey::KeyType aKey, nsIDOMNode *aData, void* aUserArg)
{
  nsCOMPtr<nsIAttribute> *attr =
    static_cast<nsCOMPtr<nsIAttribute>*>(aUserArg);

  *attr = do_QueryInterface(aData);

  NS_ASSERTION(attr->get(),
               "non-nsIAttribute somehow made it into the hashmap?!");

  return PL_DHASH_STOP;
}

static void
BlastSubtreeToPieces(nsINode *aNode)
{
  PRUint32 i, count;
  if (aNode->IsNodeOfType(nsINode::eELEMENT)) {
    nsGenericElement *element = static_cast<nsGenericElement*>(aNode);
    const nsDOMAttributeMap *map = element->GetAttributeMap();
    if (map) {
      nsCOMPtr<nsIAttribute> attr;
      while (map->Enumerate(BlastFunc, &attr) > 0) {
        BlastSubtreeToPieces(attr);

#ifdef DEBUG
        nsresult rv =
#endif
          element->UnsetAttr(attr->NodeInfo()->NamespaceID(),
                             attr->NodeInfo()->NameAtom(),
                             PR_FALSE);

        
        NS_ASSERTION(NS_SUCCEEDED(rv), "Uhoh, UnsetAttr shouldn't fail!");
      }
    }
  }

  count = aNode->GetChildCount();
  for (i = 0; i < count; ++i) {
    BlastSubtreeToPieces(aNode->GetChildAt(0));
#ifdef DEBUG
    nsresult rv =
#endif
      aNode->RemoveChildAt(0, PR_FALSE);

    
    NS_ASSERTION(NS_SUCCEEDED(rv), "Uhoh, RemoveChildAt shouldn't fail!");
  }
}

NS_IMETHODIMP
nsDocument::AdoptNode(nsIDOMNode *aAdoptedNode, nsIDOMNode **aResult)
{
  NS_ENSURE_ARG(aAdoptedNode);

  *aResult = nsnull;

  nsresult rv = nsContentUtils::CheckSameOrigin(this, aAdoptedNode);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsINode> adoptedNode;
  PRUint16 nodeType;
  aAdoptedNode->GetNodeType(&nodeType);
  switch (nodeType) {
    case nsIDOMNode::ATTRIBUTE_NODE:
    {
      
      nsCOMPtr<nsIDOMAttr> adoptedAttr = do_QueryInterface(aAdoptedNode, &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<nsIDOMElement> ownerElement;
      rv = adoptedAttr->GetOwnerElement(getter_AddRefs(ownerElement));
      NS_ENSURE_SUCCESS(rv, rv);

      if (ownerElement) {
        nsCOMPtr<nsIDOMAttr> newAttr;
        rv = ownerElement->RemoveAttributeNode(adoptedAttr,
                                               getter_AddRefs(newAttr));
        NS_ENSURE_SUCCESS(rv, rv);

        newAttr.swap(adoptedAttr);
      }

      adoptedNode = do_QueryInterface(adoptedAttr, &rv);
      NS_ENSURE_SUCCESS(rv, rv);
      break;
    }
    case nsIDOMNode::DOCUMENT_FRAGMENT_NODE:
    case nsIDOMNode::ELEMENT_NODE:
    case nsIDOMNode::PROCESSING_INSTRUCTION_NODE:
    case nsIDOMNode::TEXT_NODE:
    case nsIDOMNode::CDATA_SECTION_NODE:
    case nsIDOMNode::COMMENT_NODE:
    {
      adoptedNode = do_QueryInterface(aAdoptedNode, &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      
      
      
      
      nsIDocument *doc = this;
      do {
        nsPIDOMWindow *win = doc->GetWindow();
        if (win) {
          nsCOMPtr<nsINode> node =
            do_QueryInterface(win->GetFrameElementInternal());
          if (node &&
              nsContentUtils::ContentIsDescendantOf(node, adoptedNode)) {
            return NS_ERROR_DOM_HIERARCHY_REQUEST_ERR;
          }
        }
      } while ((doc = doc->GetParentDocument()));

      
      nsCOMPtr<nsIDOMNode> parent;
      aAdoptedNode->GetParentNode(getter_AddRefs(parent));
      NS_ENSURE_SUCCESS(rv, rv);

      if (parent) {
        nsCOMPtr<nsIDOMNode> newChild;
        rv = parent->RemoveChild(aAdoptedNode, getter_AddRefs(newChild));
        NS_ENSURE_SUCCESS(rv, rv);
      }

      break;
    }
    case nsIDOMNode::ENTITY_REFERENCE_NODE:
    {
      return NS_ERROR_NOT_IMPLEMENTED;
    }
    case nsIDOMNode::DOCUMENT_NODE:
    case nsIDOMNode::DOCUMENT_TYPE_NODE:
    case nsIDOMNode::ENTITY_NODE:
    case nsIDOMNode::NOTATION_NODE:
    {
      return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
    }
    default:
    {
      NS_WARNING("Don't know how to adopt this nodetype for adoptNode.");

      return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
    }
  }

  nsIDocument *oldDocument = adoptedNode->GetOwnerDoc();
  PRBool sameDocument = oldDocument == this;

  JSContext *cx = nsnull;
  JSObject *oldScope = nsnull;
  JSObject *newScope = nsnull;
  if (!sameDocument && oldDocument) {
    rv = nsContentUtils::GetContextAndScopes(oldDocument, this, &cx, &oldScope,
                                             &newScope);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMArray<nsINode> nodesWithProperties;
  rv = nsNodeUtils::Adopt(adoptedNode, sameDocument ? nsnull : mNodeInfoManager,
                          cx, oldScope, newScope, nodesWithProperties);
  if (NS_FAILED(rv)) {
    
    
    BlastSubtreeToPieces(adoptedNode);

    if (!sameDocument && oldDocument) {
      PRUint32 i, count = nodesWithProperties.Count();
      for (i = 0; i < count; ++i) {
        
        oldDocument->PropertyTable()->
          DeleteAllPropertiesFor(nodesWithProperties[i]);
      }
    }

    return rv;
  }

  PRUint32 i, count = nodesWithProperties.Count();
  if (!sameDocument && oldDocument) {
    nsPropertyTable *oldTable = oldDocument->PropertyTable();
    nsPropertyTable *newTable = PropertyTable();
    for (i = 0; i < count; ++i) {
      rv = oldTable->TransferOrDeleteAllPropertiesFor(nodesWithProperties[i],
                                                      newTable);
      if (NS_FAILED(rv)) {
        while (++i < count) {
          oldTable->DeleteAllPropertiesFor(nodesWithProperties[i]);
        }

        
        BlastSubtreeToPieces(adoptedNode);

        return rv;
      }
    }
  }

  rv = nsNodeUtils::CallUserDataHandlers(nodesWithProperties, this,
                                         nsIDOMUserDataHandler::NODE_ADOPTED,
                                         PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  return CallQueryInterface(adoptedNode, aResult);
}

NS_IMETHODIMP
nsDocument::GetDomConfig(nsIDOMDOMConfiguration **aConfig)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDocument::NormalizeDocument()
{
  
  
  return Normalize();
}

NS_IMETHODIMP
nsDocument::RenameNode(nsIDOMNode *aNode,
                       const nsAString& namespaceURI,
                       const nsAString& qualifiedName,
                       nsIDOMNode **aReturn)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsDocument::GetOwnerDocument(nsIDOMDocument** aOwnerDocument)
{
  return nsINode::GetOwnerDocument(aOwnerDocument);
}

nsIEventListenerManager*
nsDocument::GetListenerManager(PRBool aCreateIfNotFound)
{
  if (mListenerManager || !aCreateIfNotFound) {
    return mListenerManager;
  }

  nsresult rv = NS_NewEventListenerManager(getter_AddRefs(mListenerManager));
  NS_ENSURE_SUCCESS(rv, nsnull);

  mListenerManager->SetListenerTarget(static_cast<nsIDocument *>(this));

  return mListenerManager;
}

nsresult
nsDocument::GetSystemEventGroup(nsIDOMEventGroup **aGroup)
{
  nsIEventListenerManager* manager = GetListenerManager(PR_TRUE);
  NS_ENSURE_STATE(manager);
  return manager->GetSystemEventGroupLM(aGroup);
}

nsresult
nsDocument::PreHandleEvent(nsEventChainPreVisitor& aVisitor)
{
  aVisitor.mCanHandle = PR_TRUE;
   
   
  aVisitor.mForceContentDispatch = PR_TRUE;

  
  if (aVisitor.mEvent->message != NS_LOAD) {
    nsCOMPtr<nsPIDOMEventTarget> parentTarget = do_QueryInterface(GetWindow());
    aVisitor.mParentTarget = parentTarget;
  }
  return NS_OK;
}

nsresult
nsDocument::PostHandleEvent(nsEventChainPostVisitor& aVisitor)
{
  return NS_OK;
}

nsresult
nsDocument::DispatchDOMEvent(nsEvent* aEvent,
                             nsIDOMEvent* aDOMEvent,
                             nsPresContext* aPresContext,
                             nsEventStatus* aEventStatus)
{
  return nsEventDispatcher::DispatchDOMEvent(static_cast<nsINode*>(this),
                                             aEvent, aDOMEvent,
                                             aPresContext, aEventStatus);
}

nsresult
nsDocument::AddEventListenerByIID(nsIDOMEventListener *aListener,
                                  const nsIID& aIID)
{
  nsIEventListenerManager* manager = GetListenerManager(PR_TRUE);
  NS_ENSURE_STATE(manager);
  return manager->AddEventListenerByIID(aListener, aIID, NS_EVENT_FLAG_BUBBLE);
}

nsresult
nsDocument::RemoveEventListenerByIID(nsIDOMEventListener *aListener,
                                     const nsIID& aIID)
{
  return mListenerManager ?
    mListenerManager->RemoveEventListenerByIID(aListener, aIID,
                                               NS_EVENT_FLAG_BUBBLE) :
    NS_OK;
}

nsresult
nsDocument::AddEventListener(const nsAString& aType,
                             nsIDOMEventListener* aListener,
                             PRBool aUseCapture)
{
  return AddEventListener(aType, aListener, aUseCapture,
                          !nsContentUtils::IsChromeDoc(this));
}

nsresult
nsDocument::RemoveEventListener(const nsAString& aType,
                                nsIDOMEventListener* aListener,
                                PRBool aUseCapture)
{
  return RemoveGroupedEventListener(aType, aListener, aUseCapture, nsnull);
}

NS_IMETHODIMP
nsDocument::DispatchEvent(nsIDOMEvent* aEvent, PRBool *_retval)
{
  
  nsIPresShell *shell = GetPrimaryShell();
  nsCOMPtr<nsPresContext> context;
  if (shell) {
     context = shell->GetPresContext();
  }

  nsEventStatus status = nsEventStatus_eIgnore;
  nsresult rv =
    nsEventDispatcher::DispatchDOMEvent(static_cast<nsINode*>(this),
                                        nsnull, aEvent, context, &status);

  *_retval = (status != nsEventStatus_eConsumeNoDefault);
  return rv;
}

NS_IMETHODIMP
nsDocument::AddGroupedEventListener(const nsAString& aType,
                                    nsIDOMEventListener *aListener,
                                    PRBool aUseCapture,
                                    nsIDOMEventGroup *aEvtGrp)
{
  nsIEventListenerManager* manager = GetListenerManager(PR_TRUE);
  NS_ENSURE_STATE(manager);
  PRInt32 flags = aUseCapture ? NS_EVENT_FLAG_CAPTURE : NS_EVENT_FLAG_BUBBLE;
  return manager->AddEventListenerByType(aListener, aType, flags, aEvtGrp);
}

NS_IMETHODIMP
nsDocument::RemoveGroupedEventListener(const nsAString& aType,
                                       nsIDOMEventListener *aListener,
                                       PRBool aUseCapture,
                                       nsIDOMEventGroup *aEvtGrp)
{
  if (mListenerManager) {
    PRInt32 flags = aUseCapture ? NS_EVENT_FLAG_CAPTURE : NS_EVENT_FLAG_BUBBLE;
    mListenerManager->RemoveEventListenerByType(aListener, aType, flags,
                                                aEvtGrp);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::CanTrigger(const nsAString & type, PRBool *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDocument::IsRegisteredHere(const nsAString & type, PRBool *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDocument::AddEventListener(const nsAString& aType,
                             nsIDOMEventListener *aListener,
                             PRBool aUseCapture, PRBool aWantsUntrusted)
{
  nsIEventListenerManager* manager = GetListenerManager(PR_TRUE);
  NS_ENSURE_STATE(manager);

  PRInt32 flags = aUseCapture ? NS_EVENT_FLAG_CAPTURE : NS_EVENT_FLAG_BUBBLE;

  if (aWantsUntrusted) {
    flags |= NS_PRIV_EVENT_UNTRUSTED_PERMITTED;
  }

  return manager->AddEventListenerByType(aListener, aType, flags, nsnull);
}

NS_IMETHODIMP
nsDocument::CreateEvent(const nsAString& aEventType, nsIDOMEvent** aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);
  *aReturn = nsnull;

  

  nsIPresShell *shell = GetPrimaryShell();

  nsPresContext *presContext = nsnull;

  if (shell) {
    
    presContext = shell->GetPresContext();
  }

  
  return nsEventDispatcher::CreateEvent(presContext, nsnull,
                                        aEventType, aReturn);
}

NS_IMETHODIMP
nsDocument::CreateEventGroup(nsIDOMEventGroup **aInstancePtrResult)
{
  nsresult rv;
  nsCOMPtr<nsIDOMEventGroup> group(do_CreateInstance(kDOMEventGroupCID, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  *aInstancePtrResult = group;
  NS_ADDREF(*aInstancePtrResult);

  return NS_OK;
}

void
nsDocument::FlushPendingNotifications(mozFlushType aType)
{
  nsCOMPtr<nsIContentSink> sink;
  if (mParser) {
    sink = mParser->GetContentSink();
  } else {
    sink = do_QueryReferent(mWeakSink);
  }
  
  
  if (sink && (aType == Flush_Content || IsSafeToFlush())) {
    sink->FlushPendingNotifications(aType);
  }

  

  if (aType <= Flush_ContentAndNotify) {
    
    return;
  }

  
  
  
  
  
  
  
  
  
  if (mParentDocument && IsSafeToFlush()) {
    mozFlushType parentType = aType;
    if (aType >= Flush_Style)
      parentType = PR_MAX(Flush_Layout, aType);
    mParentDocument->FlushPendingNotifications(parentType);
  }

  nsPresShellIterator iter(this);
  nsCOMPtr<nsIPresShell> shell;
  while ((shell = iter.GetNextShell())) {
    shell->FlushPendingNotifications(aType);
  }
}

nsIScriptEventManager*
nsDocument::GetScriptEventManager()
{
  if (!mScriptEventManager) {
    mScriptEventManager = new nsScriptEventManager(this);
    
  }

  return mScriptEventManager;
}

void
nsDocument::SetXMLDeclaration(const PRUnichar *aVersion,
                              const PRUnichar *aEncoding,
                              const PRInt32 aStandalone)
{
  if (!aVersion || *aVersion == '\0') {
    mXMLDeclarationBits = 0;
    return;
  }

  mXMLDeclarationBits = XML_DECLARATION_BITS_DECLARATION_EXISTS;

  if (aEncoding && *aEncoding != '\0') {
    mXMLDeclarationBits |= XML_DECLARATION_BITS_ENCODING_EXISTS;
  }

  if (aStandalone == 1) {
    mXMLDeclarationBits |= XML_DECLARATION_BITS_STANDALONE_EXISTS |
                           XML_DECLARATION_BITS_STANDALONE_YES;
  }
  else if (aStandalone == 0) {
    mXMLDeclarationBits |= XML_DECLARATION_BITS_STANDALONE_EXISTS;
  }
}

void
nsDocument::GetXMLDeclaration(nsAString& aVersion, nsAString& aEncoding,
                              nsAString& aStandalone)
{
  aVersion.Truncate();
  aEncoding.Truncate();
  aStandalone.Truncate();

  if (!(mXMLDeclarationBits & XML_DECLARATION_BITS_DECLARATION_EXISTS)) {
    return;
  }

  
  aVersion.AssignLiteral("1.0");

  if (mXMLDeclarationBits & XML_DECLARATION_BITS_ENCODING_EXISTS) {
    
    
    GetCharacterSet(aEncoding);
  }

  if (mXMLDeclarationBits & XML_DECLARATION_BITS_STANDALONE_EXISTS) {
    if (mXMLDeclarationBits & XML_DECLARATION_BITS_STANDALONE_YES) {
      aStandalone.AssignLiteral("yes");
    } else {
      aStandalone.AssignLiteral("no");
    }
  }
}

PRBool
nsDocument::IsScriptEnabled()
{
  nsCOMPtr<nsIScriptSecurityManager> sm(do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID));
  NS_ENSURE_TRUE(sm, PR_FALSE);

  nsIScriptGlobalObject* globalObject = GetScriptGlobalObject();
  NS_ENSURE_TRUE(globalObject, PR_FALSE);

  nsIScriptContext *scriptContext = globalObject->GetContext();
  NS_ENSURE_TRUE(scriptContext, PR_FALSE);

  JSContext* cx = (JSContext *) scriptContext->GetNativeContext();
  NS_ENSURE_TRUE(cx, PR_FALSE);

  PRBool enabled;
  nsresult rv = sm->CanExecuteScripts(cx, NodePrincipal(), &enabled);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);
  return enabled;
}

nsresult
nsDocument::GetRadioGroup(const nsAString& aName,
                          nsRadioGroupStruct **aRadioGroup)
{
  nsAutoString tmKey(aName);
  if(!IsCaseSensitive())
     ToLowerCase(tmKey); 
  if (mRadioGroups.Get(tmKey, aRadioGroup))
    return NS_OK;

  nsAutoPtr<nsRadioGroupStruct> radioGroup(new nsRadioGroupStruct());
  NS_ENSURE_TRUE(radioGroup, NS_ERROR_OUT_OF_MEMORY);
  NS_ENSURE_TRUE(mRadioGroups.Put(tmKey, radioGroup), NS_ERROR_OUT_OF_MEMORY);

  *aRadioGroup = radioGroup;
  radioGroup.forget();

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::SetCurrentRadioButton(const nsAString& aName,
                                  nsIDOMHTMLInputElement* aRadio)
{
  nsRadioGroupStruct* radioGroup = nsnull;
  GetRadioGroup(aName, &radioGroup);
  if (radioGroup) {
    radioGroup->mSelectedRadioButton = aRadio;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetCurrentRadioButton(const nsAString& aName,
                                  nsIDOMHTMLInputElement** aRadio)
{
  nsRadioGroupStruct* radioGroup = nsnull;
  GetRadioGroup(aName, &radioGroup);
  if (radioGroup) {
    *aRadio = radioGroup->mSelectedRadioButton;
    NS_IF_ADDREF(*aRadio);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetPositionInGroup(nsIDOMHTMLInputElement *aRadio,
                               PRInt32 *aPositionIndex,
                               PRInt32 *aItemsInGroup)
{
  *aPositionIndex = 0;
  *aItemsInGroup = 1;
  nsAutoString name;
  aRadio->GetName(name);
  if (name.IsEmpty()) {
    return NS_OK;
  }

  nsRadioGroupStruct* radioGroup = nsnull;
  nsresult rv = GetRadioGroup(name, &radioGroup);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIFormControl> radioControl(do_QueryInterface(aRadio));
  NS_ASSERTION(radioControl, "Radio button should implement nsIFormControl");
  *aPositionIndex = radioGroup->mRadioButtons.IndexOf(radioControl);
  NS_ASSERTION(*aPositionIndex >= 0, "Radio button not found in its own group");
  *aItemsInGroup = radioGroup->mRadioButtons.Count();

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetNextRadioButton(const nsAString& aName,
                               const PRBool aPrevious,
                               nsIDOMHTMLInputElement*  aFocusedRadio,
                               nsIDOMHTMLInputElement** aRadioOut)
{
  
  
  
  
  *aRadioOut = nsnull;

  nsRadioGroupStruct* radioGroup = nsnull;
  GetRadioGroup(aName, &radioGroup);
  if (!radioGroup) {
    return NS_ERROR_FAILURE;
  }

  
  
  nsCOMPtr<nsIDOMHTMLInputElement> currentRadio;
  if (aFocusedRadio) {
    currentRadio = aFocusedRadio;
  }
  else {
    currentRadio = radioGroup->mSelectedRadioButton;
    if (!currentRadio) {
      return NS_ERROR_FAILURE;
    }
  }
  nsCOMPtr<nsIFormControl> radioControl(do_QueryInterface(currentRadio));
  PRInt32 index = radioGroup->mRadioButtons.IndexOf(radioControl);
  if (index < 0) {
    return NS_ERROR_FAILURE;
  }

  PRInt32 numRadios = radioGroup->mRadioButtons.Count();
  PRBool disabled;
  nsCOMPtr<nsIDOMHTMLInputElement> radio;
  do {
    if (aPrevious) {
      if (--index < 0) {
        index = numRadios -1;
      }
    }
    else if (++index >= numRadios) {
      index = 0;
    }
    radio = do_QueryInterface(radioGroup->mRadioButtons[index]);
    NS_ASSERTION(radio, "mRadioButtons holding a non-radio button");
    radio->GetDisabled(&disabled);
  } while (disabled && radio != currentRadio);

  NS_IF_ADDREF(*aRadioOut = radio);
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::AddToRadioGroup(const nsAString& aName,
                            nsIFormControl* aRadio)
{
  nsRadioGroupStruct* radioGroup = nsnull;
  GetRadioGroup(aName, &radioGroup);
  if (radioGroup) {
    radioGroup->mRadioButtons.AppendObject(aRadio);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::RemoveFromRadioGroup(const nsAString& aName,
                                 nsIFormControl* aRadio)
{
  nsRadioGroupStruct* radioGroup = nsnull;
  GetRadioGroup(aName, &radioGroup);
  if (radioGroup) {
    radioGroup->mRadioButtons.RemoveObject(aRadio);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::WalkRadioGroup(const nsAString& aName,
                           nsIRadioVisitor* aVisitor,
                           PRBool aFlushContent)
{
  nsRadioGroupStruct* radioGroup = nsnull;
  GetRadioGroup(aName, &radioGroup);
  if (!radioGroup) {
    return NS_OK;
  }

  PRBool stop = PR_FALSE;
  for (int i = 0; i < radioGroup->mRadioButtons.Count(); i++) {
    aVisitor->Visit(radioGroup->mRadioButtons[i], &stop);
    if (stop) {
      return NS_OK;
    }
  }

  return NS_OK;
}

void
nsDocument::RetrieveRelevantHeaders(nsIChannel *aChannel)
{
  nsCOMPtr<nsIHttpChannel> httpChannel = do_QueryInterface(aChannel);
  PRTime modDate = LL_ZERO;
  nsresult rv;

  if (httpChannel) {
    nsCAutoString tmp;
    rv = httpChannel->GetResponseHeader(NS_LITERAL_CSTRING("last-modified"),
                                        tmp);

    if (NS_SUCCEEDED(rv)) {
      PRTime time;
      PRStatus st = PR_ParseTimeString(tmp.get(), PR_TRUE, &time);
      if (st == PR_SUCCESS) {
        modDate = time;
      }
    }

    
    rv = httpChannel->GetRequestHeader(NS_LITERAL_CSTRING("referer"),
                                       mReferrer);
    if (NS_FAILED(rv)) {
      mReferrer.Truncate();
    }

    static const char *const headers[] = {
      "default-style",
      "content-style-type",
      "content-language",
      "content-disposition",
      "refresh",
      "x-dns-prefetch-control",
      
      
      
      0
    };
    
    nsCAutoString headerVal;
    const char *const *name = headers;
    while (*name) {
      rv =
        httpChannel->GetResponseHeader(nsDependentCString(*name), headerVal);
      if (NS_SUCCEEDED(rv) && !headerVal.IsEmpty()) {
        nsCOMPtr<nsIAtom> key = do_GetAtom(*name);
        SetHeaderData(key, NS_ConvertASCIItoUTF16(headerVal));
      }
      ++name;
    }
  } else {
    nsCOMPtr<nsIFileChannel> fileChannel = do_QueryInterface(aChannel);
    if (fileChannel) {
      nsCOMPtr<nsIFile> file;
      fileChannel->GetFile(getter_AddRefs(file));
      if (file) {
        PRTime msecs;
        rv = file->GetLastModifiedTime(&msecs);

        if (NS_SUCCEEDED(rv)) {
          PRInt64 intermediateValue;
          LL_I2L(intermediateValue, PR_USEC_PER_MSEC);
          LL_MUL(modDate, msecs, intermediateValue);
        }
      }
    } else {
      nsCOMPtr<nsIMultiPartChannel> partChannel = do_QueryInterface(aChannel);
      if (partChannel) {
        nsCAutoString contentDisp;
        rv = partChannel->GetContentDisposition(contentDisp);
        if (NS_SUCCEEDED(rv) && !contentDisp.IsEmpty()) {
          SetHeaderData(nsGkAtoms::headerContentDisposition,
                        NS_ConvertASCIItoUTF16(contentDisp));
        }
      }
    }
  }

  if (LL_IS_ZERO(modDate)) {
    
    
    
    modDate = PR_Now();
  }

  mLastModified.Truncate();
  if (LL_NE(modDate, LL_ZERO)) {
    PRExplodedTime prtime;
    PR_ExplodeTime(modDate, PR_LocalTimeParameters, &prtime);
    
    char formatedTime[24];
    if (PR_snprintf(formatedTime, sizeof(formatedTime),
                    "%02ld/%02ld/%04hd %02ld:%02ld:%02ld",
                    prtime.tm_month + 1, prtime.tm_mday, prtime.tm_year,
                    prtime.tm_hour     ,  prtime.tm_min,  prtime.tm_sec)) {
      CopyASCIItoUTF16(nsDependentCString(formatedTime), mLastModified);
    }
  }
}

nsresult
nsDocument::CreateElem(nsIAtom *aName, nsIAtom *aPrefix, PRInt32 aNamespaceID,
                       PRBool aDocumentDefaultType, nsIContent **aResult)
{
#ifdef DEBUG
  nsAutoString qName;
  if (aPrefix) {
    aPrefix->ToString(qName);
    qName.Append(':');
  }
  const char *name;
  aName->GetUTF8String(&name);
  AppendUTF8toUTF16(name, qName);

  
  
  
  PRBool nsAware = aPrefix != nsnull || aNamespaceID != GetDefaultNamespaceID();
  NS_ASSERTION(NS_SUCCEEDED(nsContentUtils::CheckQName(qName, nsAware)),
               "Don't pass invalid prefixes to nsDocument::CreateElem, "
               "check caller.");
#endif

  *aResult = nsnull;
  
  PRInt32 elementType = aDocumentDefaultType ? mDefaultElementType :
    aNamespaceID;

  nsCOMPtr<nsINodeInfo> nodeInfo;
  nodeInfo = mNodeInfoManager->GetNodeInfo(aName, aPrefix, aNamespaceID);
  NS_ENSURE_TRUE(nodeInfo, NS_ERROR_OUT_OF_MEMORY);

  return NS_NewElement(aResult, elementType, nodeInfo, PR_FALSE);
}

PRBool
nsDocument::IsSafeToFlush() const
{
  PRBool isSafeToFlush = PR_TRUE;
  nsPresShellIterator iter(const_cast<nsIDocument*>
                                     (static_cast<const nsIDocument*>(this)));
  nsCOMPtr<nsIPresShell> shell;
  while ((shell = iter.GetNextShell()) && isSafeToFlush) {
    shell->IsSafeToFlush(isSafeToFlush);
  }
  return isSafeToFlush;
}

nsresult
nsDocument::Sanitize()
{
  
  
  
  
  

  
  

  nsCOMPtr<nsIDOMNodeList> nodes;
  nsresult rv = GetElementsByTagName(NS_LITERAL_STRING("input"),
                                     getter_AddRefs(nodes));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 length = 0;
  if (nodes)
    nodes->GetLength(&length);

  nsCOMPtr<nsIDOMNode> item;
  nsAutoString value;
  PRUint32 i;

  for (i = 0; i < length; ++i) {
    nodes->Item(i, getter_AddRefs(item));
    NS_ASSERTION(item, "null item in node list!");

    nsCOMPtr<nsIDOMHTMLInputElement> input = do_QueryInterface(item);
    if (!input)
      continue;

    PRBool resetValue = PR_FALSE;

    input->GetAttribute(NS_LITERAL_STRING("autocomplete"), value);
    if (value.LowerCaseEqualsLiteral("off")) {
      resetValue = PR_TRUE;
    } else {
      input->GetType(value);
      if (value.LowerCaseEqualsLiteral("password"))
        resetValue = PR_TRUE;
    }

    if (resetValue) {
      nsCOMPtr<nsIFormControl> fc = do_QueryInterface(input);
      fc->Reset();
    }
  }

  
  rv = GetElementsByTagName(NS_LITERAL_STRING("form"), getter_AddRefs(nodes));
  NS_ENSURE_SUCCESS(rv, rv);

  length = 0;
  if (nodes)
    nodes->GetLength(&length);

  for (i = 0; i < length; ++i) {
    nodes->Item(i, getter_AddRefs(item));
    NS_ASSERTION(item, "null item in nodelist");

    nsCOMPtr<nsIDOMHTMLFormElement> form = do_QueryInterface(item);
    if (!form)
      continue;

    form->GetAttribute(NS_LITERAL_STRING("autocomplete"), value);
    if (value.LowerCaseEqualsLiteral("off"))
      form->Reset();
  }

  return NS_OK;
}

struct SubDocEnumArgs
{
  nsIDocument::nsSubDocEnumFunc callback;
  void *data;
};

static PLDHashOperator
SubDocHashEnum(PLDHashTable *table, PLDHashEntryHdr *hdr,
               PRUint32 number, void *arg)
{
  SubDocMapEntry *entry = static_cast<SubDocMapEntry*>(hdr);
  SubDocEnumArgs *args = static_cast<SubDocEnumArgs*>(arg);

  nsIDocument *subdoc = entry->mSubDocument;
  PRBool next = subdoc ? args->callback(subdoc, args->data) : PR_TRUE;

  return next ? PL_DHASH_NEXT : PL_DHASH_STOP;
}

void
nsDocument::EnumerateSubDocuments(nsSubDocEnumFunc aCallback, void *aData)
{
  if (mSubDocuments) {
    SubDocEnumArgs args = { aCallback, aData };
    PL_DHashTableEnumerate(mSubDocuments, SubDocHashEnum, &args);
  }
}

static PLDHashOperator
CanCacheSubDocument(PLDHashTable *table, PLDHashEntryHdr *hdr,
                    PRUint32 number, void *arg)
{
  SubDocMapEntry *entry = static_cast<SubDocMapEntry*>(hdr);
  PRBool *canCacheArg = static_cast<PRBool*>(arg);

  nsIDocument *subdoc = entry->mSubDocument;

  
  PRBool canCache = subdoc ? subdoc->CanSavePresentation(nsnull) : PR_FALSE;
  if (!canCache) {
    *canCacheArg = PR_FALSE;
    return PL_DHASH_STOP;
  }

  return PL_DHASH_NEXT;
}

#ifdef DEBUG_bryner
#define DEBUG_PAGE_CACHE
#endif

PRBool
nsDocument::CanSavePresentation(nsIRequest *aNewRequest)
{
  if (EventHandlingSuppressed()) {
    return PR_FALSE;
  }

  nsPIDOMWindow* win = GetInnerWindow();
  if (win && win->TimeoutSuspendCount()) {
    return PR_FALSE;
  }

  
  nsCOMPtr<nsPIDOMEventTarget> piTarget = do_QueryInterface(mScriptGlobalObject);
  if (piTarget) {
    nsIEventListenerManager* manager =
      piTarget->GetListenerManager(PR_FALSE);
    if (manager && manager->HasUnloadListeners()) {
      return PR_FALSE;
    }
  }

  nsCOMPtr<nsILoadGroup> loadGroup = GetDocumentLoadGroup();
  if (loadGroup) {
    nsCOMPtr<nsISimpleEnumerator> requests;
    loadGroup->GetRequests(getter_AddRefs(requests));

    PRBool hasMore = PR_FALSE;

    while (NS_SUCCEEDED(requests->HasMoreElements(&hasMore)) && hasMore) {
      nsCOMPtr<nsISupports> elem;
      requests->GetNext(getter_AddRefs(elem));

      nsCOMPtr<nsIRequest> request = do_QueryInterface(elem);
      if (request && request != aNewRequest) {
#ifdef DEBUG_PAGE_CACHE
        nsCAutoString requestName, docSpec;
        request->GetName(requestName);
        if (mDocumentURI)
          mDocumentURI->GetSpec(docSpec);

        printf("document %s has request %s\n",
               docSpec.get(), requestName.get());
#endif
        return PR_FALSE;
      }
    }
  }

  PRBool canCache = PR_TRUE;
  if (mSubDocuments)
    PL_DHashTableEnumerate(mSubDocuments, CanCacheSubDocument, &canCache);

  return canCache;
}

void
nsDocument::Destroy()
{
  
  
  if (mIsGoingAway)
    return;

  mIsGoingAway = PR_TRUE;

  RemovedFromDocShell();

  PRUint32 i, count = mChildren.ChildCount();
  for (i = 0; i < count; ++i) {
    mChildren.ChildAt(i)->DestroyContent();
  }

  mLayoutHistoryState = nsnull;

  nsContentList::OnDocumentDestroy(this);

  
  
  
  mExternalResourceMap.Shutdown();

  
  
  nsContentUtils::ReleaseWrapper(static_cast<nsINode*>(this), this);
}

void
nsDocument::RemovedFromDocShell()
{
  if (mRemovedFromDocShell)
    return;

  mRemovedFromDocShell = PR_TRUE;

  PRUint32 i, count = mChildren.ChildCount();
  for (i = 0; i < count; ++i) {
    mChildren.ChildAt(i)->SaveSubtreeState();
  }
}

already_AddRefed<nsILayoutHistoryState>
nsDocument::GetLayoutHistoryState() const
{
  nsILayoutHistoryState* state = nsnull;
  if (!mScriptGlobalObject) {
    NS_IF_ADDREF(state = mLayoutHistoryState);
  } else {
    nsCOMPtr<nsIDocShell> docShell(do_QueryReferent(mDocumentContainer));
    if (docShell) {
      docShell->GetLayoutHistoryState(&state);
    }
  }

  return state;
}

void
nsDocument::BlockOnload()
{
  if (mDisplayDocument) {
    mDisplayDocument->BlockOnload();
    return;
  }
  
  
  
  if (mOnloadBlockCount == 0 && mScriptGlobalObject) {
    nsCOMPtr<nsILoadGroup> loadGroup = GetDocumentLoadGroup();
    if (loadGroup) {
      loadGroup->AddRequest(mOnloadBlocker, nsnull);
    }
  }
  ++mOnloadBlockCount;      
}

void
nsDocument::UnblockOnload(PRBool aFireSync)
{
  if (mDisplayDocument) {
    mDisplayDocument->UnblockOnload(aFireSync);
    return;
  }

  if (mOnloadBlockCount == 0) {
    NS_NOTREACHED("More UnblockOnload() calls than BlockOnload() calls; dropping call");
    return;
  }

  --mOnloadBlockCount;

  
  
  if (mOnloadBlockCount == 0 && mScriptGlobalObject) {
    if (aFireSync) {
      
      ++mOnloadBlockCount;
      DoUnblockOnload();
    } else {
      PostUnblockOnloadEvent();
    }
  }
}

class nsUnblockOnloadEvent : public nsRunnable {
public:
  nsUnblockOnloadEvent(nsDocument *doc) : mDoc(doc) {}
  NS_IMETHOD Run() {
    mDoc->DoUnblockOnload();
    return NS_OK;
  }
private:  
  nsRefPtr<nsDocument> mDoc;
};

void
nsDocument::PostUnblockOnloadEvent()
{
  nsCOMPtr<nsIRunnable> evt = new nsUnblockOnloadEvent(this);
  nsresult rv = NS_DispatchToCurrentThread(evt);
  if (NS_SUCCEEDED(rv)) {
    
    ++mOnloadBlockCount;
  } else {
    NS_WARNING("failed to dispatch nsUnblockOnloadEvent");
  }
}

void
nsDocument::DoUnblockOnload()
{
  NS_PRECONDITION(!mDisplayDocument,
                  "Shouldn't get here for resource document");
  NS_PRECONDITION(mOnloadBlockCount != 0,
                  "Shouldn't have a count of zero here, since we stabilized in "
                  "PostUnblockOnloadEvent");
  
  --mOnloadBlockCount;

  if (mOnloadBlockCount != 0) {
    
    
    return;
  }

  
  
  if (mScriptGlobalObject) {
    nsCOMPtr<nsILoadGroup> loadGroup = GetDocumentLoadGroup();
    if (loadGroup) {
      loadGroup->RemoveRequest(mOnloadBlocker, nsnull, NS_OK);
    }
  }
}



already_AddRefed<nsIDOMElement>
nsDocument::CheckAncestryAndGetFrame(nsIDocument* aDocument) const
{
  nsIDocument* parentDoc;
  for (parentDoc = aDocument->GetParentDocument();
       parentDoc != static_cast<const nsIDocument* const>(this);
       parentDoc = parentDoc->GetParentDocument()) {
    if (!parentDoc) {
      return nsnull;
    }

    aDocument = parentDoc;
  }

  
  nsPIDOMWindow* currentWindow = aDocument->GetWindow();
  if (!currentWindow) {
    return nsnull;
  }
  nsIDOMElement* frameElement = currentWindow->GetFrameElementInternal();
  if (!frameElement) {
    return nsnull;
  }

  
  nsCOMPtr<nsIDOMDocument> domDocument;
  frameElement->GetOwnerDocument(getter_AddRefs(domDocument));
  if (domDocument != this) {
    NS_ERROR("Child documents should live in windows the parent owns");
    return nsnull;
  }

  NS_ADDREF(frameElement);
  return frameElement;
}

void
nsDocument::DispatchPageTransition(nsPIDOMEventTarget* aDispatchTarget,
                                   const nsAString& aType,
                                   PRBool aPersisted)
{
  if (aDispatchTarget) {
    nsCOMPtr<nsIDOMEvent> event;
    CreateEvent(NS_LITERAL_STRING("pagetransition"), getter_AddRefs(event));
    nsCOMPtr<nsIDOMPageTransitionEvent> ptEvent = do_QueryInterface(event);
    nsCOMPtr<nsIPrivateDOMEvent> pEvent = do_QueryInterface(ptEvent);
    if (pEvent && NS_SUCCEEDED(ptEvent->InitPageTransitionEvent(aType, PR_TRUE,
                                                                PR_TRUE,
                                                                aPersisted))) {
      pEvent->SetTrusted(PR_TRUE);
      pEvent->SetTarget(this);
      nsEventDispatcher::DispatchDOMEvent(aDispatchTarget, nsnull, event,
                                          nsnull, nsnull);
    }
  }
}

void
nsDocument::OnPageShow(PRBool aPersisted, nsIDOMEventTarget* aDispatchStartTarget)
{
  mVisible = PR_TRUE;
  UpdateLinkMap();
  
  nsIContent* root = GetRootContent();
  if (aPersisted && root) {
    
    nsRefPtr<nsContentList> links = NS_GetContentList(root,
                                                      nsGkAtoms::link,
                                                      kNameSpaceID_Unknown);

    if (links) {
      PRUint32 linkCount = links->Length(PR_TRUE);
      for (PRUint32 i = 0; i < linkCount; ++i) {
        nsCOMPtr<nsILink> link = do_QueryInterface(links->Item(i, PR_FALSE));
        if (link) {
          link->LinkAdded();
        }
      }
    }
  }

  
  if (!aDispatchStartTarget) {
    
    
    mIsShowing = PR_TRUE;
  }
 
#ifdef MOZ_SMIL
  if (mAnimationController) {
    mAnimationController->OnPageShow();
  }
#endif
  nsCOMPtr<nsPIDOMEventTarget> target =
    aDispatchStartTarget ? do_QueryInterface(aDispatchStartTarget) :
                           do_QueryInterface(GetWindow());
  DispatchPageTransition(target, NS_LITERAL_STRING("pageshow"), aPersisted);
}

void
nsDocument::OnPageHide(PRBool aPersisted, nsIDOMEventTarget* aDispatchStartTarget)
{
  
  
  nsIContent* root = GetRootContent();
  if (aPersisted && root) {
    nsRefPtr<nsContentList> links = NS_GetContentList(root,
                                                      nsGkAtoms::link,
                                                      kNameSpaceID_Unknown);

    if (links) {
      PRUint32 linkCount = links->Length(PR_TRUE);
      for (PRUint32 i = 0; i < linkCount; ++i) {
        nsCOMPtr<nsILink> link = do_QueryInterface(links->Item(i, PR_FALSE));
        if (link) {
          link->LinkRemoved();
        }
      }
    }
  }

  
  if (!aDispatchStartTarget) {
    
    
    mIsShowing = PR_FALSE;
  }

#ifdef MOZ_SMIL
  if (mAnimationController) {
    mAnimationController->OnPageHide();
  }
#endif
  
  
  nsCOMPtr<nsPIDOMEventTarget> target =
    aDispatchStartTarget ? do_QueryInterface(aDispatchStartTarget) :
                           do_QueryInterface(GetWindow());
  DispatchPageTransition(target, NS_LITERAL_STRING("pagehide"), aPersisted);

  mVisible = PR_FALSE;
}

void
nsDocument::WillDispatchMutationEvent(nsINode* aTarget)
{
  NS_ASSERTION(mSubtreeModifiedDepth != 0 ||
               mSubtreeModifiedTargets.Count() == 0,
               "mSubtreeModifiedTargets not cleared after dispatching?");
  ++mSubtreeModifiedDepth;
  if (aTarget) {
    
    
    PRInt32 count = mSubtreeModifiedTargets.Count();
    if (!count || mSubtreeModifiedTargets[count - 1] != aTarget) {
      mSubtreeModifiedTargets.AppendObject(aTarget);
    }
  }
}

void
nsDocument::MutationEventDispatched(nsINode* aTarget)
{
  --mSubtreeModifiedDepth;
  if (mSubtreeModifiedDepth == 0) {
    PRInt32 count = mSubtreeModifiedTargets.Count();
    if (!count) {
      return;
    }

    nsCOMPtr<nsPIDOMWindow> window;
    window = do_QueryInterface(GetScriptGlobalObject());
    if (window &&
        !window->HasMutationListeners(NS_EVENT_BITS_MUTATION_SUBTREEMODIFIED)) {
      mSubtreeModifiedTargets.Clear();
      return;
    }

    nsCOMArray<nsINode> realTargets;
    for (PRInt32 i = 0; i < count; ++i) {
      nsINode* possibleTarget = mSubtreeModifiedTargets[i];
      nsCOMPtr<nsIContent> content = do_QueryInterface(possibleTarget);
      if (content && content->IsInNativeAnonymousSubtree()) {
        continue;
      }

      nsINode* commonAncestor = nsnull;
      PRInt32 realTargetCount = realTargets.Count();
      for (PRInt32 j = 0; j < realTargetCount; ++j) {
        commonAncestor =
          nsContentUtils::GetCommonAncestor(possibleTarget, realTargets[j]);
        if (commonAncestor) {
          realTargets.ReplaceObjectAt(commonAncestor, j);
          break;
        }
      }
      if (!commonAncestor) {
        realTargets.AppendObject(possibleTarget);
      }
    }

    mSubtreeModifiedTargets.Clear();

    PRInt32 realTargetCount = realTargets.Count();
    for (PRInt32 k = 0; k < realTargetCount; ++k) {
      mozAutoRemovableBlockerRemover blockerRemover;

      nsMutationEvent mutation(PR_TRUE, NS_MUTATION_SUBTREEMODIFIED);
      nsEventDispatcher::Dispatch(realTargets[k], nsnull, &mutation);
    }
  }
}

static PRUint32 GetURIHash(nsIURI* aURI)
{
  nsCAutoString str;
  aURI->GetSpec(str);
  return HashString(str);
}

void
nsDocument::AddStyleRelevantLink(nsIContent* aContent, nsIURI* aURI)
{
  nsUint32ToContentHashEntry* entry = mLinkMap.PutEntry(GetURIHash(aURI));
  if (!entry) 
    return;
  entry->PutContent(aContent);
}

void
nsDocument::ForgetLink(nsIContent* aContent)
{
  
  
  
  if (mLinkMap.Count() == 0)
    return;

  nsCOMPtr<nsIURI> uri;
  if (!aContent->IsLink(getter_AddRefs(uri)))
    return;
  PRUint32 hash = GetURIHash(uri);
  nsUint32ToContentHashEntry* entry = mLinkMap.GetEntry(hash);
  if (!entry)
    return;

  entry->RemoveContent(aContent);
  if (entry->IsEmpty()) {
    
    
    mLinkMap.RemoveEntry(hash);
  }
}

class URIVisitNotifier : public nsUint32ToContentHashEntry::Visitor
{
public:
  nsCAutoString matchURISpec;
  nsCOMArray<nsIContent> contentVisited;
  
  virtual void Visit(nsIContent* aContent) {
    
    nsCOMPtr<nsIURI> uri;
    if (!aContent->IsLink(getter_AddRefs(uri))) {
      NS_ERROR("Should have found a URI for content in the link map");
      return;
    }
    nsCAutoString spec;
    uri->GetSpec(spec);
    
    
    if (!spec.Equals(matchURISpec))
      return;

    
    
    aContent->SetLinkState(eLinkState_Unknown);
    contentVisited.AppendObject(aContent);
  }
};

void
nsDocument::NotifyURIVisitednessChanged(nsIURI* aURI)
{
  if (!mVisible) {
    mVisitednessChangedURIs.AppendObject(aURI);
    return;
  }

  nsUint32ToContentHashEntry* entry = mLinkMap.GetEntry(GetURIHash(aURI));
  if (!entry)
    return;
  
  URIVisitNotifier visitor;
  aURI->GetSpec(visitor.matchURISpec);
  entry->VisitContent(&visitor);
  for (PRUint32 count = visitor.contentVisited.Count(), i = 0; i < count; ++i) {
    ContentStatesChanged(visitor.contentVisited[i],
                         nsnull, NS_EVENT_STATE_VISITED);
  }
}

void
nsDocument::DestroyLinkMap()
{
  mVisitednessChangedURIs.Clear();
  mLinkMap.Clear();
}

void
nsDocument::UpdateLinkMap()
{
  NS_ASSERTION(mVisible,
               "Should only be updating the link map in visible documents");
  if (!mVisible)
    return;
    
  PRInt32 count = mVisitednessChangedURIs.Count();
  for (PRInt32 i = 0; i < count; ++i) {
    NotifyURIVisitednessChanged(mVisitednessChangedURIs[i]);
  }
  mVisitednessChangedURIs.Clear();
}

NS_IMETHODIMP
nsDocument::GetScriptTypeID(PRUint32 *aScriptType)
{
    NS_ERROR("No default script type here - ask some element");
    return nsIProgrammingLanguage::UNKNOWN;
}

NS_IMETHODIMP
nsDocument::SetScriptTypeID(PRUint32 aScriptType)
{
    NS_ERROR("Can't change default script type for a document");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDocument::QuerySelector(const nsAString& aSelector,
                          nsIDOMElement **aReturn)
{
  return nsGenericElement::doQuerySelector(this, aSelector, aReturn);
}

NS_IMETHODIMP
nsDocument::QuerySelectorAll(const nsAString& aSelector,
                             nsIDOMNodeList **aReturn)
{
  return nsGenericElement::doQuerySelectorAll(this, aSelector, aReturn);
}

nsresult
nsDocument::CloneDocHelper(nsDocument* clone) const
{
  
  nsresult rv = clone->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  
  clone->nsDocument::SetDocumentURI(nsIDocument::GetDocumentURI());
  
  clone->SetPrincipal(NodePrincipal());
  clone->mDocumentBaseURI = mDocumentBaseURI;

  
  PRBool hasHadScriptObject = PR_TRUE;
  nsIScriptGlobalObject* scriptObject =
    GetScriptHandlingObject(hasHadScriptObject);
  NS_ENSURE_STATE(scriptObject || !hasHadScriptObject);
  clone->SetScriptHandlingObject(scriptObject);

  
  clone->SetLoadedAsData(PR_TRUE);

  

  
  clone->mCharacterSet = mCharacterSet;
  clone->mCharacterSetSource = mCharacterSetSource;
  clone->mCompatMode = mCompatMode;
  clone->mBidiOptions = mBidiOptions;
  clone->mContentLanguage = mContentLanguage;
  clone->mContentType = mContentType;
  clone->mSecurityInfo = mSecurityInfo;

  
  clone->mIsRegularHTML = mIsRegularHTML;
  clone->mXMLDeclarationBits = mXMLDeclarationBits;
  clone->mBaseTarget = mBaseTarget;
  return NS_OK;
}

void
nsDocument::SetReadyStateInternal(ReadyState rs)
{
  mReadyState = rs;
  
}

nsIDocument::ReadyState
nsDocument::GetReadyStateEnum()
{
  return mReadyState;
}

NS_IMETHODIMP
nsDocument::GetReadyState(nsAString& aReadyState)
{
  switch(mReadyState) {
  case READYSTATE_LOADING :
    aReadyState.Assign(NS_LITERAL_STRING("loading"));
    break;
  case READYSTATE_INTERACTIVE :
    aReadyState.Assign(NS_LITERAL_STRING("interactive"));
    break;
  case READYSTATE_COMPLETE :
    aReadyState.Assign(NS_LITERAL_STRING("complete"));
    break;  
  default:
    aReadyState.Assign(NS_LITERAL_STRING("uninitialized"));
  }
  return NS_OK;
}

static PRBool
SuppressEventHandlingInDocument(nsIDocument* aDocument, void* aData)
{
  aDocument->SuppressEventHandling(*static_cast<PRUint32*>(aData));
  return PR_TRUE;
}

void
nsDocument::SuppressEventHandling(PRUint32 aIncrease)
{
  mEventsSuppressed += aIncrease;
  EnumerateSubDocuments(SuppressEventHandlingInDocument, &aIncrease);
}

static void
FireOrClearDelayedEvents(nsTArray<nsCOMPtr<nsIDocument> >& aDocuments,
                         PRBool aFireEvents)
{
  nsIFocusManager* fm = nsFocusManager::GetFocusManager();
  if (!fm)
    return;

  for (PRUint32 i = 0; i < aDocuments.Length(); ++i) {
    if (!aDocuments[i]->EventHandlingSuppressed()) {
      fm->FireDelayedEvents(aDocuments[i]);
      nsPresShellIterator iter(aDocuments[i]);
      nsCOMPtr<nsIPresShell> shell;
      while ((shell = iter.GetNextShell())) {
        shell->FireOrClearDelayedEvents(aFireEvents);
      }
    }
  }
}

void
nsDocument::MaybePreLoadImage(nsIURI* uri)
{
  
  
  
  if (nsContentUtils::IsImageInCache(uri)) {
    return;
  }

  
  nsCOMPtr<imgIRequest> request;
  nsresult rv =
    nsContentUtils::LoadImage(uri,
                              this,
                              NodePrincipal(),
                              mDocumentURI, 
                              nsnull,       
                              nsIRequest::LOAD_NORMAL,
                              getter_AddRefs(request));

  
  
  
  if (NS_SUCCEEDED(rv)) {
    mPreloadingImages.AppendObject(request);
  }
}
class nsDelayedEventDispatcher : public nsRunnable
{
public:
  nsDelayedEventDispatcher(nsTArray<nsCOMPtr<nsIDocument> >& aDocuments)
  {
    mDocuments.SwapElements(aDocuments);
  }
  virtual ~nsDelayedEventDispatcher() {}

  NS_IMETHOD Run()
  {
    FireOrClearDelayedEvents(mDocuments, PR_TRUE);
    return NS_OK;
  }

private:
  nsTArray<nsCOMPtr<nsIDocument> > mDocuments;
};

static PRBool
GetAndUnsuppressSubDocuments(nsIDocument* aDocument, void* aData)
{
  PRUint32 suppression = aDocument->EventHandlingSuppressed();
  if (suppression > 0) {
    static_cast<nsDocument*>(aDocument)->DecreaseEventSuppression();
  }
  nsTArray<nsCOMPtr<nsIDocument> >* docs =
    static_cast<nsTArray<nsCOMPtr<nsIDocument> >* >(aData);
  docs->AppendElement(aDocument);
  aDocument->EnumerateSubDocuments(GetAndUnsuppressSubDocuments, docs);
  return PR_TRUE;
}

void
nsDocument::UnsuppressEventHandlingAndFireEvents(PRBool aFireEvents)
{
  if (mEventsSuppressed > 0) {
    --mEventsSuppressed;
  }

  nsTArray<nsCOMPtr<nsIDocument> > documents;
  documents.AppendElement(this);
  EnumerateSubDocuments(GetAndUnsuppressSubDocuments, &documents);

  if (aFireEvents) {
    NS_DispatchToCurrentThread(new nsDelayedEventDispatcher(documents));
  } else {
    FireOrClearDelayedEvents(documents, PR_FALSE);
  }
}

void
nsIDocument::RegisterFreezableElement(nsIContent* aContent)
{
  if (!mFreezableElements) {
    mFreezableElements = new nsTHashtable<nsPtrHashKey<nsIContent> >();
    if (!mFreezableElements)
      return;
    mFreezableElements->Init();
  }
  mFreezableElements->PutEntry(aContent);
}

PRBool
nsIDocument::UnregisterFreezableElement(nsIContent* aContent)
{
  if (!mFreezableElements)
    return PR_FALSE;
  if (!mFreezableElements->GetEntry(aContent))
    return PR_FALSE;
  mFreezableElements->RemoveEntry(aContent);
  return PR_TRUE;
}

struct EnumerateFreezablesData {
  nsIDocument::FreezableElementEnumerator mEnumerator;
  void* mData;
};

static PLDHashOperator
EnumerateFreezables(nsPtrHashKey<nsIContent>* aEntry, void* aData)
{
  EnumerateFreezablesData* data = static_cast<EnumerateFreezablesData*>(aData);
  data->mEnumerator(aEntry->GetKey(), data->mData);
  return PL_DHASH_NEXT;
}

void
nsIDocument::EnumerateFreezableElements(FreezableElementEnumerator aEnumerator,
                                        void* aData)
{
  if (!mFreezableElements)
    return;
  EnumerateFreezablesData data = { aEnumerator, aData };
  mFreezableElements->EnumerateEntries(EnumerateFreezables, &data);
}
