









#include "mozilla/Util.h"

#ifdef MOZ_LOGGING

#define FORCE_PR_LOG 1
#endif
#include "prlog.h"
#include "plstr.h"
#include "prprf.h"

#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsDocument.h"
#include "nsUnicharUtils.h"
#include "nsContentList.h"
#include "nsIObserver.h"
#include "nsIBaseWindow.h"
#include "mozilla/css/Loader.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"
#include "nsIScriptRuntime.h"
#include "nsCOMArray.h"

#include "nsGUIEvent.h"
#include "nsAsyncDOMEvent.h"
#include "nsIDOMNodeFilter.h"

#include "nsIDOMStyleSheet.h"
#include "nsDOMAttribute.h"
#include "nsIDOMDOMStringList.h"
#include "nsIDOMDOMImplementation.h"
#include "nsIDOMDocumentXBL.h"
#include "mozilla/FunctionTimer.h"
#include "nsGenericElement.h"
#include "nsGenericHTMLElement.h"
#include "nsIDOMCDATASection.h"
#include "nsIDOMProcessingInstruction.h"
#include "nsDOMString.h"
#include "nsNodeUtils.h"
#include "nsLayoutUtils.h" 
#include "nsIFrame.h"
#include "nsITabChild.h"

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
#include "nsIJSON.h"
#include "nsThreadUtils.h"
#include "nsNodeInfoManager.h"
#include "nsIFileChannel.h"
#include "nsIMultiPartChannel.h"
#include "nsIRefreshURI.h"
#include "nsIWebNavigation.h"
#include "nsIScriptError.h"

#include "nsNetUtil.h"     

#include "nsIScriptSecurityManager.h"
#include "nsIPrincipal.h"

#include "nsIDOMWindow.h"
#include "nsPIDOMWindow.h"
#include "nsIDOMElement.h"
#include "nsFocusManager.h"


#include "nsIDOMHTMLInputElement.h"
#include "nsIRadioVisitor.h"
#include "nsIFormControl.h"

#include "nsXMLEventsManager.h"

#include "nsBidiUtils.h"

#include "nsIDOMUserDataHandler.h"
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
#include "nsBlobProtocolHandler.h"

#include "nsCharsetAlias.h"
#include "nsCharsetSource.h"
#include "nsIParser.h"
#include "nsIContentSink.h"

#include "nsDateTimeFormatCID.h"
#include "nsIDateTimeFormat.h"
#include "nsEventDispatcher.h"
#include "nsMutationEvent.h"
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
#include "nsEscape.h"
#include "nsObjectLoadingContent.h"
#include "nsHtml5TreeOpExecutor.h"
#ifdef MOZ_MEDIA
#include "nsHTMLMediaElement.h"
#endif 

#include "mozAutoDocUpdate.h"
#include "nsGlobalWindow.h"
#include "mozilla/dom/indexedDB/IndexedDatabaseManager.h"
#include "nsDOMNavigationTiming.h"
#include "nsEventStateManager.h"

#include "nsSMILAnimationController.h"
#include "imgIContainer.h"
#include "nsSVGUtils.h"

#include "nsRefreshDriver.h"


#include "nsIContentSecurityPolicy.h"
#include "nsCSPService.h"
#include "nsHTMLStyleSheet.h"
#include "nsHTMLCSSStyleSheet.h"

#include "mozilla/dom/Link.h"
#include "nsXULAppAPI.h"
#include "nsDOMTouchEvent.h"

#include "mozilla/Preferences.h"

#include "imgILoader.h"
#include "nsWrapperCacheInlines.h"

using namespace mozilla;
using namespace mozilla::dom;

typedef nsTArray<Link*> LinkArray;


nsWeakPtr nsDocument::sFullScreenDoc = nullptr;



nsWeakPtr nsDocument::sFullScreenRootDoc = nullptr;

#ifdef PR_LOGGING
static PRLogModuleInfo* gDocumentLeakPRLog;
static PRLogModuleInfo* gCspPRLog;
#endif

#define NAME_NOT_VALID ((nsSimpleContentList*)1)

nsIdentifierMapEntry::~nsIdentifierMapEntry()
{
}

void
nsIdentifierMapEntry::Traverse(nsCycleCollectionTraversalCallback* aCallback)
{
  NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(*aCallback,
                                     "mIdentifierMap mNameContentList");
  aCallback->NoteXPCOMChild(static_cast<nsIDOMNodeList*>(mNameContentList));

  NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(*aCallback, "mIdentifierMap mDocAllList");
  aCallback->NoteXPCOMChild(static_cast<nsIDOMNodeList*>(mDocAllList));

  if (mImageElement) {
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(*aCallback,
                                       "mIdentifierMap mImageElement element");
    nsIContent* imageElement = mImageElement;
    aCallback->NoteXPCOMChild(imageElement);
  }
}

bool
nsIdentifierMapEntry::IsEmpty()
{
  return mIdContentList.Count() == 0 && !mNameContentList &&
         !mChangeCallbacks && !mImageElement;
}

Element*
nsIdentifierMapEntry::GetIdElement()
{
  return static_cast<Element*>(mIdContentList.SafeElementAt(0));
}

Element*
nsIdentifierMapEntry::GetImageIdElement()
{
  return mImageElement ? mImageElement.get() : GetIdElement();
}

void
nsIdentifierMapEntry::AppendAllIdContent(nsCOMArray<nsIContent>* aElements)
{
  for (PRInt32 i = 0; i < mIdContentList.Count(); ++i) {
    aElements->AppendObject(static_cast<Element*>(mIdContentList[i]));
  }
}

void
nsIdentifierMapEntry::AddContentChangeCallback(nsIDocument::IDTargetObserver aCallback,
                                               void* aData, bool aForImage)
{
  if (!mChangeCallbacks) {
    mChangeCallbacks = new nsTHashtable<ChangeCallbackEntry>;
    if (!mChangeCallbacks)
      return;
    mChangeCallbacks->Init();
  }

  ChangeCallback cc = { aCallback, aData, aForImage };
  mChangeCallbacks->PutEntry(cc);
}

void
nsIdentifierMapEntry::RemoveContentChangeCallback(nsIDocument::IDTargetObserver aCallback,
                                                  void* aData, bool aForImage)
{
  if (!mChangeCallbacks)
    return;
  ChangeCallback cc = { aCallback, aData, aForImage };
  mChangeCallbacks->RemoveEntry(cc);
  if (mChangeCallbacks->Count() == 0) {
    mChangeCallbacks = nullptr;
  }
}

struct FireChangeArgs {
  Element* mFrom;
  Element* mTo;
  bool mImageOnly;
  bool mHaveImageOverride;
};

static PLDHashOperator
FireChangeEnumerator(nsIdentifierMapEntry::ChangeCallbackEntry *aEntry, void *aArg)
{
  FireChangeArgs* args = static_cast<FireChangeArgs*>(aArg);
  
  
  if (aEntry->mKey.mForImage ? (args->mHaveImageOverride && !args->mImageOnly) :
                               args->mImageOnly)
    return PL_DHASH_NEXT;
  return aEntry->mKey.mCallback(args->mFrom, args->mTo, aEntry->mKey.mData)
      ? PL_DHASH_NEXT : PL_DHASH_REMOVE;
}

void
nsIdentifierMapEntry::FireChangeCallbacks(Element* aOldElement,
                                          Element* aNewElement,
                                          bool aImageOnly)
{
  if (!mChangeCallbacks)
    return;

  FireChangeArgs args = { aOldElement, aNewElement, aImageOnly, !!mImageElement };
  mChangeCallbacks->EnumerateEntries(FireChangeEnumerator, &args);
}

bool
nsIdentifierMapEntry::AddIdElement(Element* aElement)
{
  NS_PRECONDITION(aElement, "Must have element");
  NS_PRECONDITION(mIdContentList.IndexOf(nullptr) < 0,
                  "Why is null in our list?");

#ifdef DEBUG
  Element* currentElement =
    static_cast<Element*>(mIdContentList.SafeElementAt(0));
#endif

  
  if (mIdContentList.Count() == 0) {
    if (!mIdContentList.AppendElement(aElement))
      return false;
    NS_ASSERTION(currentElement == nullptr, "How did that happen?");
    FireChangeCallbacks(nullptr, aElement);
    return true;
  }

  
  
  PRInt32 start = 0;
  PRInt32 end = mIdContentList.Count();
  do {
    NS_ASSERTION(start < end, "Bogus start/end");
    
    PRInt32 cur = (start + end) / 2;
    NS_ASSERTION(cur >= start && cur < end, "What happened here?");

    Element* curElement = static_cast<Element*>(mIdContentList[cur]);
    if (curElement == aElement) {
      
      
      
      return true;
    }

    if (nsContentUtils::PositionIsBefore(aElement, curElement)) {
      end = cur;
    } else {
      start = cur + 1;
    }
  } while (start != end);

  if (!mIdContentList.InsertElementAt(aElement, start))
    return false;

  if (start == 0) {
    Element* oldElement =
      static_cast<Element*>(mIdContentList.SafeElementAt(1));
    NS_ASSERTION(currentElement == oldElement, "How did that happen?");
    FireChangeCallbacks(oldElement, aElement);
  }
  return true;
}

void
nsIdentifierMapEntry::RemoveIdElement(Element* aElement)
{
  NS_PRECONDITION(aElement, "Missing element");

  
  

  
  
  
  NS_ASSERTION(!aElement->OwnerDoc()->IsHTML() ||
               mIdContentList.IndexOf(aElement) >= 0,
               "Removing id entry that doesn't exist");

  
  
  Element* currentElement =
    static_cast<Element*>(mIdContentList.SafeElementAt(0));
  mIdContentList.RemoveElement(aElement);
  if (currentElement == aElement) {
    FireChangeCallbacks(currentElement,
                        static_cast<Element*>(mIdContentList.SafeElementAt(0)));
  }
}

void
nsIdentifierMapEntry::SetImageElement(Element* aElement)
{
  Element* oldElement = GetImageIdElement();
  mImageElement = aElement;
  Element* newElement = GetImageIdElement();
  if (oldElement != newElement) {
    FireChangeCallbacks(oldElement, newElement, true);
  }
}

void
nsIdentifierMapEntry::AddNameElement(nsIDocument* aDocument, Element* aElement)
{
  if (!mNameContentList) {
    mNameContentList = new nsSimpleContentList(aDocument);
  }

  mNameContentList->AppendElement(aElement);
}

void
nsIdentifierMapEntry::RemoveNameElement(Element* aElement)
{
  if (mNameContentList) {
    mNameContentList->RemoveElement(aElement);
  }
}


size_t
nsIdentifierMapEntry::SizeOfExcludingThis(nsIdentifierMapEntry* aEntry,
                                          nsMallocSizeOfFun aMallocSizeOf,
                                          void*)
{
  return aEntry->GetKey().SizeOfExcludingThisIfUnshared(aMallocSizeOf);
}



class SubDocMapEntry : public PLDHashEntryHdr
{
public:
  
  Element *mKey; 
  nsIDocument *mSubDocument;
};

struct FindContentData
{
  FindContentData(nsIDocument *aSubDoc)
    : mSubDocument(aSubDoc), mResult(nullptr)
  {
  }

  nsISupports *mSubDocument;
  Element *mResult;
};





struct nsRadioGroupStruct
{
  nsRadioGroupStruct()
    : mRequiredRadioCount(0)
    , mGroupSuffersFromValueMissing(false)
  {}

  


  nsCOMPtr<nsIDOMHTMLInputElement> mSelectedRadioButton;
  nsCOMArray<nsIFormControl> mRadioButtons;
  PRUint32 mRequiredRadioCount;
  bool mGroupSuffersFromValueMissing;
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

DOMCI_DATA(StyleSheetList, nsDOMStyleSheetList)



NS_INTERFACE_TABLE_HEAD(nsDOMStyleSheetList)
  NS_INTERFACE_TABLE3(nsDOMStyleSheetList,
                      nsIDOMStyleSheetList,
                      nsIDocumentObserver,
                      nsIMutationObserver)
  NS_INTERFACE_TABLE_TO_MAP_SEGUE
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(StyleSheetList)
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
    return nullptr;
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
      *aReturn = nullptr;

      return NS_OK;
  }

  return CallQueryInterface(sheet, aReturn);
}

void
nsDOMStyleSheetList::NodeWillBeDestroyed(const nsINode *aNode)
{
  mDocument = nullptr;
}

void
nsDOMStyleSheetList::StyleSheetAdded(nsIDocument *aDocument,
                                     nsIStyleSheet* aStyleSheet,
                                     bool aDocumentSheet)
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
                                       bool aDocumentSheet)
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
nsOnloadBlocker::IsPending(bool *_retval)
{
  *_retval = true;
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
  *aLoadGroup = nullptr;
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
  : mHaveShutDown(false)
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
  *aPendingLoad = nullptr;
  if (mHaveShutDown) {
    return nullptr;
  }
  
  
  nsCOMPtr<nsIURI> clone;
  nsresult rv = aURI->CloneIgnoringRef(getter_AddRefs(clone));
  if (NS_FAILED(rv) || !clone) {
    return nullptr;
  }
  
  ExternalResource* resource;
  mMap.Get(clone, &resource);
  if (resource) {
    return resource->mDocument;
  }

  nsRefPtr<PendingLoad> load;
  mPendingLoads.Get(clone, getter_AddRefs(load));
  if (load) {
    load.forget(aPendingLoad);
    return nullptr;
  }

  load = new PendingLoad(aDisplayDocument);

  mPendingLoads.Put(clone, load);

  if (NS_FAILED(load->StartLoad(clone, aRequestingNode))) {
    
    
    AddExternalResource(clone, nullptr, nullptr, aDisplayDocument);
  } else {
    load.forget(aPendingLoad);
  }

  return nullptr;
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
  bool next =
    aData->mDocument ? args->callback(aData->mDocument, args->data) : true;
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

static PLDHashOperator
ExternalResourceHider(nsIURI* aKey,
                      nsExternalResourceMap::ExternalResource* aData,
                      void* aClosure)
{
  if (aData->mViewer) {
    aData->mViewer->Hide();
  }
  return PL_DHASH_NEXT;
}

void
nsExternalResourceMap::HideViewers()
{
  mMap.EnumerateRead(ExternalResourceHider, nullptr);
}

static PLDHashOperator
ExternalResourceShower(nsIURI* aKey,
                       nsExternalResourceMap::ExternalResource* aData,
                       void* aClosure)
{
  if (aData->mViewer) {
    aData->mViewer->Show();
  }
  return PL_DHASH_NEXT;
}

void
nsExternalResourceMap::ShowViewers()
{
  mMap.EnumerateRead(ExternalResourceShower, nullptr);
}

void
TransferZoomLevels(nsIDocument* aFromDoc,
                   nsIDocument* aToDoc)
{
  NS_ABORT_IF_FALSE(aFromDoc && aToDoc,
                    "transferring zoom levels from/to null doc");

  nsIPresShell* fromShell = aFromDoc->GetShell();
  if (!fromShell)
    return;

  nsPresContext* fromCtxt = fromShell->GetPresContext();
  if (!fromCtxt)
    return;

  nsIPresShell* toShell = aToDoc->GetShell();
  if (!toShell)
    return;

  nsPresContext* toCtxt = toShell->GetPresContext();
  if (!toCtxt)
    return;

  toCtxt->SetFullZoom(fromCtxt->GetFullZoom());
  toCtxt->SetMinFontSize(fromCtxt->MinFontSize(nullptr));
  toCtxt->SetTextZoom(fromCtxt->TextZoom());
}

void
TransferShowingState(nsIDocument* aFromDoc, nsIDocument* aToDoc)
{
  NS_ABORT_IF_FALSE(aFromDoc && aToDoc,
                    "transferring showing state from/to null doc");

  if (aFromDoc->IsShowing()) {
    aToDoc->OnPageShow(true, nullptr);
  }
}

nsresult
nsExternalResourceMap::AddExternalResource(nsIURI* aURI,
                                           nsIContentViewer* aViewer,
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
    doc = aViewer->GetDocument();
    NS_ASSERTION(doc, "Must have a document");

    nsCOMPtr<nsIXULDocument> xulDoc = do_QueryInterface(doc);
    if (xulDoc) {
      
      rv = NS_ERROR_NOT_AVAILABLE;
    } else {
      doc->SetDisplayDocument(aDisplayDocument);

      
      aViewer->SetSticky(false);

      rv = aViewer->Init(nullptr, nsIntRect(0, 0, 0, 0));
      if (NS_SUCCEEDED(rv)) {
        rv = aViewer->Open(nullptr, nullptr);
      }
    }
    
    if (NS_FAILED(rv)) {
      doc = nullptr;
      aViewer = nullptr;
      aLoadGroup = nullptr;
    }
  }

  ExternalResource* newResource = new ExternalResource();
  mMap.Put(aURI, newResource);

  newResource->mDocument = doc;
  newResource->mViewer = aViewer;
  newResource->mLoadGroup = aLoadGroup;
  if (doc) {
    TransferZoomLevels(aDisplayDocument, doc);
    TransferShowingState(aDisplayDocument, doc);
  }

  const nsTArray< nsCOMPtr<nsIObserver> > & obs = load->Observers();
  for (PRUint32 i = 0; i < obs.Length(); ++i) {
    obs[i]->Observe(doc, "external-resource-document-created", nullptr);
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

  nsCOMPtr<nsIContentViewer> viewer;
  nsCOMPtr<nsILoadGroup> loadGroup;
  nsresult rv = SetupViewer(aRequest, getter_AddRefs(viewer),
                            getter_AddRefs(loadGroup));

  
  nsresult rv2 = map.AddExternalResource(mURI, viewer, loadGroup,
                                         mDisplayDocument);
  if (NS_FAILED(rv)) {
    return rv;
  }
  if (NS_FAILED(rv2)) {
    mTargetListener = nullptr;
    return rv2;
  }
  
  return mTargetListener->OnStartRequest(aRequest, aContext);
}

nsresult
nsExternalResourceMap::PendingLoad::SetupViewer(nsIRequest* aRequest,
                                                nsIContentViewer** aViewer,
                                                nsILoadGroup** aLoadGroup)
{
  NS_PRECONDITION(!mTargetListener, "Unexpected call to OnStartRequest");
  *aViewer = nullptr;
  *aLoadGroup = nullptr;
  
  nsCOMPtr<nsIChannel> chan(do_QueryInterface(aRequest));
  NS_ENSURE_TRUE(chan, NS_ERROR_UNEXPECTED);

  nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(aRequest));
  if (httpChannel) {
    bool requestSucceeded;
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
                                        type.get(), nullptr, nullptr,
                                        getter_AddRefs(listener),
                                        getter_AddRefs(viewer));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(viewer, NS_ERROR_UNEXPECTED);

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
  viewer.forget(aViewer);
  newLoadGroup.forget(aLoadGroup);
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
  
  
  
  
  bool doesInheritSecurityContext;
  rv =
    NS_URIChainHasFlags(aURI,
                        nsIProtocolHandler::URI_INHERITS_SECURITY_CONTEXT,
                        &doesInheritSecurityContext);
  if (NS_FAILED(rv) || !doesInheritSecurityContext) {
    rv = requestingPrincipal->CheckMayLoad(aURI, true);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  PRInt16 shouldLoad = nsIContentPolicy::ACCEPT;
  rv = NS_CheckContentLoadPolicy(nsIContentPolicy::TYPE_OTHER,
                                 aURI,
                                 requestingPrincipal,
                                 aRequestingNode,
                                 EmptyCString(), 
                                 nullptr,         
                                 &shouldLoad,
                                 nsContentUtils::GetContentPolicy(),
                                 nsContentUtils::GetSecurityManager());
  if (NS_FAILED(rv)) return rv;
  if (NS_CP_REJECTED(shouldLoad)) {
    
    return NS_ERROR_CONTENT_BLOCKED;
  }

  nsIDocument* doc = aRequestingNode->OwnerDoc();

  nsCOMPtr<nsIInterfaceRequestor> req = nsContentUtils::GetSameOriginChecker();
  NS_ENSURE_TRUE(req, NS_ERROR_OUT_OF_MEMORY);

  nsCOMPtr<nsILoadGroup> loadGroup = doc->GetDocumentLoadGroup();
  nsCOMPtr<nsIChannel> channel;
  rv = NS_NewChannel(getter_AddRefs(channel), aURI, nullptr, loadGroup, req);
  NS_ENSURE_SUCCESS(rv, rv);

  mURI = aURI;

  return channel->AsyncOpen(this, nullptr);
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
      (IID_IS(nsIPrompt) || IID_IS(nsIAuthPrompt) || IID_IS(nsIAuthPrompt2) ||
       IID_IS(nsITabChild))) {
    return mCallbacks->GetInterface(aIID, aSink);
  }

  *aSink = nullptr;

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
    mViewer->Close(nullptr);
    mViewer->Destroy();
  }
}








class nsDOMStyleSheetSetList MOZ_FINAL : public nsIDOMDOMStringList
{
public:
  NS_DECL_ISUPPORTS

  NS_DECL_NSIDOMDOMSTRINGLIST

  nsDOMStyleSheetSetList(nsIDocument* aDocument);

  void Disconnect()
  {
    mDocument = nullptr;
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
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(DOMStringList)
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
nsDOMStyleSheetSetList::Contains(const nsAString& aString, bool *aResult)
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





class nsDOMImplementation : public nsIDOMDOMImplementation
{
public:
  nsDOMImplementation(nsIDocument* aOwner,
                      nsIScriptGlobalObject* aScriptObject,
                      nsIURI* aDocumentURI,
                      nsIURI* aBaseURI);
  virtual ~nsDOMImplementation();

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(nsDOMImplementation)

  
  NS_DECL_NSIDOMDOMIMPLEMENTATION

protected:
  nsCOMPtr<nsIDocument> mOwner;
  nsWeakPtr mScriptObject;
  nsCOMPtr<nsIURI> mDocumentURI;
  nsCOMPtr<nsIURI> mBaseURI;
};

nsDOMImplementation::nsDOMImplementation(nsIDocument* aOwner,
                                         nsIScriptGlobalObject* aScriptObject,
                                         nsIURI* aDocumentURI,
                                         nsIURI* aBaseURI)
  : mOwner(aOwner),
    mScriptObject(do_GetWeakReference(aScriptObject)),
    mDocumentURI(aDocumentURI),
    mBaseURI(aBaseURI)
{
}

nsDOMImplementation::~nsDOMImplementation()
{
}

DOMCI_DATA(DOMImplementation, nsDOMImplementation)


NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsDOMImplementation)
  NS_INTERFACE_MAP_ENTRY(nsIDOMDOMImplementation)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMDOMImplementation)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(DOMImplementation)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTION_1(nsDOMImplementation, mOwner)

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsDOMImplementation)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsDOMImplementation)


NS_IMETHODIMP
nsDOMImplementation::HasFeature(const nsAString& aFeature,
                                const nsAString& aVersion,
                                bool* aReturn)
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
  *aReturn = nullptr;
  NS_ENSURE_STATE(mOwner);

  nsresult rv = nsContentUtils::CheckQName(aQualifiedName);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIAtom> name = do_GetAtom(aQualifiedName);
  NS_ENSURE_TRUE(name, NS_ERROR_OUT_OF_MEMORY);

  
  return NS_NewDOMDocumentType(aReturn, mOwner->NodeInfoManager(),
                               name, aPublicId,
                               aSystemId, NullString());
}

NS_IMETHODIMP
nsDOMImplementation::CreateDocument(const nsAString& aNamespaceURI,
                                    const nsAString& aQualifiedName,
                                    nsIDOMDocumentType* aDoctype,
                                    nsIDOMDocument** aReturn)
{
  *aReturn = nullptr;

  nsresult rv;
  if (!aQualifiedName.IsEmpty()) {
    const nsAFlatString& qName = PromiseFlatString(aQualifiedName);
    const PRUnichar *colon;
    rv = nsContentUtils::CheckQName(qName, true, &colon);
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

  nsCOMPtr<nsIScriptGlobalObject> scriptHandlingObject =
    do_QueryReferent(mScriptObject);
  
  NS_ENSURE_STATE(!mScriptObject || scriptHandlingObject);

  nsCOMPtr<nsIDOMDocument> document;

  rv = nsContentUtils::CreateDocument(aNamespaceURI, aQualifiedName, aDoctype,
                                      mDocumentURI, mBaseURI, 
                                      mOwner->NodePrincipal(),
                                      scriptHandlingObject,
                                        DocumentFlavorLegacyGuess,
                                      getter_AddRefs(document));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDocument> doc = do_QueryInterface(document);
  doc->SetReadyStateInternal(nsIDocument::READYSTATE_COMPLETE);

  document.forget(aReturn);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMImplementation::CreateHTMLDocument(const nsAString& aTitle,
                                        nsIDOMDocument** aReturn)
{
  *aReturn = nullptr;
  NS_ENSURE_STATE(mOwner);

  nsCOMPtr<nsIDOMDocumentType> doctype;
  
  nsresult rv = NS_NewDOMDocumentType(getter_AddRefs(doctype),
                                      mOwner->NodeInfoManager(),
                                      nsGkAtoms::html, 
                                      EmptyString(), 
                                      EmptyString(), 
                                      NullString()); 
  NS_ENSURE_SUCCESS(rv, rv);


  nsCOMPtr<nsIScriptGlobalObject> scriptHandlingObject =
    do_QueryReferent(mScriptObject);

  NS_ENSURE_STATE(!mScriptObject || scriptHandlingObject);
                                                       
  nsCOMPtr<nsIDOMDocument> document;
  rv = nsContentUtils::CreateDocument(EmptyString(), EmptyString(),
                                      doctype, mDocumentURI, mBaseURI,
                                      mOwner->NodePrincipal(),
                                      scriptHandlingObject,
                                      DocumentFlavorLegacyGuess,
                                      getter_AddRefs(document));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIDocument> doc = do_QueryInterface(document);

  nsCOMPtr<nsIContent> root;
  rv = doc->CreateElem(NS_LITERAL_STRING("html"), NULL, kNameSpaceID_XHTML,
                       getter_AddRefs(root));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = doc->AppendChildTo(root, false);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIContent> head;
  rv = doc->CreateElem(NS_LITERAL_STRING("head"), NULL, kNameSpaceID_XHTML,
                       getter_AddRefs(head));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = root->AppendChildTo(head, false);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIContent> title;
  rv = doc->CreateElem(NS_LITERAL_STRING("title"), NULL, kNameSpaceID_XHTML,
                       getter_AddRefs(title));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = head->AppendChildTo(title, false);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIContent> titleText;
  rv = NS_NewTextNode(getter_AddRefs(titleText), doc->NodeInfoManager());
  NS_ENSURE_SUCCESS(rv, rv);
  rv = titleText->SetText(aTitle, false);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = title->AppendChildTo(titleText, false);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIContent> body;
  rv = doc->CreateElem(NS_LITERAL_STRING("body"), NULL, kNameSpaceID_XHTML,
                       getter_AddRefs(body));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = root->AppendChildTo(body, false);
  NS_ENSURE_SUCCESS(rv, rv);

  doc->SetReadyStateInternal(nsIDocument::READYSTATE_COMPLETE);

  document.forget(aReturn);

  return NS_OK;
}




nsIDocument::nsIDocument()
  : nsINode(nullptr),
    mCharacterSet(NS_LITERAL_CSTRING("ISO-8859-1")),
    mNodeInfoManager(nullptr),
    mCompatMode(eCompatibility_FullStandards),
    mIsInitialDocumentInWindow(false),
    mMayStartLayout(true),
    mVisible(true),
    mRemovedFromDocShell(false),
    
    
    
    
    mAllowDNSPrefetch(true),
    mIsBeingUsedAsImage(false),
    mHasLinksToUpdate(false),
    mPartID(0)
{
  SetInDocument();
}




nsDocument::nsDocument(const char* aContentType)
  : nsIDocument()
  , mAnimatingImages(true)
  , mVisibilityState(eHidden)
{
  SetContentTypeInternal(nsDependentCString(aContentType));

#ifdef PR_LOGGING
  if (!gDocumentLeakPRLog)
    gDocumentLeakPRLog = PR_NewLogModule("DocumentLeak");

  if (gDocumentLeakPRLog)
    PR_LOG(gDocumentLeakPRLog, PR_LOG_DEBUG,
           ("DOCUMENT %p created", this));

  if (!gCspPRLog)
    gCspPRLog = PR_NewLogModule("CSP");
#endif

  
  SetDOMStringToNull(mLastStyleSheetSet);

  mLinksToUpdate.Init();
}

static PLDHashOperator
ClearAllBoxObjects(nsIContent* aKey, nsPIBoxObject* aBoxObject, void* aUserArg)
{
  if (aBoxObject) {
    aBoxObject->Clear();
  }
  return PL_DHASH_NEXT;
}

nsIDocument::~nsIDocument()
{
  if (mNodeInfoManager) {
    mNodeInfoManager->DropDocumentReference();
  }
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

  NS_ASSERTION(!mIsShowing, "Destroying a currently-showing document");

  mInDestructor = true;
  mInUnlinkOrDeletion = true;

  
  mObservers.Clear();

  if (mStyleSheetSetList) {
    mStyleSheetSetList->Disconnect();
  }

  if (mAnimationController) {
    mAnimationController->Disconnect();
  }

  mParentDocument = nullptr;

  
  
  if (mSubDocuments) {
    PL_DHashTableDestroy(mSubDocuments);

    mSubDocuments = nullptr;
  }

  
  
  DestroyElementMaps();

  nsAutoScriptBlocker scriptBlocker;

  PRInt32 indx; 
  PRUint32 count = mChildren.ChildCount();
  for (indx = PRInt32(count) - 1; indx >= 0; --indx) {
    mChildren.ChildAt(indx)->UnbindFromTree();
    mChildren.RemoveChildAt(indx);
  }
  mFirstChild = nullptr;
  mCachedRootElement = nullptr;

  
  indx = mStyleSheets.Count();
  while (--indx >= 0) {
    mStyleSheets[indx]->SetOwningDocument(nullptr);
  }
  indx = mCatalogSheets.Count();
  while (--indx >= 0) {
    mCatalogSheets[indx]->SetOwningDocument(nullptr);
  }
  if (mAttrStyleSheet) {
    mAttrStyleSheet->SetOwningDocument(nullptr);
    NS_RELEASE(mAttrStyleSheet);
  }
  if (mStyleAttrStyleSheet)
    mStyleAttrStyleSheet->SetOwningDocument(nullptr);

  if (mListenerManager) {
    mListenerManager->Disconnect();
    UnsetFlags(NODE_HAS_LISTENERMANAGER);
  }

  if (mScriptLoader) {
    mScriptLoader->DropDocumentReference();
  }

  if (mCSSLoader) {
    
    mCSSLoader->DropDocumentReference();
  }

  delete mHeaderData;

  if (mBoxObjectTable) {
    mBoxObjectTable->EnumerateRead(ClearAllBoxObjects, nullptr);
    delete mBoxObjectTable;
  }

  mPendingTitleChangeEvent.Revoke();

  for (PRUint32 i = 0; i < mFileDataUris.Length(); ++i) {
    nsBlobProtocolHandler::RemoveFileDataEntry(mFileDataUris[i]);
  }

  
  
  SetImageLockingState(false);
  mImageTracker.Clear();

  mPlugins.Clear();
}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsDocument)

NS_INTERFACE_TABLE_HEAD(nsDocument)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_DOCUMENT_INTERFACE_TABLE_BEGIN(nsDocument)
    NS_INTERFACE_TABLE_ENTRY(nsDocument, nsIDocument)
    NS_INTERFACE_TABLE_ENTRY(nsDocument, nsIDOMDocumentXBL)
    NS_INTERFACE_TABLE_ENTRY(nsDocument, nsIScriptObjectPrincipal)
    NS_INTERFACE_TABLE_ENTRY(nsDocument, nsIDOMEventTarget)
    NS_INTERFACE_TABLE_ENTRY(nsDocument, nsISupportsWeakReference)
    NS_INTERFACE_TABLE_ENTRY(nsDocument, nsIRadioGroupContainer)
    NS_INTERFACE_TABLE_ENTRY(nsDocument, nsIMutationObserver)
    NS_INTERFACE_TABLE_ENTRY(nsDocument, nsIApplicationCacheContainer)
    NS_INTERFACE_TABLE_ENTRY(nsDocument, nsIDOMDocumentTouch)
    NS_INTERFACE_TABLE_ENTRY(nsDocument, nsITouchEventReceiver)
    NS_INTERFACE_TABLE_ENTRY(nsDocument, nsIInlineEventHandlers)
    NS_INTERFACE_TABLE_ENTRY(nsDocument, nsIObserver)
  NS_OFFSET_AND_INTERFACE_TABLE_END
  NS_OFFSET_AND_INTERFACE_TABLE_TO_MAP_SEGUE
  NS_INTERFACE_MAP_ENTRIES_CYCLE_COLLECTION(nsDocument)
  NS_INTERFACE_MAP_ENTRY_TEAROFF(nsIDOMXPathNSResolver,
                                 new nsNode3Tearoff(this))
  NS_INTERFACE_MAP_ENTRY_TEAROFF(nsIDOMNodeSelector,
                                 new nsNodeSelectorTearoff(this))
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


NS_IMPL_CYCLE_COLLECTING_ADDREF(nsDocument)
NS_IMPL_CYCLE_COLLECTING_RELEASE_WITH_DESTROY(nsDocument, 
                                              nsNodeUtils::LastRelease(this))

NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_BEGIN(nsDocument)
  if (nsGenericElement::CanSkip(tmp, aRemovingAllowed)) {
    nsEventListenerManager* elm = tmp->GetListenerManager(false);
    if (elm) {
      elm->UnmarkGrayJSListeners();
    }
    return true;
  }
NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_END

NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_IN_CC_BEGIN(nsDocument)
  return nsGenericElement::CanSkipInCC(tmp);
NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_IN_CC_END

NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_THIS_BEGIN(nsDocument)
  return nsGenericElement::CanSkipThis(tmp);
NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_THIS_END

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
BoxObjectTraverser(nsIContent* key, nsPIBoxObject* boxObject, void* userArg)
{
  nsCycleCollectionTraversalCallback *cb = 
    static_cast<nsCycleCollectionTraversalCallback*>(userArg);
 
  NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(*cb, "mBoxObjectTable entry");
  cb->NoteXPCOMChild(boxObject);

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

static const char* kNSURIs[] = {
  "([none])",
  "(xmlns)",
  "(xml)",
  "(xhtml)",
  "(XLink)",
  "(XSLT)",
  "(XBL)",
  "(MathML)",
  "(RDF)",
  "(XUL)"
};

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INTERNAL(nsDocument)
  if (NS_UNLIKELY(cb.WantDebugInfo())) {
    char name[512];
    nsCAutoString loadedAsData;
    if (tmp->IsLoadedAsData()) {
      loadedAsData.AssignLiteral("data");
    } else {
      loadedAsData.AssignLiteral("normal");
    }
    PRUint32 nsid = tmp->GetDefaultNamespaceID();
    nsCAutoString uri;
    if (tmp->mDocumentURI)
      tmp->mDocumentURI->GetSpec(uri);
    if (nsid < ArrayLength(kNSURIs)) {
      PR_snprintf(name, sizeof(name), "nsDocument %s %s %s",
                  loadedAsData.get(), kNSURIs[nsid], uri.get());
    }
    else {
      PR_snprintf(name, sizeof(name), "nsDocument %s %s",
                  loadedAsData.get(), uri.get());
    }
    cb.DescribeRefCountedNode(tmp->mRefCnt.get(), sizeof(nsDocument), name);
  }
  else {
    NS_IMPL_CYCLE_COLLECTION_DESCRIBE(nsDocument, tmp->mRefCnt.get())
  }

  
  
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS

  if (!nsINode::Traverse(tmp, cb)) {
    return NS_SUCCESS_INTERRUPTED_TRAVERSE;
  }

  tmp->mIdentifierMap.EnumerateEntries(IdentifierMapEntryTraverse, &cb);

  tmp->mExternalResourceMap.Traverse(&cb);

  
  for (PRInt32 indx = PRInt32(tmp->mChildren.ChildCount()); indx > 0; --indx) {
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mChildren[i]");
    cb.NoteXPCOMChild(tmp->mChildren.ChildAt(indx - 1));
  }

  
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mSecurityInfo)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mDisplayDocument)

  
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mParser)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mScriptGlobalObject)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NATIVE_MEMBER(mListenerManager,
                                                  nsEventListenerManager)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mDOMStyleSheets)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mScriptLoader)

  tmp->mRadioGroups.EnumerateRead(RadioGroupsTraverser, &cb);

  
  
  if (tmp->mBoxObjectTable) {
    tmp->mBoxObjectTable->EnumerateRead(BoxObjectTraverser, &cb);
  }

  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mChannel)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR_AMBIGUOUS(mStyleAttrStyleSheet, nsIStyleSheet)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mXPathEvaluatorTearoff)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mLayoutHistoryState)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnloadBlocker)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mFirstBaseNodeWithHref)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mDOMImplementation)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR_AMBIGUOUS(mImageMaps,
                                                       nsIDOMNodeList)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOriginalDocument)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mCachedEncoder)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mStateObjectCached)

  
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMARRAY(mStyleSheets)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMARRAY(mCatalogSheets)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMARRAY(mPreloadingImages)

  for (PRUint32 i = 0; i < tmp->mFrameRequestCallbacks.Length(); ++i) {
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mFrameRequestCallbacks[i]");
    cb.NoteXPCOMChild(tmp->mFrameRequestCallbacks[i]);
  }

  
  if (tmp->mAnimationController) {
    tmp->mAnimationController->Traverse(&cb);
  }

  if (tmp->mSubDocuments && tmp->mSubDocuments->ops) {
    PL_DHashTableEnumerate(tmp->mSubDocuments, SubDocTraverser, &cb);
  }
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END


NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(nsDocument)
  nsINode::Trace(tmp, aCallback, aClosure);
NS_IMPL_CYCLE_COLLECTION_TRACE_END


NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsDocument)
  tmp->mInUnlinkOrDeletion = true;

  
  tmp->mExternalResourceMap.Shutdown();

  nsAutoScriptBlocker scriptBlocker;

  nsINode::Unlink(tmp);

  
  for (PRInt32 indx = PRInt32(tmp->mChildren.ChildCount()) - 1; 
       indx >= 0; --indx) {
    tmp->mChildren.ChildAt(indx)->UnbindFromTree();
    tmp->mChildren.RemoveChildAt(indx);
  }
  tmp->mFirstChild = nullptr;

  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mXPathEvaluatorTearoff)
  tmp->mCachedRootElement = nullptr; 
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mDisplayDocument)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mFirstBaseNodeWithHref)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mDOMImplementation)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mImageMaps)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOriginalDocument)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mCachedEncoder)

  tmp->mParentDocument = nullptr;

  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMARRAY(mPreloadingImages)


  if (tmp->mBoxObjectTable) {
   tmp->mBoxObjectTable->EnumerateRead(ClearAllBoxObjects, nullptr);
   delete tmp->mBoxObjectTable;
   tmp->mBoxObjectTable = nullptr;
 }

  if (tmp->mListenerManager) {
    tmp->mListenerManager->Disconnect();
    tmp->UnsetFlags(NODE_HAS_LISTENERMANAGER);
    tmp->mListenerManager = nullptr;
  }

  if (tmp->mSubDocuments) {
    PL_DHashTableDestroy(tmp->mSubDocuments);
    tmp->mSubDocuments = nullptr;
  }

  tmp->mFrameRequestCallbacks.Clear();

  tmp->mRadioGroups.Clear();

  
  
  

  tmp->mIdentifierMap.Clear();

  if (tmp->mAnimationController) {
    tmp->mAnimationController->Unlink();
  }

  tmp->mPendingTitleChangeEvent.Revoke();

  tmp->mInUnlinkOrDeletion = false;
NS_IMPL_CYCLE_COLLECTION_UNLINK_END


nsresult
nsDocument::Init()
{
  if (mCSSLoader || mNodeInfoManager || mScriptLoader) {
    return NS_ERROR_ALREADY_INITIALIZED;
  }

  mIdentifierMap.Init();
  mStyledLinks.Init();
  mRadioGroups.Init();

  
  nsINode::nsSlots* slots = GetSlots();

  
  
  
  
  NS_ENSURE_TRUE(slots->mMutationObservers.PrependElementUnlessExists(static_cast<nsIMutationObserver*>(this)),
                 NS_ERROR_OUT_OF_MEMORY);


  mOnloadBlocker = new nsOnloadBlocker();
  mCSSLoader = new mozilla::css::Loader(this);
  
  mCSSLoader->SetCompatibilityMode(eCompatibility_FullStandards);

  mNodeInfoManager = new nsNodeInfoManager();
  nsresult  rv = mNodeInfoManager->Init(this);
  NS_ENSURE_SUCCESS(rv, rv);

  
  mNodeInfo = mNodeInfoManager->GetDocumentNodeInfo();
  NS_ENSURE_TRUE(mNodeInfo, NS_ERROR_OUT_OF_MEMORY);
  NS_ABORT_IF_FALSE(mNodeInfo->NodeType() == nsIDOMNode::DOCUMENT_NODE,
                    "Bad NodeType in aNodeInfo");

  NS_ASSERTION(OwnerDoc() == this, "Our nodeinfo is busted!");

  mScriptLoader = new nsScriptLoader(this);

  mImageTracker.Init();
  mPlugins.Init();

  return NS_OK;
}

void
nsIDocument::DeleteAllProperties()
{
  for (PRUint32 i = 0; i < GetPropertyTableCount(); ++i) {
    PropertyTable(i)->DeleteAllProperties();
  }
}

void
nsIDocument::DeleteAllPropertiesFor(nsINode* aNode)
{
  for (PRUint32 i = 0; i < GetPropertyTableCount(); ++i) {
    PropertyTable(i)->DeleteAllPropertiesFor(aNode);
  }
}

nsPropertyTable*
nsIDocument::GetExtraPropertyTable(PRUint16 aCategory)
{
  NS_ASSERTION(aCategory > 0, "Category 0 should have already been handled");
  while (aCategory >= mExtraPropertyTables.Length() + 1) {
    mExtraPropertyTables.AppendElement(new nsPropertyTable());
  }
  return mExtraPropertyTables[aCategory - 1];
}

void
nsDocument::AddXMLEventsContent(nsIContent *aXMLEventsElement)
{
  if (!mXMLEventsManager) {
    mXMLEventsManager = new nsXMLEventsManager();
    AddObserver(mXMLEventsManager);
  }
  mXMLEventsManager->AddXMLEventsContent(aXMLEventsElement);
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

  mSecurityInfo = nullptr;

  mDocumentLoadGroup = nullptr;

  
  
  if (mSubDocuments) {
    PL_DHashTableDestroy(mSubDocuments);

    mSubDocuments = nullptr;
  }

  
  
  DestroyElementMaps();

  bool oldVal = mInUnlinkOrDeletion;
  mInUnlinkOrDeletion = true;
  PRUint32 count = mChildren.ChildCount();
  { 
    MOZ_AUTO_DOC_UPDATE(this, UPDATE_CONTENT_MODEL, true);    
    for (PRInt32 i = PRInt32(count) - 1; i >= 0; i--) {
      nsCOMPtr<nsIContent> content = mChildren.ChildAt(i);

      nsIContent* previousSibling = content->GetPreviousSibling();

      if (nsINode::GetFirstChild() == content) {
        mFirstChild = content->GetNextSibling();
      }
      mChildren.RemoveChildAt(i);
      nsNodeUtils::ContentRemoved(this, content, i, previousSibling);
      content->UnbindFromTree();
    }
  }
  mInUnlinkOrDeletion = oldVal;
  mCachedRootElement = nullptr;

  
  ResetStylesheetsToURI(aURI);
  
  
  if (mListenerManager) {
    mListenerManager->Disconnect();
    mListenerManager = nullptr;
  }

  
  mDOMStyleSheets = nullptr;

  
  
  
  
  SetPrincipal(nullptr);

  
  mOriginalURI = nullptr;

  SetDocumentURI(aURI);
  
  
  mDocumentBaseURI = nullptr;

  if (aLoadGroup) {
    mDocumentLoadGroup = do_GetWeakReference(aLoadGroup);
    
    
    

    
    
  }

  mLastModified.Truncate();
  
  
  SetContentTypeInternal(EmptyCString());
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
      nsCOMPtr<nsIDocShell> docShell = do_QueryReferent(mDocumentContainer);

      if (!docShell && aLoadGroup) {
        nsCOMPtr<nsIInterfaceRequestor> cbs;
        aLoadGroup->GetNotificationCallbacks(getter_AddRefs(cbs));
        docShell = do_GetInterface(cbs);
      }

      MOZ_ASSERT(docShell,
                 "must be in a docshell or pass in an explicit principal");

      nsCOMPtr<nsIPrincipal> principal;
      nsresult rv = securityManager->
        GetDocShellCodebasePrincipal(mDocumentURI, docShell,
                                     getter_AddRefs(principal));
      if (NS_SUCCEEDED(rv)) {
        SetPrincipal(principal);
      }
    }
  }

  
  nsPIDOMWindow* win = GetInnerWindow();
  if (win) {
    win->RefreshCompartmentPrincipal();
  }
}

nsresult
nsDocument::ResetStylesheetsToURI(nsIURI* aURI)
{
  NS_PRECONDITION(aURI, "Null URI passed to ResetStylesheetsToURI");

  mozAutoDocUpdate upd(this, UPDATE_STYLE, true);
  
  
  PRInt32 indx = mStyleSheets.Count();
  while (--indx >= 0) {
    nsIStyleSheet* sheet = mStyleSheets[indx];
    sheet->SetOwningDocument(nullptr);

    if (sheet->IsApplicable()) {
      RemoveStyleSheetFromStyleSets(sheet);
    }

    
  }

  indx = mCatalogSheets.Count();
  while (--indx >= 0) {
    nsIStyleSheet* sheet = mCatalogSheets[indx];
    sheet->SetOwningDocument(nullptr);

    if (sheet->IsApplicable()) {
      nsCOMPtr<nsIPresShell> shell = GetShell();
      if (shell) {
        shell->StyleSet()->RemoveStyleSheet(nsStyleSet::eAgentSheet, sheet);
      }
    }

    
  }


  
  mStyleSheets.Clear();
  
  
  

  
  nsresult rv = NS_OK;
  if (mAttrStyleSheet) {
    
    nsCOMPtr<nsIPresShell> shell = GetShell();
    if (shell) {
      shell->StyleSet()->RemoveStyleSheet(nsStyleSet::ePresHintSheet,
                                          mAttrStyleSheet);
    }
    mAttrStyleSheet->Reset(aURI);
  } else {
    rv = NS_NewHTMLStyleSheet(&mAttrStyleSheet, aURI, this);
    if (NS_FAILED(rv)) {
      NS_IF_RELEASE(mAttrStyleSheet);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  
  
  mAttrStyleSheet->SetOwningDocument(this);
  
  if (mStyleAttrStyleSheet) {
    
    nsCOMPtr<nsIPresShell> shell = GetShell();
    if (shell) {
      shell->StyleSet()->
        RemoveStyleSheet(nsStyleSet::eStyleAttrSheet, mStyleAttrStyleSheet);
    }
    mStyleAttrStyleSheet->Reset(aURI);
  } else {
    mStyleAttrStyleSheet = new nsHTMLCSSStyleSheet();
    NS_ENSURE_TRUE(mStyleAttrStyleSheet, NS_ERROR_OUT_OF_MEMORY);
    rv = mStyleAttrStyleSheet->Init(aURI, this);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  mStyleAttrStyleSheet->SetOwningDocument(this);

  
  nsCOMPtr<nsIPresShell> shell = GetShell();
  if (shell) {
    FillStyleSet(shell->StyleSet());
  }

  return rv;
}

void
nsDocument::FillStyleSet(nsStyleSet* aStyleSet)
{
  NS_PRECONDITION(aStyleSet, "Must have a style set");
  NS_PRECONDITION(aStyleSet->SheetCount(nsStyleSet::ePresHintSheet) == 0,
                  "Style set already has a preshint sheet?");
  NS_PRECONDITION(aStyleSet->SheetCount(nsStyleSet::eDocSheet) == 0,
                  "Style set already has document sheets?");
  NS_PRECONDITION(aStyleSet->SheetCount(nsStyleSet::eStyleAttrSheet) == 0,
                  "Style set already has style attr sheets?");
  NS_PRECONDITION(mStyleAttrStyleSheet, "No style attr stylesheet?");
  NS_PRECONDITION(mAttrStyleSheet, "No attr stylesheet?");
  
  aStyleSet->AppendStyleSheet(nsStyleSet::ePresHintSheet, mAttrStyleSheet);

  aStyleSet->AppendStyleSheet(nsStyleSet::eStyleAttrSheet,
                              mStyleAttrStyleSheet);

  PRInt32 i;
  for (i = mStyleSheets.Count() - 1; i >= 0; --i) {
    nsIStyleSheet* sheet = mStyleSheets[i];
    if (sheet->IsApplicable()) {
      aStyleSet->AddDocStyleSheet(sheet, this);
    }
  }

  for (i = mCatalogSheets.Count() - 1; i >= 0; --i) {
    nsIStyleSheet* sheet = mCatalogSheets[i];
    if (sheet->IsApplicable()) {
      aStyleSet->AppendStyleSheet(nsStyleSet::eAgentSheet, sheet);
    }
  }
}

nsresult
nsDocument::StartDocumentLoad(const char* aCommand, nsIChannel* aChannel,
                              nsILoadGroup* aLoadGroup,
                              nsISupports* aContainer,
                              nsIStreamListener **aDocListener,
                              bool aReset, nsIContentSink* aSink)
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

  MOZ_ASSERT(GetReadyStateEnum() == nsIDocument::READYSTATE_UNINITIALIZED,
             "Bad readyState");
  SetReadyStateInternal(READYSTATE_LOADING);

  if (nsCRT::strcmp(kLoadAsData, aCommand) == 0) {
    mLoadedAsData = true;
    
    
    

    
    ScriptLoader()->SetEnabled(false);

    
    CSSLoader()->SetEnabled(false); 
  } else if (nsCRT::strcmp("external-resource", aCommand) == 0) {
    
    ScriptLoader()->SetEnabled(false);
  }

  mMayStartLayout = false;

  mHaveInputEncoding = true;

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
    SetContentTypeInternal(Substring(start, semicolon));
  }

  RetrieveRelevantHeaders(aChannel);

  mChannel = aChannel;

  nsresult rv = InitCSP();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
nsDocument::InitCSP()
{
  if (CSPService::sCSPEnabled) {
    nsAutoString cspHeaderValue;
    nsAutoString cspROHeaderValue;

    this->GetHeaderData(nsGkAtoms::headerCSP, cspHeaderValue);
    this->GetHeaderData(nsGkAtoms::headerCSPReportOnly, cspROHeaderValue);

    bool system = false;
    nsIScriptSecurityManager *ssm = nsContentUtils::GetSecurityManager();

    if (NS_SUCCEEDED(ssm->IsSystemPrincipal(NodePrincipal(), &system)) && system) {
      
      return NS_OK;
    }

    if (cspHeaderValue.IsEmpty() && cspROHeaderValue.IsEmpty()) {
      
      return NS_OK;
    }

#ifdef PR_LOGGING 
    PR_LOG(gCspPRLog, PR_LOG_DEBUG, ("CSP header specified for document %p", this));
#endif

    nsresult rv;
    nsCOMPtr<nsIContentSecurityPolicy> mCSP;
    mCSP = do_CreateInstance("@mozilla.org/contentsecuritypolicy;1", &rv);

    if (NS_FAILED(rv)) {
#ifdef PR_LOGGING 
      PR_LOG(gCspPRLog, PR_LOG_DEBUG, ("Failed to create CSP object: %x", rv));
#endif
      return rv;
    }

    
    nsCOMPtr<nsIHttpChannel> httpChannel = do_QueryInterface(mChannel);
    mCSP->ScanRequestData(httpChannel);

    
    nsCOMPtr<nsIURI> chanURI;
    mChannel->GetURI(getter_AddRefs(chanURI));

#ifdef PR_LOGGING 
    PR_LOG(gCspPRLog, PR_LOG_DEBUG, ("CSP Loaded"));
#endif

    
    
    
    
    if (cspHeaderValue.IsEmpty()) {
      mCSP->SetReportOnlyMode(true);

      
      
      
      nsCharSeparatedTokenizer tokenizer(cspROHeaderValue, ',');
      while (tokenizer.hasMoreTokens()) {
        const nsSubstring& policy = tokenizer.nextToken();
        mCSP->RefinePolicy(policy, chanURI);
#ifdef PR_LOGGING
        {
          PR_LOG(gCspPRLog, PR_LOG_DEBUG,
                  ("CSP (report only) refined with policy: \"%s\"",
                    NS_ConvertUTF16toUTF8(policy).get()));
        }
#endif
      }
    } else {
      
      

      
      
      
      nsCharSeparatedTokenizer tokenizer(cspHeaderValue, ',');
      while (tokenizer.hasMoreTokens()) {
        const nsSubstring& policy = tokenizer.nextToken();
        mCSP->RefinePolicy(policy, chanURI);
#ifdef PR_LOGGING
        {
          PR_LOG(gCspPRLog, PR_LOG_DEBUG,
                ("CSP refined with policy: \"%s\"",
                  NS_ConvertUTF16toUTF8(policy).get()));
        }
#endif
      }
    }

    
    nsCOMPtr<nsIDocShell> docShell = do_QueryReferent(mDocumentContainer);
    if (docShell) {
        bool safeAncestry = false;

        
        rv = mCSP->PermitsAncestry(docShell, &safeAncestry);
        NS_ENSURE_SUCCESS(rv, rv);

        if (!safeAncestry) {
#ifdef PR_LOGGING
            PR_LOG(gCspPRLog, PR_LOG_DEBUG, 
                   ("CSP doesn't like frame's ancestry, not loading."));
#endif
            
            mChannel->Cancel(NS_ERROR_CSP_FRAME_ANCESTOR_VIOLATION);
        }
    }

    
    nsIPrincipal* principal = GetPrincipal();

    if (principal) {
        principal->SetCsp(mCSP);
#ifdef PR_LOGGING
        PR_LOG(gCspPRLog, PR_LOG_DEBUG, 
                ("Inserted CSP into principal %p", principal));
    }
    else {
      PR_LOG(gCspPRLog, PR_LOG_DEBUG, 
              ("Couldn't copy CSP into absent principal %p", principal));
#endif
    }
  }
#ifdef PR_LOGGING
  else { 
    PR_LOG(gCspPRLog, PR_LOG_DEBUG, 
           ("CSP is disabled, skipping CSP init for document %p", this));
  }
#endif
  return NS_OK;
}

void
nsDocument::StopDocumentLoad()
{
  if (mParser) {
    mParserAborted = true;
    mParser->Terminate();
  }
}

void
nsDocument::SetDocumentURI(nsIURI* aURI)
{
  nsCOMPtr<nsIURI> oldBase = GetDocBaseURI();
  mDocumentURI = NS_TryToMakeImmutable(aURI);
  nsIURI* newBase = GetDocBaseURI();

  bool equalBases = false;
  if (oldBase && newBase) {
    oldBase->Equals(newBase, &equalBases);
  }
  else {
    equalBases = !oldBase && !newBase;
  }

  
  
  if (!mOriginalURI)
    mOriginalURI = mDocumentURI;

  
  
  if (!equalBases) {
    RefreshLinkHrefs();
  }
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
nsDocument::AddToNameTable(Element *aElement, nsIAtom* aName)
{
  nsIdentifierMapEntry *entry =
    mIdentifierMap.PutEntry(nsDependentAtomString(aName));

  
  if (entry) {
    entry->AddNameElement(this, aElement);
  }
}

void
nsDocument::RemoveFromNameTable(Element *aElement, nsIAtom* aName)
{
  
  if (mIdentifierMap.Count() == 0)
    return;

  nsIdentifierMapEntry *entry =
    mIdentifierMap.GetEntry(nsDependentAtomString(aName));
  if (!entry) 
    return;

  entry->RemoveNameElement(aElement);
}

void
nsDocument::AddToIdTable(Element *aElement, nsIAtom* aId)
{
  nsIdentifierMapEntry *entry =
    mIdentifierMap.PutEntry(nsDependentAtomString(aId));

  if (entry) { 
    entry->AddIdElement(aElement);
  }
}

void
nsDocument::RemoveFromIdTable(Element *aElement, nsIAtom* aId)
{
  NS_ASSERTION(aId, "huhwhatnow?");

  
  if (mIdentifierMap.Count() == 0) {
    return;
  }

  nsIdentifierMapEntry *entry =
    mIdentifierMap.GetEntry(nsDependentAtomString(aId));
  if (!entry) 
    return;

  entry->RemoveIdElement(aElement);
  if (entry->IsEmpty()) {
    mIdentifierMap.RawRemoveEntry(entry);
  }
}

nsIPrincipal*
nsDocument::GetPrincipal()
{
  return NodePrincipal();
}

extern bool sDisablePrefetchHTTPSPref;

void
nsDocument::SetPrincipal(nsIPrincipal *aNewPrincipal)
{
  if (aNewPrincipal && mAllowDNSPrefetch && sDisablePrefetchHTTPSPref) {
    nsCOMPtr<nsIURI> uri;
    aNewPrincipal->GetURI(getter_AddRefs(uri));
    bool isHTTPS;
    if (!uri || NS_FAILED(uri->SchemeIs("https", &isHTTPS)) ||
        isHTTPS) {
      mAllowDNSPrefetch = false;
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
  CopyUTF8toUTF16(GetContentTypeInternal(), aContentType);

  return NS_OK;
}

void
nsDocument::SetContentType(const nsAString& aContentType)
{
  NS_ASSERTION(GetContentTypeInternal().IsEmpty() ||
               GetContentTypeInternal().Equals(NS_ConvertUTF16toUTF8(aContentType)),
               "Do you really want to change the content-type?");

  SetContentTypeInternal(NS_ConvertUTF16toUTF8(aContentType));
}



NS_IMETHODIMP
nsDocument::HasFocus(bool* aResult)
{
  *aResult = false;

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
      
      *aResult = true;
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
  *aElement = nullptr;

  
  nsCOMPtr<nsPIDOMWindow> window = GetWindow();
  if (window) {
    nsCOMPtr<nsPIDOMWindow> focusedWindow;
    nsIContent* focusedContent =
      nsFocusManager::GetFocusedDescendant(window, false,
                                           getter_AddRefs(focusedWindow));
    
    if (focusedContent && focusedContent->OwnerDoc() == this) {
      if (focusedContent->IsInNativeAnonymousSubtree()) {
        focusedContent = focusedContent->FindFirstNonNativeAnonymous();
      }
      if (focusedContent) {
        CallQueryInterface(focusedContent, aElement);
        return NS_OK;
      }
    }
  }

  
  nsCOMPtr<nsIDOMHTMLDocument> htmlDoc = do_QueryObject(this);
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
nsDocument::GetCurrentScript(nsIDOMElement **aElement)
{
  nsIScriptElement* script = mScriptLoader->GetCurrentScript();
  if (script) {
    return CallQueryInterface(script, aElement);
  }
  
  *aElement = nullptr;

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::ElementFromPoint(float aX, float aY, nsIDOMElement** aReturn)
{
  return ElementFromPointHelper(aX, aY, false, true, aReturn);
}

nsresult
nsDocument::ElementFromPointHelper(float aX, float aY,
                                   bool aIgnoreRootScrollFrame,
                                   bool aFlushLayout,
                                   nsIDOMElement** aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);
  *aReturn = nullptr;
  
  if (!aIgnoreRootScrollFrame && (aX < 0 || aY < 0))
    return NS_OK;

  nscoord x = nsPresContext::CSSPixelsToAppUnits(aX);
  nscoord y = nsPresContext::CSSPixelsToAppUnits(aY);
  nsPoint pt(x, y);

  
  
  if (aFlushLayout)
    FlushPendingNotifications(Flush_Layout);

  nsIPresShell *ps = GetShell();
  NS_ENSURE_STATE(ps);
  nsIFrame *rootFrame = ps->GetRootFrame();

  
  if (!rootFrame)
    return NS_OK; 

  nsIFrame *ptFrame = nsLayoutUtils::GetFrameForPoint(rootFrame, pt, true,
                                                      aIgnoreRootScrollFrame);
  if (!ptFrame)
    return NS_OK;

  nsIContent* elem = GetContentInThisDocument(ptFrame);
  if (elem && !elem->IsElement()) {
    elem = elem->GetParent();
  }
  if (elem) {
    CallQueryInterface(elem, aReturn);
  }
  return NS_OK;
}

nsresult
nsDocument::NodesFromRectHelper(float aX, float aY,
                                float aTopSize, float aRightSize,
                                float aBottomSize, float aLeftSize,
                                bool aIgnoreRootScrollFrame,
                                bool aFlushLayout,
                                nsIDOMNodeList** aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);
  
  nsSimpleContentList* elements = new nsSimpleContentList(this);
  NS_ADDREF(elements);
  *aReturn = elements;

  
  
  if (!aIgnoreRootScrollFrame && (aX < 0 || aY < 0))
    return NS_OK;

  nscoord x = nsPresContext::CSSPixelsToAppUnits(aX - aLeftSize);
  nscoord y = nsPresContext::CSSPixelsToAppUnits(aY - aTopSize);
  nscoord w = nsPresContext::CSSPixelsToAppUnits(aLeftSize + aRightSize) + 1;
  nscoord h = nsPresContext::CSSPixelsToAppUnits(aTopSize + aBottomSize) + 1;

  nsRect rect(x, y, w, h);

  
  
  if (aFlushLayout) {
    FlushPendingNotifications(Flush_Layout);
  }

  nsIPresShell *ps = GetShell();
  NS_ENSURE_STATE(ps);
  nsIFrame *rootFrame = ps->GetRootFrame();

  
  if (!rootFrame)
    return NS_OK; 

  nsAutoTArray<nsIFrame*,8> outFrames;
  nsLayoutUtils::GetFramesForArea(rootFrame, rect, outFrames,
                                  true, aIgnoreRootScrollFrame);

  
  nsIContent* lastAdded = nullptr;

  for (PRUint32 i = 0; i < outFrames.Length(); i++) {
    nsIContent* node = GetContentInThisDocument(outFrames[i]);

    if (node && !node->IsElement() && !node->IsNodeOfType(nsINode::eTEXT)) {
      
      
      node = node->GetParent();
    }
    if (node && node != lastAdded) {
      elements->AppendElement(node);
      lastAdded = node;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetElementsByClassName(const nsAString& aClasses,
                                   nsIDOMNodeList** aReturn)
{
  return nsContentUtils::GetElementsByClassName(this, aClasses, aReturn);
}

NS_IMETHODIMP
nsDocument::ReleaseCapture()
{
  
  
  nsCOMPtr<nsIDOMNode> node = do_QueryInterface(nsIPresShell::GetCapturingContent());
  if (node && nsContentUtils::CanCallerAccess(node)) {
    nsIPresShell::SetCapturingContent(nullptr, 0);
  }
  return NS_OK;
}

nsresult
nsDocument::SetBaseURI(nsIURI* aURI)
{
  if (!aURI && !mDocumentBaseURI) {
    return NS_OK;
  }
  
  
  if (aURI && mDocumentBaseURI) {
    bool equalBases = false;
    mDocumentBaseURI->Equals(aURI, &equalBases);
    if (equalBases) {
      return NS_OK;
    }
  }

  if (aURI) {
    mDocumentBaseURI = NS_TryToMakeImmutable(aURI);
  } else {
    mDocumentBaseURI = nullptr;
  }
  RefreshLinkHrefs();

  return NS_OK;
}

void
nsDocument::GetBaseTarget(nsAString &aBaseTarget)
{
  aBaseTarget = mBaseTarget;
}

void
nsDocument::SetDocumentCharacterSet(const nsACString& aCharSetID)
{
  if (!mCharacterSet.Equals(aCharSetID)) {
    mCharacterSet = aCharSetID;

#ifdef DEBUG
    nsCAutoString canonicalName;
    nsCharsetAlias::GetPreferred(aCharSetID, canonicalName);
    NS_ASSERTION(canonicalName.Equals(aCharSetID),
                 "charset name must be canonical");
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
    bool found = false;
    do {  
      if (data->mField == aHeaderField) {
        if (!aData.IsEmpty()) {
          data->mData.Assign(aData);
        }
        else {  
          *lastPtr = data->mNext;
          data->mNext = nullptr;
          delete data;
        }
        found = true;

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

  if (aHeaderField == nsGkAtoms::headerDefaultStyle) {
    
    
    if (DOMStringIsNull(mLastStyleSheetSet)) {
      
      
      
      
      
      EnableStyleSheetsForSetInternal(aData, true);
    }
  }

  if (aHeaderField == nsGkAtoms::refresh) {
    
    
    nsCOMPtr<nsIRefreshURI> refresher = do_QueryReferent(mDocumentContainer);
    if (refresher) {
      
      
      
      
      
      refresher->SetupRefreshURIFromHeader(mDocumentURI, NodePrincipal(),
                                           NS_ConvertUTF16toUTF8(aData));
    }
  }

  if (aHeaderField == nsGkAtoms::headerDNSPrefetchControl &&
      mAllowDNSPrefetch) {
    
    mAllowDNSPrefetch = aData.IsEmpty() || aData.LowerCaseEqualsLiteral("on");
  }
}

bool
nsDocument::TryChannelCharset(nsIChannel *aChannel,
                              PRInt32& aCharsetSource,
                              nsACString& aCharset,
                              nsHtml5TreeOpExecutor* aExecutor)
{
  if(kCharsetFromChannel <= aCharsetSource) {
    return true;
  }

  if (aChannel) {
    nsCAutoString charsetVal;
    nsresult rv = aChannel->GetContentCharset(charsetVal);
    if (NS_SUCCEEDED(rv)) {
      nsCAutoString preferred;
      rv = nsCharsetAlias::GetPreferred(charsetVal, preferred);
      if(NS_SUCCEEDED(rv)) {
        aCharset = preferred;
        aCharsetSource = kCharsetFromChannel;
        return true;
      } else if (aExecutor && !charsetVal.IsEmpty()) {
        aExecutor->ComplainAboutBogusProtocolCharset(this);
      }
    }
  }
  return false;
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
  *aInstancePtrResult = nullptr;

  NS_ASSERTION(!mPresShell, "We have a presshell already!");

  NS_ENSURE_FALSE(GetBFCacheEntry(), NS_ERROR_FAILURE);

  FillStyleSet(aStyleSet);
  
  nsCOMPtr<nsIPresShell> shell;
  nsresult rv = NS_NewPresShell(getter_AddRefs(shell));
  if (NS_FAILED(rv)) {
    return rv;
  }

  rv = shell->Init(this, aContext, aViewManager, aStyleSet, aCompatMode);
  NS_ENSURE_SUCCESS(rv, rv);

  
  mPresShell = shell;

  mExternalResourceMap.ShowViewers();

  MaybeRescheduleAnimationFrameNotifications();

  shell.swap(*aInstancePtrResult);

  return NS_OK;
}

void
nsDocument::MaybeRescheduleAnimationFrameNotifications()
{
  if (!mPresShell || !IsEventHandlingEnabled()) {
    
    return;
  }

  nsRefreshDriver* rd = mPresShell->GetPresContext()->RefreshDriver();
  if (!mFrameRequestCallbacks.IsEmpty()) {
    rd->ScheduleFrameRequestCallbacks(this);
  }
}

void
nsIDocument::TakeFrameRequestCallbacks(FrameRequestCallbackList& aCallbacks)
{
  aCallbacks.AppendElements(mFrameRequestCallbacks);
  mFrameRequestCallbacks.Clear();
}

PLDHashOperator RequestDiscardEnumerator(imgIRequest* aKey,
                                         PRUint32 aData,
                                         void* userArg)
{
  aKey->RequestDiscard();
  return PL_DHASH_NEXT;
}

void
nsDocument::DeleteShell()
{
  mExternalResourceMap.HideViewers();
  if (IsEventHandlingEnabled()) {
    RevokeAnimationFrameNotifications();
  }

  
  
  
  mImageTracker.EnumerateRead(RequestDiscardEnumerator, nullptr);

  mPresShell = nullptr;
}

void
nsDocument::RevokeAnimationFrameNotifications()
{
  if (!mFrameRequestCallbacks.IsEmpty()) {
    mPresShell->GetPresContext()->RefreshDriver()->
      RevokeFrameRequestCallbacks(this);
  }
}

static void
SubDocClearEntry(PLDHashTable *table, PLDHashEntryHdr *entry)
{
  SubDocMapEntry *e = static_cast<SubDocMapEntry *>(entry);

  NS_RELEASE(e->mKey);
  if (e->mSubDocument) {
    e->mSubDocument->SetParentDocument(nullptr);
    NS_RELEASE(e->mSubDocument);
  }
}

static bool
SubDocInitEntry(PLDHashTable *table, PLDHashEntryHdr *entry, const void *key)
{
  SubDocMapEntry *e =
    const_cast<SubDocMapEntry *>
              (static_cast<const SubDocMapEntry *>(entry));

  e->mKey = const_cast<Element*>(static_cast<const Element*>(key));
  NS_ADDREF(e->mKey);

  e->mSubDocument = nullptr;
  return true;
}

nsresult
nsDocument::SetSubDocumentFor(Element* aElement, nsIDocument* aSubDoc)
{
  NS_ENSURE_TRUE(aElement, NS_ERROR_UNEXPECTED);

  if (!aSubDoc) {
    

    if (mSubDocuments) {
      SubDocMapEntry *entry =
        static_cast<SubDocMapEntry*>
                   (PL_DHashTableOperate(mSubDocuments, aElement,
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

      mSubDocuments = PL_NewDHashTable(&hash_table_ops, nullptr,
                                       sizeof(SubDocMapEntry), 16);
      if (!mSubDocuments) {
        return NS_ERROR_OUT_OF_MEMORY;
      }
    }

    
    SubDocMapEntry *entry =
      static_cast<SubDocMapEntry*>
                 (PL_DHashTableOperate(mSubDocuments, aElement,
                                       PL_DHASH_ADD));

    if (!entry) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    if (entry->mSubDocument) {
      entry->mSubDocument->SetParentDocument(nullptr);

      
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
  if (mSubDocuments && aContent->IsElement()) {
    SubDocMapEntry *entry =
      static_cast<SubDocMapEntry*>
                 (PL_DHashTableOperate(mSubDocuments, aContent->AsElement(),
                                       PL_DHASH_LOOKUP));

    if (PL_DHASH_ENTRY_IS_BUSY(entry)) {
      return entry->mSubDocument;
    }
  }

  return nullptr;
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

Element*
nsDocument::FindContentForSubDocument(nsIDocument *aDocument) const
{
  NS_ENSURE_TRUE(aDocument, nullptr);

  if (!mSubDocuments) {
    return nullptr;
  }

  FindContentData data(aDocument);
  PL_DHashTableEnumerate(mSubDocuments, FindContentEnumerator, &data);

  return data.mResult;
}

bool
nsDocument::IsNodeOfType(PRUint32 aFlags) const
{
    return !(aFlags & ~eDOCUMENT);
}

Element*
nsIDocument::GetRootElement() const
{
  return (mCachedRootElement && mCachedRootElement->GetNodeParent() == this) ?
         mCachedRootElement : GetRootElementInternal();
}

Element*
nsDocument::GetRootElementInternal() const
{
  
  
  PRUint32 i;
  for (i = mChildren.ChildCount(); i > 0; --i) {
    nsIContent* child = mChildren.ChildAt(i - 1);
    if (child->IsElement()) {
      const_cast<nsDocument*>(this)->mCachedRootElement = child->AsElement();
      return child->AsElement();
    }
  }
  
  const_cast<nsDocument*>(this)->mCachedRootElement = nullptr;
  return nullptr;
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
                          bool aNotify)
{
  if (aKid->IsElement() && GetRootElement()) {
    NS_ERROR("Inserting element child when we already have one");
    return NS_ERROR_DOM_HIERARCHY_REQUEST_ERR;
  }

  return doInsertChildAt(aKid, aIndex, aNotify, mChildren);
}

nsresult
nsDocument::AppendChildTo(nsIContent* aKid, bool aNotify)
{
  
  
  
  
  
  return nsDocument::InsertChildAt(aKid, GetChildCount(), aNotify);
}

void
nsDocument::RemoveChildAt(PRUint32 aIndex, bool aNotify)
{
  nsCOMPtr<nsIContent> oldKid = GetChildAt(aIndex);
  if (!oldKid) {
    return;
  }

  if (oldKid->IsElement()) {
    
    DestroyElementMaps();
  }

  doRemoveChildAt(aIndex, aNotify, oldKid, mChildren);
  mCachedRootElement = nullptr;
}

PRInt32
nsDocument::GetNumberOfStyleSheets() const
{
  return mStyleSheets.Count();
}

nsIStyleSheet*
nsDocument::GetStyleSheetAt(PRInt32 aIndex) const
{
  NS_ENSURE_TRUE(0 <= aIndex && aIndex < mStyleSheets.Count(), nullptr);
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
  nsCOMPtr<nsIPresShell> shell = GetShell();
  if (shell) {
    shell->StyleSet()->AddDocStyleSheet(aSheet, this);
  }
}

void
nsDocument::AddStyleSheet(nsIStyleSheet* aSheet)
{
  NS_PRECONDITION(aSheet, "null arg");
  mStyleSheets.AppendObject(aSheet);
  aSheet->SetOwningDocument(this);

  if (aSheet->IsApplicable()) {
    AddStyleSheetToStyleSets(aSheet);
  }

  NS_DOCUMENT_NOTIFY_OBSERVERS(StyleSheetAdded, (this, aSheet, true));
}

void
nsDocument::RemoveStyleSheetFromStyleSets(nsIStyleSheet* aSheet)
{
  nsCOMPtr<nsIPresShell> shell = GetShell();
  if (shell) {
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
    if (aSheet->IsApplicable()) {
      RemoveStyleSheetFromStyleSets(aSheet);
    }

    NS_DOCUMENT_NOTIFY_OBSERVERS(StyleSheetRemoved, (this, aSheet, true));
  }

  aSheet->SetOwningDocument(nullptr);
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
      if (newSheet->IsApplicable()) {
        AddStyleSheetToStyleSets(newSheet);
      }

      NS_DOCUMENT_NOTIFY_OBSERVERS(StyleSheetAdded, (this, newSheet, true));
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

  if (aSheet->IsApplicable()) {
    AddStyleSheetToStyleSets(aSheet);
  }

  NS_DOCUMENT_NOTIFY_OBSERVERS(StyleSheetAdded, (this, aSheet, true));
}


void
nsDocument::SetStyleSheetApplicableState(nsIStyleSheet* aSheet,
                                         bool aApplicable)
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
  NS_ENSURE_TRUE(0 <= aIndex && aIndex < mCatalogSheets.Count(), nullptr);
  return mCatalogSheets[aIndex];
}

void
nsDocument::AddCatalogStyleSheet(nsIStyleSheet* aSheet)
{
  mCatalogSheets.AppendObject(aSheet);
  aSheet->SetOwningDocument(this);

  if (aSheet->IsApplicable()) {
    
    nsCOMPtr<nsIPresShell> shell = GetShell();
    if (shell) {
      shell->StyleSet()->AppendStyleSheet(nsStyleSet::eAgentSheet, aSheet);
    }
  }
                                                                                
  NS_DOCUMENT_NOTIFY_OBSERVERS(StyleSheetAdded, (this, aSheet, false));
}

void
nsDocument::EnsureCatalogStyleSheet(const char *aStyleSheetURI)
{
  mozilla::css::Loader* cssLoader = CSSLoader();
  if (cssLoader->GetEnabled()) {
    PRInt32 sheetCount = GetNumberOfCatalogStyleSheets();
    for (PRInt32 i = 0; i < sheetCount; i++) {
      nsIStyleSheet* sheet = GetCatalogStyleSheetAt(i);
      NS_ASSERTION(sheet, "unexpected null stylesheet in the document");
      if (sheet) {
        nsCAutoString uriStr;
        sheet->GetSheetURI()->GetSpec(uriStr);
        if (uriStr.Equals(aStyleSheetURI))
          return;
      }
    }

    nsCOMPtr<nsIURI> uri;
    NS_NewURI(getter_AddRefs(uri), aStyleSheetURI);
    if (uri) {
      nsRefPtr<nsCSSStyleSheet> sheet;
      cssLoader->LoadSheetSync(uri, true, true, getter_AddRefs(sheet));
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

static void
NotifyActivityChanged(nsIContent *aContent, void *aUnused)
{
#ifdef MOZ_MEDIA
  nsCOMPtr<nsIDOMHTMLMediaElement> domMediaElem(do_QueryInterface(aContent));
  if (domMediaElem) {
    nsHTMLMediaElement* mediaElem = static_cast<nsHTMLMediaElement*>(aContent);
    mediaElem->NotifyOwnerDocumentActivityChanged();
  }
#endif
  nsCOMPtr<nsIObjectLoadingContent> objectLoadingContent(do_QueryInterface(aContent));
  if (objectLoadingContent) {
    nsObjectLoadingContent* olc = static_cast<nsObjectLoadingContent*>(objectLoadingContent.get());
    olc->NotifyOwnerDocumentActivityChanged();
  }
}

void
nsIDocument::SetContainer(nsISupports* aContainer)
{
  mDocumentContainer = do_GetWeakReference(aContainer);
  EnumerateFreezableElements(NotifyActivityChanged, nullptr);
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
  NS_ABORT_IF_FALSE(aScriptGlobalObject || !mAnimationController ||
                    mAnimationController->IsPausedByType(
                        nsSMILTimeContainer::PAUSE_PAGEHIDE |
                        nsSMILTimeContainer::PAUSE_BEGIN),
                    "Clearing window pointer while animations are unpaused");

  if (mScriptGlobalObject && !aScriptGlobalObject) {
    
    
    mLayoutHistoryState = GetLayoutHistoryState();

    if (mPresShell && !EventHandlingSuppressed()) {
      RevokeAnimationFrameNotifications();
    }

    
    if (mOnloadBlockCount != 0) {
      nsCOMPtr<nsILoadGroup> loadGroup = GetDocumentLoadGroup();
      if (loadGroup) {
        loadGroup->RemoveRequest(mOnloadBlocker, nullptr, NS_OK);
      }
    }
  }

  mScriptGlobalObject = aScriptGlobalObject;

  if (aScriptGlobalObject) {
    mScriptObject = nullptr;
    mHasHadScriptHandlingObject = true;
    
    mLayoutHistoryState = nullptr;
    mScopeObject = do_GetWeakReference(aScriptGlobalObject);

#ifdef DEBUG
    if (!mWillReparent) {
      
      
      JSObject *obj = GetWrapperPreserveColor();
      if (obj) {
        JSObject *newScope = aScriptGlobalObject->GetGlobalJSObject();
        nsIScriptContext *scx = aScriptGlobalObject->GetContext();
        JSContext *cx = scx ? scx->GetNativeContext() : nullptr;
        if (!cx) {
          nsContentUtils::ThreadJSContextStack()->Peek(&cx);
          if (!cx) {
            cx = nsContentUtils::ThreadJSContextStack()->GetSafeJSContext();
            NS_ASSERTION(cx, "Uhoh, no context, this is bad!");
          }
        }
        if (cx) {
          NS_ASSERTION(JS_GetGlobalForObject(cx, obj) == newScope,
                       "Wrong scope, this is really bad!");
        }
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
        bool allowDNSPrefetch;
        docShell->GetAllowDNSPrefetch(&allowDNSPrefetch);
        mAllowDNSPrefetch = allowDNSPrefetch;
      }
    }

    MaybeRescheduleAnimationFrameNotifications();
  }

  
  
  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(mScriptGlobalObject);
  mWindow = window;

  
  
  
  
  
  mVisibilityState = GetVisibilityState();
}

nsIScriptGlobalObject*
nsDocument::GetScriptHandlingObjectInternal() const
{
  NS_ASSERTION(!mScriptGlobalObject,
               "Do not call this when mScriptGlobalObject is set!");

  nsCOMPtr<nsIScriptGlobalObject> scriptHandlingObject =
    do_QueryReferent(mScriptObject);
  nsCOMPtr<nsPIDOMWindow> win = do_QueryInterface(scriptHandlingObject);
  if (win) {
    NS_ASSERTION(win->IsInnerWindow(), "Should have inner window here!");
    nsPIDOMWindow* outer = win->GetOuterWindow();
    if (!outer || outer->GetCurrentInnerWindow() != win) {
      NS_WARNING("Wrong inner/outer window combination!");
      return nullptr;
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
    mHasHadScriptHandlingObject = true;
  }
}

nsPIDOMWindow *
nsDocument::GetWindowInternal() const
{
  NS_ASSERTION(!mWindow, "This should not be called when mWindow is not null!");

  nsCOMPtr<nsPIDOMWindow> win(do_QueryInterface(GetScriptGlobalObject()));

  if (!win) {
    return nullptr;
  }

  return win->GetOuterWindow();
}

nsPIDOMWindow *
nsDocument::GetInnerWindowInternal()
{
  NS_ASSERTION(mRemovedFromDocShell,
               "This document should have been removed from docshell!");

  nsCOMPtr<nsPIDOMWindow> win(do_QueryInterface(GetScriptGlobalObject()));

  return win;
}

nsScriptLoader*
nsDocument::ScriptLoader()
{
  return mScriptLoader;
}

bool
nsDocument::InternalAllowXULXBL()
{
  if (nsContentUtils::AllowXULXBLForPrincipal(NodePrincipal())) {
    mAllowXULXBL = eTriTrue;
    return true;
  }

  mAllowXULXBL = eTriFalse;
  return false;
}



void
nsDocument::AddObserver(nsIDocumentObserver* aObserver)
{
  NS_ASSERTION(mObservers.IndexOf(aObserver) == nsTArray<int>::NoIndex,
               "Observer already in the list");
  mObservers.AppendElement(aObserver);
  AddMutationObserver(aObserver);
}

bool
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
      mInXBLUpdate = false;
      BindingManager()->EndOutermostUpdate();
    } else if (!mInDestructor) {
      nsContentUtils::AddScriptRunner(
        NS_NewRunnableMethod(this, &nsDocument::MaybeEndOutermostXBLUpdate));
    }
  }
}

void
nsDocument::BeginUpdate(nsUpdateType aUpdateType)
{
  if (mUpdateNestLevel == 0 && !mInXBLUpdate) {
    mInXBLUpdate = true;
    BindingManager()->BeginOutermostUpdate();
  }
  
  ++mUpdateNestLevel;
  nsContentUtils::AddScriptBlocker();
  NS_DOCUMENT_NOTIFY_OBSERVERS(BeginUpdate, (this, aUpdateType));
}

void
nsDocument::EndUpdate(nsUpdateType aUpdateType)
{
  NS_DOCUMENT_NOTIFY_OBSERVERS(EndUpdate, (this, aUpdateType));

  nsContentUtils::RemoveScriptBlocker();

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

void
nsDocument::ReportEmptyGetElementByIdArg()
{
  nsContentUtils::ReportToConsole(nsIScriptError::warningFlag,
                                  "DOM", this,
                                  nsContentUtils::eDOM_PROPERTIES,
                                  "EmptyGetElementByIdParam");
}

Element*
nsDocument::GetElementById(const nsAString& aElementId)
{
  if (!CheckGetElementByIdArg(aElementId)) {
    return nullptr;
  }

  nsIdentifierMapEntry *entry = mIdentifierMap.GetEntry(aElementId);
  return entry ? entry->GetIdElement() : nullptr;
}

const nsSmallVoidArray*
nsDocument::GetAllElementsForId(const nsAString& aElementId) const
{
  if (aElementId.IsEmpty()) {
    return nullptr;
  }

  nsIdentifierMapEntry *entry = mIdentifierMap.GetEntry(aElementId);
  return entry ? entry->GetIdElements() : nullptr;  
}

NS_IMETHODIMP
nsDocument::GetElementById(const nsAString& aId, nsIDOMElement** aReturn)
{
  Element *content = GetElementById(aId);
  if (content) {
    return CallQueryInterface(content, aReturn);
  }

  *aReturn = nullptr;

  return NS_OK;
}

Element*
nsDocument::AddIDTargetObserver(nsIAtom* aID, IDTargetObserver aObserver,
                                void* aData, bool aForImage)
{
  nsDependentAtomString id(aID);

  if (!CheckGetElementByIdArg(id))
    return nullptr;

  nsIdentifierMapEntry *entry = mIdentifierMap.PutEntry(id);
  NS_ENSURE_TRUE(entry, nullptr);

  entry->AddContentChangeCallback(aObserver, aData, aForImage);
  return aForImage ? entry->GetImageIdElement() : entry->GetIdElement();
}

void
nsDocument::RemoveIDTargetObserver(nsIAtom* aID, IDTargetObserver aObserver,
                                   void* aData, bool aForImage)
{
  nsDependentAtomString id(aID);

  if (!CheckGetElementByIdArg(id))
    return;

  nsIdentifierMapEntry *entry = mIdentifierMap.GetEntry(id);
  if (!entry) {
    return;
  }

  entry->RemoveContentChangeCallback(aObserver, aData, aForImage);
}

NS_IMETHODIMP
nsDocument::MozSetImageElement(const nsAString& aImageElementId,
                               nsIDOMElement* aImageElement)
{
  if (aImageElementId.IsEmpty())
    return NS_OK;

  
  
  nsAutoScriptBlocker scriptBlocker;

  nsCOMPtr<nsIContent> content = do_QueryInterface(aImageElement);
  nsIdentifierMapEntry *entry = mIdentifierMap.PutEntry(aImageElementId);
  if (entry) {
    entry->SetImageElement(content ? content->AsElement() : nullptr);
    if (entry->IsEmpty()) {
      mIdentifierMap.RemoveEntry(aImageElementId);
    }
  }
  return NS_OK;
}

Element*
nsDocument::LookupImageElement(const nsAString& aId)
{
  if (aId.IsEmpty())
    return nullptr;

  nsIdentifierMapEntry *entry = mIdentifierMap.GetEntry(aId);
  return entry ? entry->GetImageIdElement() : nullptr;
}

void
nsDocument::DispatchContentLoadedEvents()
{
  NS_TIME_FUNCTION;
  
  
  
  
  mPreloadingImages.Clear();

  if (mTiming) {
    mTiming->NotifyDOMContentLoadedStart(nsIDocument::GetDocumentURI());
  }
    
  
  
  
  nsContentUtils::DispatchTrustedEvent(this, static_cast<nsIDocument*>(this),
                                       NS_LITERAL_STRING("DOMContentLoaded"),
                                       true, true);

  if (mTiming) {
    mTiming->NotifyDOMContentLoadedEnd(nsIDocument::GetDocumentURI());
  }

  
  
  
  

  
  
  
  nsCOMPtr<nsIDOMEventTarget> target_frame;

  if (mParentDocument) {
    target_frame =
      do_QueryInterface(mParentDocument->FindContentForSubDocument(this));
  }

  if (target_frame) {
    nsCOMPtr<nsIDocument> parent = mParentDocument;
    do {
      nsCOMPtr<nsIDOMDocument> domDoc = do_QueryInterface(parent);

      nsCOMPtr<nsIDOMEvent> event;
      if (domDoc) {
        domDoc->CreateEvent(NS_LITERAL_STRING("Events"),
                            getter_AddRefs(event));

      }

      if (event) {
        event->InitEvent(NS_LITERAL_STRING("DOMFrameContentLoaded"), true,
                         true);

        event->SetTarget(target_frame);
        event->SetTrusted(true);

        
        
        
        
        

        nsEvent* innerEvent = event->GetInternalNSEvent();
        if (innerEvent) {
          nsEventStatus status = nsEventStatus_eIgnore;

          nsIPresShell *shell = parent->GetShell();
          if (shell) {
            nsRefPtr<nsPresContext> context = shell->GetPresContext();

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

  
  
  Element* root = GetRootElement();
  if (root && root->HasAttr(kNameSpaceID_None, nsGkAtoms::manifest)) {
    nsContentUtils::DispatchChromeEvent(this, static_cast<nsIDocument*>(this),
                                        NS_LITERAL_STRING("MozApplicationManifest"),
                                        true, true);
  }

  UnblockOnload(true);
}

void
nsDocument::EndLoad()
{
  
  
  
  if (mParser) {
    mWeakSink = do_GetWeakReference(mParser->GetContentSink());
    mParser = nullptr;
  }
  
  NS_DOCUMENT_NOTIFY_OBSERVERS(EndLoad, (this));
  
  if (!mSynchronousDOMContentLoaded) {
    nsRefPtr<nsIRunnable> ev =
      NS_NewRunnableMethod(this, &nsDocument::DispatchContentLoadedEvents);
    NS_DispatchToCurrentThread(ev);
  } else {
    DispatchContentLoadedEvents();
  }
}

void
nsDocument::ContentStateChanged(nsIContent* aContent, nsEventStates aStateMask)
{
  NS_PRECONDITION(!nsContentUtils::IsSafeToRunScript(),
                  "Someone forgot a scriptblocker");
  NS_DOCUMENT_NOTIFY_OBSERVERS(ContentStateChanged,
                               (this, aContent, aStateMask));
}

void
nsDocument::DocumentStatesChanged(nsEventStates aStateMask)
{
  
  mGotDocumentState &= ~aStateMask;
  mDocumentState &= ~aStateMask;

  NS_DOCUMENT_NOTIFY_OBSERVERS(DocumentStatesChanged, (this, aStateMask));
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





nsIContent*
nsIDocument::GetDocumentType() const
{
  for (nsIContent* child = GetFirstChild();
       child;
       child = child->GetNextSibling()) {
    if (child->NodeType() == nsIDOMNode::DOCUMENT_TYPE_NODE) {
      return child;
    }
  }
  return NULL;
}

NS_IMETHODIMP
nsDocument::GetDoctype(nsIDOMDocumentType** aDoctype)
{
  MOZ_ASSERT(aDoctype);
  nsCOMPtr<nsIDOMDocumentType> doctype = do_QueryInterface(GetDocumentType());
  doctype.forget(aDoctype);
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetImplementation(nsIDOMDOMImplementation** aImplementation)
{
  if (!mDOMImplementation) {
    nsCOMPtr<nsIURI> uri;
    NS_NewURI(getter_AddRefs(uri), "about:blank");
    NS_ENSURE_TRUE(uri, NS_ERROR_OUT_OF_MEMORY);
    bool hasHadScriptObject = true;
    nsIScriptGlobalObject* scriptObject =
      GetScriptHandlingObject(hasHadScriptObject);
    NS_ENSURE_STATE(scriptObject || !hasHadScriptObject);
    mDOMImplementation = new nsDOMImplementation(this, scriptObject, uri, uri);
    if (!mDOMImplementation) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  NS_ADDREF(*aImplementation = mDOMImplementation);

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetDocumentElement(nsIDOMElement** aDocumentElement)
{
  NS_ENSURE_ARG_POINTER(aDocumentElement);

  Element* root = GetRootElement();
  if (root) {
    return CallQueryInterface(root, aDocumentElement);
  }

  *aDocumentElement = nullptr;

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::CreateElement(const nsAString& aTagName,
                          nsIDOMElement** aReturn)
{
  *aReturn = nullptr;
  nsCOMPtr<nsIContent> content;
  nsresult rv = CreateElement(aTagName, getter_AddRefs(content));
  NS_ENSURE_SUCCESS(rv, rv);
  return CallQueryInterface(content, aReturn);
}

bool IsLowercaseASCII(const nsAString& aValue)
{
  PRInt32 len = aValue.Length();
  for (PRInt32 i = 0; i < len; ++i) {
    PRUnichar c = aValue[i];
    if (!(0x0061 <= (c) && ((c) <= 0x007a))) {
      return false;
    }
  }
  return true;
}

nsresult
nsDocument::CreateElement(const nsAString& aTagName,
                          nsIContent** aReturn)
{
  *aReturn = nullptr;

  nsresult rv = nsContentUtils::CheckQName(aTagName, false);
  NS_ENSURE_SUCCESS(rv, rv);

  bool needsLowercase = IsHTML() && !IsLowercaseASCII(aTagName);
  nsAutoString lcTagName;
  if (needsLowercase) {
    nsContentUtils::ASCIIToLower(aTagName, lcTagName);
  }

  rv = CreateElem(needsLowercase ? lcTagName : aTagName,
                  nullptr, mDefaultElementType, aReturn);
  return rv;
}

NS_IMETHODIMP
nsDocument::CreateElementNS(const nsAString& aNamespaceURI,
                            const nsAString& aQualifiedName,
                            nsIDOMElement** aReturn)
{
  *aReturn = nullptr;
  nsCOMPtr<nsIContent> content;
  nsresult rv = CreateElementNS(aNamespaceURI, aQualifiedName,
                                getter_AddRefs(content));
  NS_ENSURE_SUCCESS(rv, rv);
  return CallQueryInterface(content, aReturn);
}

nsresult
nsDocument::CreateElementNS(const nsAString& aNamespaceURI,
                            const nsAString& aQualifiedName,
                            nsIContent** aReturn)
{
  nsCOMPtr<nsINodeInfo> nodeInfo;
  nsresult rv = nsContentUtils::GetNodeInfoFromQName(aNamespaceURI,
                                                     aQualifiedName,
                                                     mNodeInfoManager,
                                                     nsIDOMNode::ELEMENT_NODE,
                                                     getter_AddRefs(nodeInfo));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_NewElement(aReturn, nodeInfo.forget(), NOT_FROM_PARSER);
}

NS_IMETHODIMP
nsDocument::CreateTextNode(const nsAString& aData, nsIDOMText** aReturn)
{
  *aReturn = nullptr;
  nsCOMPtr<nsIContent> content;
  nsresult rv = CreateTextNode(aData, getter_AddRefs(content));
  NS_ENSURE_SUCCESS(rv, rv);
  return CallQueryInterface(content, aReturn);
}

nsresult
nsDocument::CreateTextNode(const nsAString& aData, nsIContent** aReturn)
{
  nsresult rv = NS_NewTextNode(aReturn, mNodeInfoManager);
  if (NS_SUCCEEDED(rv)) {
    
    (*aReturn)->SetText(aData, false);
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
  *aReturn = nullptr;

  nsCOMPtr<nsIContent> comment;
  nsresult rv = NS_NewCommentNode(getter_AddRefs(comment), mNodeInfoManager);

  if (NS_SUCCEEDED(rv)) {
    
    comment->SetText(aData, false);

    rv = CallQueryInterface(comment, aReturn);
  }

  return rv;
}

NS_IMETHODIMP
nsDocument::CreateCDATASection(const nsAString& aData,
                               nsIDOMCDATASection** aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);
  *aReturn = nullptr;

  if (IsHTML()) {
    return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
  }

  if (FindInReadable(NS_LITERAL_STRING("]]>"), aData)) {
    return NS_ERROR_DOM_INVALID_CHARACTER_ERR;
  }

  nsCOMPtr<nsIContent> content;
  nsresult rv = NS_NewXMLCDATASection(getter_AddRefs(content),
                                      mNodeInfoManager);

  if (NS_SUCCEEDED(rv)) {
    
    content->SetText(aData, false);

    rv = CallQueryInterface(content, aReturn);
  }

  return rv;
}

NS_IMETHODIMP
nsDocument::CreateProcessingInstruction(const nsAString& aTarget,
                                        const nsAString& aData,
                                        nsIDOMProcessingInstruction** aReturn)
{
  *aReturn = nullptr;

  nsresult rv = nsContentUtils::CheckQName(aTarget, false);
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
  *aReturn = nullptr;

  WarnOnceAbout(eCreateAttribute);

  NS_ENSURE_TRUE(mNodeInfoManager, NS_ERROR_NOT_INITIALIZED);

  nsresult rv = nsContentUtils::CheckQName(aName, false);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsINodeInfo> nodeInfo;
  rv = mNodeInfoManager->GetNodeInfo(aName, nullptr, kNameSpaceID_None,
                                     nsIDOMNode::ATTRIBUTE_NODE,
                                     getter_AddRefs(nodeInfo));
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString value;
  nsCOMPtr<nsIDOMAttr> attribute =
    new nsDOMAttribute(nullptr, nodeInfo.forget(), value, false);
  attribute.forget(aReturn);
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::CreateAttributeNS(const nsAString & aNamespaceURI,
                              const nsAString & aQualifiedName,
                              nsIDOMAttr **aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = nullptr;

  WarnOnceAbout(eCreateAttributeNS);

  nsCOMPtr<nsINodeInfo> nodeInfo;
  nsresult rv = nsContentUtils::GetNodeInfoFromQName(aNamespaceURI,
                                                     aQualifiedName,
                                                     mNodeInfoManager,
                                                     nsIDOMNode::ATTRIBUTE_NODE,
                                                     getter_AddRefs(nodeInfo));
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString value;
  nsCOMPtr<nsIDOMAttr> attribute =
    new nsDOMAttribute(nullptr, nodeInfo.forget(), value, true);
  attribute.forget(aResult);
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetElementsByTagName(const nsAString& aTagname,
                                 nsIDOMNodeList** aReturn)
{
  nsRefPtr<nsContentList> list = GetElementsByTagName(aTagname);
  NS_ENSURE_TRUE(list, NS_ERROR_OUT_OF_MEMORY);

  
  *aReturn = list.forget().get();
  return NS_OK;
}

already_AddRefed<nsContentList>
nsDocument::GetElementsByTagNameNS(const nsAString& aNamespaceURI,
                                   const nsAString& aLocalName)
{
  PRInt32 nameSpaceId = kNameSpaceID_Wildcard;

  if (!aNamespaceURI.EqualsLiteral("*")) {
    nsresult rv =
      nsContentUtils::NameSpaceManager()->RegisterNameSpace(aNamespaceURI,
                                                            nameSpaceId);
    NS_ENSURE_SUCCESS(rv, nullptr);
  }

  NS_ASSERTION(nameSpaceId != kNameSpaceID_Unknown, "Unexpected namespace ID!");

  return NS_GetContentList(this, nameSpaceId, aLocalName);
}

NS_IMETHODIMP
nsDocument::GetElementsByTagNameNS(const nsAString& aNamespaceURI,
                                   const nsAString& aLocalName,
                                   nsIDOMNodeList** aReturn)
{
  nsRefPtr<nsContentList> list = GetElementsByTagNameNS(aNamespaceURI,
                                                        aLocalName);
  NS_ENSURE_TRUE(list, NS_ERROR_OUT_OF_MEMORY);

  
  *aReturn = list.forget().get();
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetAsync(bool *aAsync)
{
  NS_ERROR("nsDocument::GetAsync() should be overriden by subclass!");

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDocument::SetAsync(bool aAsync)
{
  NS_ERROR("nsDocument::SetAsync() should be overriden by subclass!");

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDocument::Load(const nsAString& aUrl, bool *aReturn)
{
  NS_ERROR("nsDocument::Load() should be overriden by subclass!");

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
    bool disabled;
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
  EnableStyleSheetsForSetInternal(aSheetSet, true);
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
    
    
    
    
    EnableStyleSheetsForSetInternal(aSheetSet, false);
  }

  return NS_OK;
}

void
nsDocument::EnableStyleSheetsForSetInternal(const nsAString& aSheetSet,
                                            bool aUpdateCSSLoader)
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
                       bool aDeep,
                       PRUint8 aArgc,
                       nsIDOMNode** aResult)
{
  if (aArgc == 0) {
    aDeep = true;
  }

  *aResult = nullptr;

  nsCOMPtr<nsINode> imported = do_QueryInterface(aImportedNode);
  NS_ENSURE_TRUE(imported, NS_ERROR_UNEXPECTED);

  nsresult rv = nsContentUtils::CheckSameOrigin(this, imported);
  NS_ENSURE_SUCCESS(rv, rv);

  switch (imported->NodeType()) {
    case nsIDOMNode::ATTRIBUTE_NODE:
    case nsIDOMNode::DOCUMENT_FRAGMENT_NODE:
    case nsIDOMNode::ELEMENT_NODE:
    case nsIDOMNode::PROCESSING_INSTRUCTION_NODE:
    case nsIDOMNode::TEXT_NODE:
    case nsIDOMNode::CDATA_SECTION_NODE:
    case nsIDOMNode::COMMENT_NODE:
    case nsIDOMNode::DOCUMENT_TYPE_NODE:
    {
      nsCOMPtr<nsIDOMNode> newNode;
      nsCOMArray<nsINode> nodesWithProperties;
      rv = nsNodeUtils::Clone(imported, aDeep, mNodeInfoManager,
                              nodesWithProperties, getter_AddRefs(newNode));
      NS_ENSURE_SUCCESS(rv, rv);

      nsIDocument *ownerDoc = imported->OwnerDoc();
      rv = nsNodeUtils::CallUserDataHandlers(nodesWithProperties, ownerDoc,
                                             nsIDOMUserDataHandler::NODE_IMPORTED,
                                             true);
      NS_ENSURE_SUCCESS(rv, rv);

      newNode.swap(*aResult);

      return NS_OK;
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
                          GetDocBaseURI());
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
  *aResult = nullptr;
  nsCOMPtr<nsIContent> content(do_QueryInterface(aNode));
  if (!content)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMElement> elt(do_QueryInterface(content->GetBindingParent()));
  NS_IF_ADDREF(*aResult = elt);
  return NS_OK;
}

static nsIContent*
GetElementByAttribute(nsIContent* aContent, nsIAtom* aAttrName,
                      const nsAString& aAttrValue, bool aUniversalMatch)
{
  if (aUniversalMatch ? aContent->HasAttr(kNameSpaceID_None, aAttrName) :
                        aContent->AttrValueIs(kNameSpaceID_None, aAttrName,
                                              aAttrValue, eCaseMatters)) {
    return aContent;
  }

  for (nsIContent* child = aContent->GetFirstChild();
       child;
       child = child->GetNextSibling()) {

    nsIContent* matchedContent =
      GetElementByAttribute(child, aAttrName, aAttrValue, aUniversalMatch);
    if (matchedContent)
      return matchedContent;
  }

  return nullptr;
}

nsIContent*
nsDocument::GetAnonymousElementByAttribute(nsIContent* aElement,
                                           nsIAtom* aAttrName,
                                           const nsAString& aAttrValue) const
{
  nsINodeList* nodeList = BindingManager()->GetAnonymousNodesFor(aElement);
  if (!nodeList)
    return nullptr;

  PRUint32 length = 0;
  nodeList->GetLength(&length);

  bool universalMatch = aAttrValue.EqualsLiteral("*");

  for (PRUint32 i = 0; i < length; ++i) {
    nsIContent* current = nodeList->GetNodeAt(i);
    nsIContent* matchedElm =
      GetElementByAttribute(current, aAttrName, aAttrValue, universalMatch);
    if (matchedElm)
      return matchedElm;
  }

  return nullptr;
}

NS_IMETHODIMP
nsDocument::GetAnonymousElementByAttribute(nsIDOMElement* aElement,
                                           const nsAString& aAttrName,
                                           const nsAString& aAttrValue,
                                           nsIDOMElement** aResult)
{
  *aResult = nullptr;

  nsCOMPtr<nsIAtom> attribute = do_GetAtom(aAttrName);
  nsCOMPtr<nsIContent> content(do_QueryInterface(aElement));

  nsIContent* matchedContent =
    GetAnonymousElementByAttribute(content, attribute, aAttrValue);
  return matchedContent ? CallQueryInterface(matchedContent, aResult) : NS_OK;
}


NS_IMETHODIMP
nsDocument::GetAnonymousNodes(nsIDOMElement* aElement,
                              nsIDOMNodeList** aResult)
{
  *aResult = nullptr;

  nsCOMPtr<nsIContent> content(do_QueryInterface(aElement));
  return BindingManager()->GetAnonymousNodesFor(content, aResult);
}

NS_IMETHODIMP
nsDocument::CreateRange(nsIDOMRange** aReturn)
{
  nsRefPtr<nsRange> range = new nsRange();
  nsresult rv = range->Set(this, 0, this, 0);
  NS_ENSURE_SUCCESS(rv, rv);

  range.forget(aReturn);
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::CreateNodeIterator(nsIDOMNode *aRoot,
                               PRUint32 aWhatToShow,
                               nsIDOMNodeFilter *aFilter,
                               PRUint8 aOptionalArgc,
                               nsIDOMNodeIterator **_retval)
{
  *_retval = nullptr;

  if (!aOptionalArgc) {
    aWhatToShow = nsIDOMNodeFilter::SHOW_ALL;
  }

  if (!aRoot) {
    return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
  }

  nsCOMPtr<nsINode> root = do_QueryInterface(aRoot);
  NS_ENSURE_TRUE(root, NS_ERROR_UNEXPECTED);

  nsresult rv = nsContentUtils::CheckSameOrigin(this, root);
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<nsNodeIterator> iterator = new nsNodeIterator(root, aWhatToShow,
                                                         aFilter);
  iterator.forget(_retval);
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::CreateTreeWalker(nsIDOMNode *aRoot,
                             PRUint32 aWhatToShow,
                             nsIDOMNodeFilter *aFilter,
                             PRUint8 aOptionalArgc,
                             nsIDOMTreeWalker **_retval)
{
  *_retval = nullptr;

  if (!aOptionalArgc) {
    aWhatToShow = nsIDOMNodeFilter::SHOW_ALL;
  }

  if (!aRoot)
    return NS_ERROR_DOM_NOT_SUPPORTED_ERR;

  nsresult rv = nsContentUtils::CheckSameOrigin(this, aRoot);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ENSURE_ARG_POINTER(_retval);

  nsCOMPtr<nsINode> root = do_QueryInterface(aRoot);
  NS_ENSURE_TRUE(root, NS_ERROR_DOM_NOT_SUPPORTED_ERR);

  nsTreeWalker* walker = new nsTreeWalker(root,
                                          aWhatToShow,
                                          aFilter);
  NS_ENSURE_TRUE(walker, NS_ERROR_OUT_OF_MEMORY);

  NS_ADDREF(*_retval = walker);

  return NS_OK;
}


NS_IMETHODIMP
nsDocument::GetDefaultView(nsIDOMWindow** aDefaultView)
{
  *aDefaultView = nullptr;
  nsCOMPtr<nsPIDOMWindow> win = GetWindow();
  win.forget(aDefaultView);
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetLocation(nsIDOMLocation **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = nullptr;

  nsCOMPtr<nsIDOMWindow> w = do_QueryInterface(mScriptGlobalObject);

  if (!w) {
    return NS_OK;
  }

  return w->GetLocation(_retval);
}

Element*
nsIDocument::GetHtmlElement()
{
  Element* rootElement = GetRootElement();
  if (rootElement && rootElement->IsHTML(nsGkAtoms::html))
    return rootElement;
  return nullptr;
}

Element*
nsIDocument::GetHtmlChildElement(nsIAtom* aTag)
{
  Element* html = GetHtmlElement();
  if (!html)
    return nullptr;

  
  
  for (nsIContent* child = html->GetFirstChild();
       child;
       child = child->GetNextSibling()) {
    if (child->IsHTML(aTag))
      return child->AsElement();
  }
  return nullptr;
}

nsIContent*
nsDocument::GetTitleContent(PRUint32 aNamespace)
{
  
  
  
  
  
  if (!mMayHaveTitleElement)
    return nullptr;

  nsRefPtr<nsContentList> list =
    NS_GetContentList(this, aNamespace, NS_LITERAL_STRING("title"));

  return list->Item(0, false);
}

void
nsDocument::GetTitleFromElement(PRUint32 aNamespace, nsAString& aTitle)
{
  nsIContent* title = GetTitleContent(aNamespace);
  if (!title)
    return;
  nsContentUtils::GetNodeTextContent(title, false, aTitle);
}

NS_IMETHODIMP
nsDocument::GetTitle(nsAString& aTitle)
{
  aTitle.Truncate();

  nsIContent *rootElement = GetRootElement();
  if (!rootElement)
    return NS_OK;

  nsAutoString tmp;

  switch (rootElement->GetNameSpaceID()) {
#ifdef MOZ_XUL
    case kNameSpaceID_XUL:
      rootElement->GetAttr(kNameSpaceID_None, nsGkAtoms::title, tmp);
      break;
#endif
    case kNameSpaceID_SVG:
      if (rootElement->Tag() == nsGkAtoms::svg) {
        GetTitleFromElement(kNameSpaceID_SVG, tmp);
        break;
      } 
    default:
      GetTitleFromElement(kNameSpaceID_XHTML, tmp);
      break;
  }

  tmp.CompressWhitespace();
  aTitle = tmp;
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::SetTitle(const nsAString& aTitle)
{
  Element *rootElement = GetRootElement();
  if (!rootElement)
    return NS_OK;

  switch (rootElement->GetNameSpaceID()) {
    case kNameSpaceID_SVG:
      return NS_OK; 
#ifdef MOZ_XUL
    case kNameSpaceID_XUL:
      return rootElement->SetAttr(kNameSpaceID_None, nsGkAtoms::title,
                                  aTitle, true);
#endif
  }

  
  
  mozAutoDocUpdate updateBatch(this, UPDATE_CONTENT_MODEL, true);

  nsIContent* title = GetTitleContent(kNameSpaceID_XHTML);
  if (!title) {
    Element *head = GetHeadElement();
    if (!head)
      return NS_OK;

    {
      nsCOMPtr<nsINodeInfo> titleInfo;
      titleInfo = mNodeInfoManager->GetNodeInfo(nsGkAtoms::title, nullptr,
                                                kNameSpaceID_XHTML,
                                                nsIDOMNode::ELEMENT_NODE);
      if (!titleInfo)
        return NS_OK;
      title = NS_NewHTMLTitleElement(titleInfo.forget());
      if (!title)
        return NS_OK;
    }

    head->AppendChildTo(title, true);
  }

  return nsContentUtils::SetNodeTextContent(title, aTitle, false);
}

void
nsDocument::NotifyPossibleTitleChange(bool aBoundTitleElement)
{
  NS_ASSERTION(!mInUnlinkOrDeletion || !aBoundTitleElement,
               "Setting a title while unlinking or destroying the element?");
  if (mInUnlinkOrDeletion) {
    return;
  }

  if (aBoundTitleElement) {
    mMayHaveTitleElement = true;
  }
  if (mPendingTitleChangeEvent.IsPending())
    return;

  nsRefPtr<nsRunnableMethod<nsDocument, void, false> > event =
    NS_NewNonOwningRunnableMethod(this,
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
  mHaveFiredTitleChange = true;

  nsAutoString title;
  GetTitle(title);

  nsCOMPtr<nsIPresShell> shell = GetShell();
  if (shell) {
    nsCOMPtr<nsISupports> container = shell->GetPresContext()->GetContainer();
    if (container) {
      nsCOMPtr<nsIBaseWindow> docShellWin = do_QueryInterface(container);
      if (docShellWin) {
        docShellWin->SetTitle(title.get());
      }
    }
  }

  
  nsContentUtils::DispatchChromeEvent(this, static_cast<nsIDocument*>(this),
                                      NS_LITERAL_STRING("DOMTitleChanged"),
                                      true, true);
}

NS_IMETHODIMP
nsDocument::GetBoxObjectFor(nsIDOMElement* aElement, nsIBoxObject** aResult)
{
  nsCOMPtr<nsIContent> content(do_QueryInterface(aElement));
  NS_ENSURE_TRUE(content, NS_ERROR_UNEXPECTED);

  nsIDocument* doc = content->OwnerDoc();
  NS_ENSURE_TRUE(doc == this, NS_ERROR_DOM_WRONG_DOCUMENT_ERR);

  if (!mHasWarnedAboutBoxObjects && !content->IsXUL()) {
    mHasWarnedAboutBoxObjects = true;
    nsContentUtils::ReportToConsole(nsIScriptError::warningFlag,
                                    "BoxObjects", this,
                                    nsContentUtils::eDOM_PROPERTIES,
                                    "UseOfGetBoxObjectForWarning");
  }

  *aResult = nullptr;

  if (!mBoxObjectTable) {
    mBoxObjectTable = new nsInterfaceHashtable<nsPtrHashKey<nsIContent>, nsPIBoxObject>;
    mBoxObjectTable->Init(12);
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
      NS_NewRunnableMethod(this, &nsDocument::MaybeInitializeFinalizeFrameLoaders);
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
      NS_NewRunnableMethod(this, &nsDocument::MaybeInitializeFinalizeFrameLoaders);
    NS_ENSURE_TRUE(mFrameLoaderRunner, NS_ERROR_OUT_OF_MEMORY);
    nsContentUtils::AddScriptRunner(mFrameLoaderRunner);
  }
  return NS_OK;
}

void
nsDocument::MaybeInitializeFinalizeFrameLoaders()
{
  if (mDelayFrameLoaderInitialization || mUpdateNestLevel != 0) {
    
    
    mFrameLoaderRunner = nullptr;
    return;
  }

  
  
  if (!nsContentUtils::IsSafeToRunScript()) {
    if (!mInDestructor && !mFrameLoaderRunner &&
        (mInitializableFrameLoaders.Length() ||
         mFinalizableFrameLoaders.Length())) {
      mFrameLoaderRunner =
        NS_NewRunnableMethod(this, &nsDocument::MaybeInitializeFinalizeFrameLoaders);
      nsContentUtils::AddScriptRunner(mFrameLoaderRunner);
    }
    return;
  }
  mFrameLoaderRunner = nullptr;

  
  
  
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

bool
nsDocument::FrameLoaderScheduledToBeFinalized(nsIDocShell* aShell)
{
  if (aShell) {
    PRUint32 length = mFinalizableFrameLoaders.Length();
    for (PRUint32 i = 0; i < length; ++i) {
      if (mFinalizableFrameLoaders[i]->GetExistingDocShell() == aShell) {
        return true;
      }
    }
  }
  return false;
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

nsSMILAnimationController*
nsDocument::GetAnimationController()
{
  
  
  if (mAnimationController)
    return mAnimationController;
  
  
  if (!NS_SMILEnabled() || mLoadedAsData || mLoadedAsInteractiveData)
    return nullptr;

  mAnimationController = new nsSMILAnimationController(this);
  
  
  
  nsIPresShell *shell = GetShell();
  if (mAnimationController && shell) {
    nsPresContext *context = shell->GetPresContext();
    if (context &&
        context->ImageAnimationMode() == imgIContainer::kDontAnimMode) {
      mAnimationController->Pause(nsSMILTimeContainer::PAUSE_USERPREF);
    }
  }

  
  
  
  if (!mIsShowing && !mIsBeingUsedAsImage) {
    mAnimationController->OnPageHide();
  }

  return mAnimationController;
}

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
        nsIPresShell *shell = GetShell();
        if (shell) {
          nsPresContext *context = shell->GetPresContext();
          NS_ENSURE_TRUE(context, NS_ERROR_UNEXPECTED);
          context->SetBidi(options, true);
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
  *aParentNode = nullptr;

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetParentElement(nsIDOMElement** aParentElement)
{
  *aParentElement = nullptr;
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetChildNodes(nsIDOMNodeList** aChildNodes)
{
  return nsINode::GetChildNodes(aChildNodes);
}

NS_IMETHODIMP
nsDocument::HasChildNodes(bool* aHasChildNodes)
{
  NS_ENSURE_ARG(aHasChildNodes);

  *aHasChildNodes = (mChildren.ChildCount() != 0);

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::HasAttributes(bool* aHasAttributes)
{
  NS_ENSURE_ARG(aHasAttributes);

  *aHasAttributes = false;

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
  *aPreviousSibling = nullptr;

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetNextSibling(nsIDOMNode** aNextSibling)
{
  *aNextSibling = nullptr;

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetAttributes(nsIDOMNamedNodeMap** aAttributes)
{
  *aAttributes = nullptr;

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
nsDocument::GetLocalName(nsAString& aLocalName)
{
  SetDOMStringToNull(aLocalName);

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::InsertBefore(nsIDOMNode* aNewChild, nsIDOMNode* aRefChild,
                         nsIDOMNode** aReturn)
{
  return ReplaceOrInsertBefore(false, aNewChild, aRefChild, aReturn);
}

NS_IMETHODIMP
nsDocument::ReplaceChild(nsIDOMNode* aNewChild, nsIDOMNode* aOldChild,
                         nsIDOMNode** aReturn)
{
  return ReplaceOrInsertBefore(true, aNewChild, aOldChild, aReturn);
}

NS_IMETHODIMP
nsDocument::RemoveChild(nsIDOMNode* aOldChild, nsIDOMNode** aReturn)
{
  return nsINode::RemoveChild(aOldChild, aReturn);
}

NS_IMETHODIMP
nsDocument::AppendChild(nsIDOMNode* aNewChild, nsIDOMNode** aReturn)
{
  return nsDocument::InsertBefore(aNewChild, nullptr, aReturn);
}

NS_IMETHODIMP
nsDocument::CloneNode(bool aDeep, PRUint8 aOptionalArgc, nsIDOMNode** aReturn)
{
  if (!aOptionalArgc) {
    aDeep = true;
  }
  
  return nsNodeUtils::CloneNodeImpl(this, aDeep, !mCreatingStaticClone, aReturn);
}

NS_IMETHODIMP
nsDocument::Normalize()
{
  return nsIDocument::Normalize();
}

NS_IMETHODIMP
nsDocument::IsSupported(const nsAString& aFeature, const nsAString& aVersion,
                        bool* aReturn)
{
  return nsGenericElement::InternalIsSupported(static_cast<nsIDOMDocument*>(this),
                                               aFeature, aVersion, aReturn);
}

NS_IMETHODIMP
nsDocument::GetDOMBaseURI(nsAString &aURI)
{
  return nsIDocument::GetDOMBaseURI(aURI);
}

NS_IMETHODIMP
nsDocument::GetTextContent(nsAString &aTextContent)
{
  SetDOMStringToNull(aTextContent);
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::IsEqualNode(nsIDOMNode* aOther, bool* aResult)
{
  return nsINode::IsEqualNode(aOther, aResult);
}

NS_IMETHODIMP
nsDocument::CompareDocumentPosition(nsIDOMNode *other,
                                   PRUint16 *aResult)
{
  return nsINode::CompareDocumentPosition(other, aResult);
}

NS_IMETHODIMP
nsDocument::SetTextContent(const nsAString & aTextContent)
{
  return nsINode::SetTextContent(aTextContent);
}

NS_IMETHODIMP
nsDocument::LookupPrefix(const nsAString & namespaceURI, nsAString & aResult)
{
  return nsINode::LookupPrefix(namespaceURI, aResult);
}

NS_IMETHODIMP
nsDocument::IsDefaultNamespace(const nsAString & namespaceURI,
                              bool *aResult)
{
  return nsINode::IsDefaultNamespace(namespaceURI, aResult);
}

NS_IMETHODIMP
nsDocument::LookupNamespaceURI(const nsAString & prefix,
                              nsAString & aResult)
{
  return nsINode::LookupNamespaceURI(prefix, aResult);
}

NS_IMETHODIMP
nsDocument::SetUserData(const nsAString & key,
                       nsIVariant *data, nsIDOMUserDataHandler *handler,
                       nsIVariant **aResult)
{
  return nsINode::SetUserData(key, data, handler, aResult);
}

NS_IMETHODIMP
nsDocument::GetUserData(const nsAString & key,
                        nsIVariant **aResult)
{
  return nsINode::GetUserData(key, aResult);
}

NS_IMETHODIMP
nsDocument::Contains(nsIDOMNode* aOther, bool* aReturn)
{
  return nsINode::Contains(aOther, aReturn);
}

NS_IMETHODIMP
nsDocument::GetInputEncoding(nsAString& aInputEncoding)
{
  WarnOnceAbout(eInputEncoding);
  if (mHaveInputEncoding) {
    return GetCharacterSet(aInputEncoding);
  }

  SetDOMStringToNull(aInputEncoding);
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetMozSyntheticDocument(bool *aSyntheticDocument)
{
  *aSyntheticDocument = mIsSyntheticDocument;
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

static void BlastSubtreeToPieces(nsINode *aNode);

PLDHashOperator
BlastFunc(nsAttrHashKey::KeyType aKey, nsDOMAttribute *aData, void* aUserArg)
{
  nsCOMPtr<nsIAttribute> *attr =
    static_cast<nsCOMPtr<nsIAttribute>*>(aUserArg);

  *attr = aData;

  NS_ASSERTION(attr->get(),
               "non-nsIAttribute somehow made it into the hashmap?!");

  return PL_DHASH_STOP;
}

static void
BlastSubtreeToPieces(nsINode *aNode)
{
  if (aNode->IsElement()) {
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
                             false);

        
        NS_ASSERTION(NS_SUCCEEDED(rv), "Uhoh, UnsetAttr shouldn't fail!");
      }
    }
  }

  PRUint32 count = aNode->GetChildCount();
  for (PRUint32 i = 0; i < count; ++i) {
    BlastSubtreeToPieces(aNode->GetFirstChild());
    aNode->RemoveChildAt(0, false);
  }
}


class nsUserDataCaller : public nsRunnable
{
public:
  nsUserDataCaller(nsCOMArray<nsINode>& aNodesWithProperties,
                   nsIDocument* aOwnerDoc)
    : mNodesWithProperties(aNodesWithProperties),
      mOwnerDoc(aOwnerDoc)
  {
  }

  NS_IMETHOD Run()
  {
    nsNodeUtils::CallUserDataHandlers(mNodesWithProperties, mOwnerDoc,
                                      nsIDOMUserDataHandler::NODE_ADOPTED,
                                      false);
    return NS_OK;
  }

private:
  nsCOMArray<nsINode> mNodesWithProperties;
  nsCOMPtr<nsIDocument> mOwnerDoc;
};











static nsresult
GetContextAndScope(nsIDocument* aOldDocument, nsIDocument* aNewDocument,
                   JSContext** aCx, JSObject** aNewScope)
{
  MOZ_ASSERT(aOldDocument);
  MOZ_ASSERT(aNewDocument);

  *aCx = nullptr;
  *aNewScope = nullptr;

  JSObject* newScope = aNewDocument->GetWrapper();
  JSObject* global;
  if (!newScope) {
    nsIScriptGlobalObject *newSGO = aNewDocument->GetScopeObject();
    if (!newSGO || !(global = newSGO->GetGlobalJSObject())) {
      return NS_OK;
    }
  }

  JSContext* cx = nsContentUtils::GetContextFromDocument(aOldDocument);
  if (!cx) {
    cx = nsContentUtils::GetContextFromDocument(aNewDocument);

    if (!cx) {
      
      
      

      nsIThreadJSContextStack* stack = nsContentUtils::ThreadJSContextStack();
      stack->Peek(&cx);

      if (!cx) {
        cx = stack->GetSafeJSContext();

        if (!cx) {
          
          NS_WARNING("No context reachable in GetContextAndScopes()!");

          return NS_ERROR_NOT_AVAILABLE;
        }
      }
    }
  }

  if (!newScope && cx) {
    JS::Value v;
    nsresult rv = nsContentUtils::WrapNative(cx, global, aNewDocument,
                                             aNewDocument, &v);
    NS_ENSURE_SUCCESS(rv, rv);

    newScope = JSVAL_TO_OBJECT(v);
  }

  *aCx = cx;
  *aNewScope = newScope;

  return NS_OK;
}

NS_IMETHODIMP
nsDocument::AdoptNode(nsIDOMNode *aAdoptedNode, nsIDOMNode **aResult)
{
  NS_ENSURE_ARG(aAdoptedNode);

  *aResult = nullptr;

  nsCOMPtr<nsINode> adoptedNode = do_QueryInterface(aAdoptedNode);
  NS_ENSURE_TRUE(adoptedNode, NS_ERROR_UNEXPECTED);

  nsresult rv = nsContentUtils::CheckSameOrigin(this, adoptedNode);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  {
    nsINode* parent = adoptedNode->GetNodeParent();
    if (parent) {
      nsContentUtils::MaybeFireNodeRemoved(adoptedNode, parent,
                                           adoptedNode->OwnerDoc());
    }
  }

  nsAutoScriptBlocker scriptBlocker;

  switch (adoptedNode->NodeType()) {
    case nsIDOMNode::ATTRIBUTE_NODE:
    {
      
      nsCOMPtr<nsIDOMAttr> adoptedAttr = do_QueryInterface(aAdoptedNode);
      NS_ASSERTION(adoptedAttr, "Attribute not implementing nsIDOMAttr");

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

      break;
    }
    case nsIDOMNode::DOCUMENT_FRAGMENT_NODE:
    case nsIDOMNode::ELEMENT_NODE:
    case nsIDOMNode::PROCESSING_INSTRUCTION_NODE:
    case nsIDOMNode::TEXT_NODE:
    case nsIDOMNode::CDATA_SECTION_NODE:
    case nsIDOMNode::COMMENT_NODE:
    case nsIDOMNode::DOCUMENT_TYPE_NODE:
    {
      
      
      
      
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

      
      nsCOMPtr<nsINode> parent = adoptedNode->GetNodeParent();
      if (parent) {
        parent->RemoveChildAt(parent->IndexOf(adoptedNode), true);
      }

      break;
    }
    case nsIDOMNode::DOCUMENT_NODE:
    {
      return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
    }
    default:
    {
      NS_WARNING("Don't know how to adopt this nodetype for adoptNode.");

      return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
    }
  }

  nsCOMPtr<nsIDocument> oldDocument = adoptedNode->OwnerDoc();
  bool sameDocument = oldDocument == this;

  JSContext *cx = nullptr;
  JSObject *newScope = nullptr;
  if (!sameDocument) {
    rv = GetContextAndScope(oldDocument, this, &cx, &newScope);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMArray<nsINode> nodesWithProperties;
  rv = nsNodeUtils::Adopt(adoptedNode, sameDocument ? nullptr : mNodeInfoManager,
                          cx, newScope, nodesWithProperties);
  if (NS_FAILED(rv)) {
    
    
    BlastSubtreeToPieces(adoptedNode);

    if (!sameDocument && oldDocument) {
      PRUint32 count = nodesWithProperties.Count();
      for (PRUint32 j = 0; j < oldDocument->GetPropertyTableCount(); ++j) {
        for (PRUint32 i = 0; i < count; ++i) {
          
          oldDocument->PropertyTable(j)->
            DeleteAllPropertiesFor(nodesWithProperties[i]);
        }
      }
    }

    return rv;
  }

  PRUint32 count = nodesWithProperties.Count();
  if (!sameDocument && oldDocument) {
    for (PRUint32 j = 0; j < oldDocument->GetPropertyTableCount(); ++j) {
      nsPropertyTable *oldTable = oldDocument->PropertyTable(j);
      nsPropertyTable *newTable = PropertyTable(j);
      for (PRUint32 i = 0; i < count; ++i) {
        if (NS_SUCCEEDED(rv)) {
          rv = oldTable->TransferOrDeleteAllPropertiesFor(nodesWithProperties[i],
                                                          newTable);
        } else {
          oldTable->DeleteAllPropertiesFor(nodesWithProperties[i]);
        }
      }
    }

    if (NS_FAILED(rv)) {
      
      BlastSubtreeToPieces(adoptedNode);

      return rv;
    }
  }

  if (nodesWithProperties.Count()) {
    nsContentUtils::AddScriptRunner(new nsUserDataCaller(nodesWithProperties,
                                                         this));
  }

  NS_ASSERTION(adoptedNode->OwnerDoc() == this,
               "Should still be in the document we just got adopted into");

  NS_ADDREF(*aResult = aAdoptedNode);
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetOwnerDocument(nsIDOMDocument** aOwnerDocument)
{
  return nsINode::GetOwnerDocument(aOwnerDocument);
}

nsEventListenerManager*
nsDocument::GetListenerManager(bool aCreateIfNotFound)
{
  if (!mListenerManager && aCreateIfNotFound) {
    mListenerManager =
      new nsEventListenerManager(static_cast<nsIDOMEventTarget*>(this));
    SetFlags(NODE_HAS_LISTENERMANAGER);
  }

  return mListenerManager;
}

nsresult
nsDocument::PreHandleEvent(nsEventChainPreVisitor& aVisitor)
{
  aVisitor.mCanHandle = true;
   
   
  aVisitor.mForceContentDispatch = true;

  
  if (aVisitor.mEvent->message != NS_LOAD) {
    nsGlobalWindow* window = static_cast<nsGlobalWindow*>(GetWindow());
    aVisitor.mParentTarget = static_cast<nsIDOMEventTarget*>(window);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::CreateEvent(const nsAString& aEventType, nsIDOMEvent** aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);
  *aReturn = nullptr;

  

  nsIPresShell *shell = GetShell();

  nsPresContext *presContext = nullptr;

  if (shell) {
    
    presContext = shell->GetPresContext();
  }

  
  return nsEventDispatcher::CreateEvent(presContext, nullptr,
                                        aEventType, aReturn);
}

void
nsDocument::FlushPendingNotifications(mozFlushType aType)
{
  
  
  
  
  
  
  if ((!IsHTML() ||
       (aType > Flush_ContentAndNotify && mPresShell &&
        !mPresShell->DidInitialReflow())) &&
      (mParser || mWeakSink)) {
    nsCOMPtr<nsIContentSink> sink;
    if (mParser) {
      sink = mParser->GetContentSink();
    } else {
      sink = do_QueryReferent(mWeakSink);
      if (!sink) {
        mWeakSink = nullptr;
      }
    }
    
    
    if (sink && (aType == Flush_Content || IsSafeToFlush())) {
      sink->FlushPendingNotifications(aType);
    }
  }

  

  if (aType <= Flush_ContentAndNotify) {
    
    return;
  }

  
  
  
  
  
  
  
  
  
  if (mParentDocument && IsSafeToFlush()) {
    mozFlushType parentType = aType;
    if (aType >= Flush_Style)
      parentType = NS_MAX(Flush_Layout, aType);
    mParentDocument->FlushPendingNotifications(parentType);
  }

  
  
  
  
  
  
  if (mNeedStyleFlush ||
      (mNeedLayoutFlush && aType >= Flush_InterruptibleLayout) ||
      aType >= Flush_Display ||
      mInFlush) {
    nsCOMPtr<nsIPresShell> shell = GetShell();
    if (shell) {
      mNeedStyleFlush = false;
      mNeedLayoutFlush = mNeedLayoutFlush && (aType < Flush_InterruptibleLayout);
      
      
      
      bool oldInFlush = mInFlush;
      mInFlush = true;
      shell->FlushPendingNotifications(aType);
      mInFlush = oldInFlush;
    }
  }
}

static bool
Flush(nsIDocument* aDocument, void* aData)
{
  const mozFlushType* type = static_cast<const mozFlushType*>(aData);
  aDocument->FlushPendingNotifications(*type);
  return true;
}

void
nsDocument::FlushExternalResources(mozFlushType aType)
{
  NS_ASSERTION(aType >= Flush_Style,
    "should only need to flush for style or higher in external resources");

  if (GetDisplayDocument()) {
    return;
  }
  EnumerateExternalResources(Flush, &aType);
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

bool
nsDocument::IsScriptEnabled()
{
  nsCOMPtr<nsIScriptSecurityManager> sm(do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID));
  NS_ENSURE_TRUE(sm, false);

  nsIScriptGlobalObject* globalObject = GetScriptGlobalObject();
  NS_ENSURE_TRUE(globalObject, false);

  nsIScriptContext *scriptContext = globalObject->GetContext();
  NS_ENSURE_TRUE(scriptContext, false);

  JSContext* cx = scriptContext->GetNativeContext();
  NS_ENSURE_TRUE(cx, false);

  bool enabled;
  nsresult rv = sm->CanExecuteScripts(cx, NodePrincipal(), &enabled);
  NS_ENSURE_SUCCESS(rv, false);
  return enabled;
}

nsRadioGroupStruct*
nsDocument::GetRadioGroup(const nsAString& aName)
{
  nsAutoString tmKey(aName);
  if (IsHTML()) {
    ToLowerCase(tmKey); 
  }

  nsRadioGroupStruct* radioGroup;
  if (mRadioGroups.Get(tmKey, &radioGroup)) {
    return radioGroup;
  }

  nsAutoPtr<nsRadioGroupStruct> newRadioGroup(new nsRadioGroupStruct());
  mRadioGroups.Put(tmKey, newRadioGroup);

  return newRadioGroup.forget();
}

NS_IMETHODIMP
nsDocument::SetCurrentRadioButton(const nsAString& aName,
                                  nsIDOMHTMLInputElement* aRadio)
{
  nsRadioGroupStruct* radioGroup = GetRadioGroup(aName);
  NS_ENSURE_TRUE(radioGroup, NS_OK);

  radioGroup->mSelectedRadioButton = aRadio;
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetCurrentRadioButton(const nsAString& aName,
                                  nsIDOMHTMLInputElement** aRadio)
{
  nsRadioGroupStruct* radioGroup = GetRadioGroup(aName);
  NS_ENSURE_TRUE(radioGroup, NS_OK);

  *aRadio = radioGroup->mSelectedRadioButton;
  NS_IF_ADDREF(*aRadio);
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetNextRadioButton(const nsAString& aName,
                               const bool aPrevious,
                               nsIDOMHTMLInputElement*  aFocusedRadio,
                               nsIDOMHTMLInputElement** aRadioOut)
{
  
  
  
  
  *aRadioOut = nullptr;

  nsRadioGroupStruct* radioGroup = GetRadioGroup(aName);
  NS_ENSURE_TRUE(radioGroup, NS_ERROR_FAILURE);

  
  
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
  bool disabled;
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
  nsRadioGroupStruct* radioGroup = GetRadioGroup(aName);
  NS_ENSURE_TRUE(radioGroup, NS_OK);

  radioGroup->mRadioButtons.AppendObject(aRadio);

  nsCOMPtr<nsIContent> element = do_QueryInterface(aRadio);
  NS_ASSERTION(element, "radio controls have to be content elements");
  if (element->HasAttr(kNameSpaceID_None, nsGkAtoms::required)) {
    radioGroup->mRequiredRadioCount++;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::RemoveFromRadioGroup(const nsAString& aName,
                                 nsIFormControl* aRadio)
{
  nsRadioGroupStruct* radioGroup = GetRadioGroup(aName);
  NS_ENSURE_TRUE(radioGroup, NS_OK);

  radioGroup->mRadioButtons.RemoveObject(aRadio);

  nsCOMPtr<nsIContent> element = do_QueryInterface(aRadio);
  NS_ASSERTION(element, "radio controls have to be content elements");
  if (element->HasAttr(kNameSpaceID_None, nsGkAtoms::required)) {
    NS_ASSERTION(radioGroup->mRequiredRadioCount != 0,
                 "mRequiredRadioCount about to wrap below 0!");
    radioGroup->mRequiredRadioCount--;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::WalkRadioGroup(const nsAString& aName,
                           nsIRadioVisitor* aVisitor,
                           bool aFlushContent)
{
  nsRadioGroupStruct* radioGroup = GetRadioGroup(aName);
  NS_ENSURE_TRUE(radioGroup, NS_OK);

  for (int i = 0; i < radioGroup->mRadioButtons.Count(); i++) {
    if (!aVisitor->Visit(radioGroup->mRadioButtons[i])) {
      return NS_OK;
    }
  }

  return NS_OK;
}

PRUint32
nsDocument::GetRequiredRadioCount(const nsAString& aName) const
{
  nsRadioGroupStruct* radioGroup = nullptr;
  
  
  
  nsAutoString tmKey(aName);
  if (IsHTML())
     ToLowerCase(tmKey); 
  mRadioGroups.Get(tmKey, &radioGroup);

  return radioGroup ? radioGroup->mRequiredRadioCount : 0;
}

void
nsDocument::RadioRequiredChanged(const nsAString& aName, nsIFormControl* aRadio)
{
  nsRadioGroupStruct* radioGroup = GetRadioGroup(aName);

  if (!radioGroup) {
    return;
  }

  nsCOMPtr<nsIContent> element = do_QueryInterface(aRadio);
  NS_ASSERTION(element, "radio controls have to be content elements");
  if (element->HasAttr(kNameSpaceID_None, nsGkAtoms::required)) {
    radioGroup->mRequiredRadioCount++;
  } else {
    NS_ASSERTION(radioGroup->mRequiredRadioCount != 0,
                 "mRequiredRadioCount about to wrap below 0!");
    radioGroup->mRequiredRadioCount--;
  }
}

bool
nsDocument::GetValueMissingState(const nsAString& aName) const
{
  nsRadioGroupStruct* radioGroup = nullptr;
  
  
  
  nsAutoString tmKey(aName);
  if (IsHTML())
     ToLowerCase(tmKey); 
  mRadioGroups.Get(tmKey, &radioGroup);

  return radioGroup && radioGroup->mGroupSuffersFromValueMissing;
}

void
nsDocument::SetValueMissingState(const nsAString& aName, bool aValue)
{
  nsRadioGroupStruct* radioGroup = GetRadioGroup(aName);

  if (!radioGroup) {
    return;
  }

  radioGroup->mGroupSuffersFromValueMissing = aValue;
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
      PRStatus st = PR_ParseTimeString(tmp.get(), true, &time);
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
      "x-content-security-policy",
      "x-content-security-policy-report-only",
      "x-frame-options",
      
      
      
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
      nsCAutoString contentDisp;
      rv = aChannel->GetContentDispositionHeader(contentDisp);
      if (NS_SUCCEEDED(rv)) {
        SetHeaderData(nsGkAtoms::headerContentDisposition,
                      NS_ConvertASCIItoUTF16(contentDisp));
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
nsDocument::CreateElem(const nsAString& aName, nsIAtom *aPrefix, PRInt32 aNamespaceID,
                       nsIContent **aResult)
{
#ifdef DEBUG
  nsAutoString qName;
  if (aPrefix) {
    aPrefix->ToString(qName);
    qName.Append(':');
  }
  qName.Append(aName);

  
  
  
  bool nsAware = aPrefix != nullptr || aNamespaceID != GetDefaultNamespaceID();
  NS_ASSERTION(NS_SUCCEEDED(nsContentUtils::CheckQName(qName, nsAware)),
               "Don't pass invalid prefixes to nsDocument::CreateElem, "
               "check caller.");
#endif

  *aResult = nullptr;
  
  nsCOMPtr<nsINodeInfo> nodeInfo;
  mNodeInfoManager->GetNodeInfo(aName, aPrefix, aNamespaceID,
                                nsIDOMNode::ELEMENT_NODE,
                                getter_AddRefs(nodeInfo));
  NS_ENSURE_TRUE(nodeInfo, NS_ERROR_OUT_OF_MEMORY);

  return NS_NewElement(aResult, nodeInfo.forget(), NOT_FROM_PARSER);
}

bool
nsDocument::IsSafeToFlush() const
{
  nsIPresShell* shell = GetShell();
  if (!shell)
    return true;

  return shell->IsSafeToFlush();
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

    bool resetValue = false;

    input->GetAttribute(NS_LITERAL_STRING("autocomplete"), value);
    if (value.LowerCaseEqualsLiteral("off")) {
      resetValue = true;
    } else {
      input->GetType(value);
      if (value.LowerCaseEqualsLiteral("password"))
        resetValue = true;
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
  bool next = subdoc ? args->callback(subdoc, args->data) : true;

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
  bool *canCacheArg = static_cast<bool*>(arg);

  nsIDocument *subdoc = entry->mSubDocument;

  
  bool canCache = subdoc ? subdoc->CanSavePresentation(nullptr) : false;
  if (!canCache) {
    *canCacheArg = false;
    return PL_DHASH_STOP;
  }

  return PL_DHASH_NEXT;
}

#ifdef DEBUG_bryner
#define DEBUG_PAGE_CACHE
#endif

bool
nsDocument::CanSavePresentation(nsIRequest *aNewRequest)
{
  if (EventHandlingSuppressed()) {
    return false;
  }

  nsPIDOMWindow* win = GetInnerWindow();
  if (win && win->TimeoutSuspendCount()) {
    return false;
  }

  
  nsCOMPtr<nsIDOMEventTarget> piTarget = do_QueryInterface(mScriptGlobalObject);
  if (piTarget) {
    nsEventListenerManager* manager =
      piTarget->GetListenerManager(false);
    if (manager && manager->HasUnloadListeners()) {
      return false;
    }
  }

  
  nsCOMPtr<nsILoadGroup> loadGroup = GetDocumentLoadGroup();
  if (loadGroup) {
    nsCOMPtr<nsISimpleEnumerator> requests;
    loadGroup->GetRequests(getter_AddRefs(requests));

    bool hasMore = false;

    
    
    
    nsCOMPtr<nsIChannel> baseChannel;
    nsCOMPtr<nsIMultiPartChannel> part(do_QueryInterface(aNewRequest));
    if (part) {
      part->GetBaseChannel(getter_AddRefs(baseChannel));
    }

    while (NS_SUCCEEDED(requests->HasMoreElements(&hasMore)) && hasMore) {
      nsCOMPtr<nsISupports> elem;
      requests->GetNext(getter_AddRefs(elem));

      nsCOMPtr<nsIRequest> request = do_QueryInterface(elem);
      if (request && request != aNewRequest && request != baseChannel) {
#ifdef DEBUG_PAGE_CACHE
        nsCAutoString requestName, docSpec;
        request->GetName(requestName);
        if (mDocumentURI)
          mDocumentURI->GetSpec(docSpec);

        printf("document %s has request %s\n",
               docSpec.get(), requestName.get());
#endif
        return false;
      }
    }
  }

  
  indexedDB::IndexedDatabaseManager* idbManager =
    indexedDB::IndexedDatabaseManager::Get();
  if (idbManager && idbManager->HasOpenTransactions(win)) {
    return false;
  }

  bool canCache = true;
  if (mSubDocuments)
    PL_DHashTableEnumerate(mSubDocuments, CanCacheSubDocument, &canCache);

  return canCache;
}

void
nsDocument::Destroy()
{
  
  
  if (mIsGoingAway)
    return;

  mIsGoingAway = true;

  RemovedFromDocShell();

  bool oldVal = mInUnlinkOrDeletion;
  mInUnlinkOrDeletion = true;
  PRUint32 i, count = mChildren.ChildCount();
  for (i = 0; i < count; ++i) {
    mChildren.ChildAt(i)->DestroyContent();
  }
  mInUnlinkOrDeletion = oldVal;

  mLayoutHistoryState = nullptr;

  
  
  
  mExternalResourceMap.Shutdown();

  
  
  nsContentUtils::ReleaseWrapper(static_cast<nsINode*>(this), this);
}

void
nsDocument::RemovedFromDocShell()
{
  if (mRemovedFromDocShell)
    return;

  mRemovedFromDocShell = true;
  EnumerateFreezableElements(NotifyActivityChanged, nullptr); 

  PRUint32 i, count = mChildren.ChildCount();
  for (i = 0; i < count; ++i) {
    mChildren.ChildAt(i)->SaveSubtreeState();
  }
}

already_AddRefed<nsILayoutHistoryState>
nsDocument::GetLayoutHistoryState() const
{
  nsILayoutHistoryState* state = nullptr;
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
nsDocument::EnsureOnloadBlocker()
{
  
  
  if (mOnloadBlockCount != 0 && mScriptGlobalObject) {
    nsCOMPtr<nsILoadGroup> loadGroup = GetDocumentLoadGroup();
    if (loadGroup) {
      
      nsCOMPtr<nsISimpleEnumerator> requests;
      loadGroup->GetRequests(getter_AddRefs(requests));

      bool hasMore = false;
      while (NS_SUCCEEDED(requests->HasMoreElements(&hasMore)) && hasMore) {
        nsCOMPtr<nsISupports> elem;
        requests->GetNext(getter_AddRefs(elem));
        nsCOMPtr<nsIRequest> request = do_QueryInterface(elem);
        if (request && request == mOnloadBlocker) {
          return;
        }
      }

      
      loadGroup->AddRequest(mOnloadBlocker, nullptr);
    }
  }
}

void
nsDocument::AsyncBlockOnload()
{
  while (mAsyncOnloadBlockCount) {
    --mAsyncOnloadBlockCount;
    BlockOnload();
  }
}

void
nsDocument::BlockOnload()
{
  if (mDisplayDocument) {
    mDisplayDocument->BlockOnload();
    return;
  }
  
  
  
  if (mOnloadBlockCount == 0 && mScriptGlobalObject) {
    if (!nsContentUtils::IsSafeToRunScript()) {
      
      
      ++mAsyncOnloadBlockCount;
      if (mAsyncOnloadBlockCount == 1) {
        bool success = nsContentUtils::AddScriptRunner(
          NS_NewRunnableMethod(this, &nsDocument::AsyncBlockOnload));

        
        
        
        if (!success) {
          NS_WARNING("Disaster! Onload blocking script runner failed to add - expect bad things!");
          mAsyncOnloadBlockCount = 0;
        }
      }
      return;
    }
    nsCOMPtr<nsILoadGroup> loadGroup = GetDocumentLoadGroup();
    if (loadGroup) {
      loadGroup->AddRequest(mOnloadBlocker, nullptr);
    }
  }
  ++mOnloadBlockCount;      
}

void
nsDocument::UnblockOnload(bool aFireSync)
{
  if (mDisplayDocument) {
    mDisplayDocument->UnblockOnload(aFireSync);
    return;
  }

  if (mOnloadBlockCount == 0 && mAsyncOnloadBlockCount == 0) {
    NS_NOTREACHED("More UnblockOnload() calls than BlockOnload() calls; dropping call");
    return;
  }

  --mOnloadBlockCount;

  
  
  if (mOnloadBlockCount == 0 && mScriptGlobalObject) {
    if (aFireSync && mAsyncOnloadBlockCount == 0) {
      
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

  if (mAsyncOnloadBlockCount != 0) {
    
    PostUnblockOnloadEvent();
  }

  
  
  if (mScriptGlobalObject) {
    nsCOMPtr<nsILoadGroup> loadGroup = GetDocumentLoadGroup();
    if (loadGroup) {
      loadGroup->RemoveRequest(mOnloadBlocker, nullptr, NS_OK);
    }
  }
}

nsIContent*
nsDocument::GetContentInThisDocument(nsIFrame* aFrame) const
{
  for (nsIFrame* f = aFrame; f;
       f = nsLayoutUtils::GetParentOrPlaceholderForCrossDoc(f)) {
    nsIContent* content = f->GetContent();
    if (!content || content->IsInAnonymousSubtree())
      continue;

    if (content->OwnerDoc() == this) {
      return content;
    }
    
    
    
    f = f->PresContext()->GetPresShell()->GetRootFrame();
  }

  return nullptr;
}

void
nsDocument::DispatchPageTransition(nsIDOMEventTarget* aDispatchTarget,
                                   const nsAString& aType,
                                   bool aPersisted)
{
  if (aDispatchTarget) {
    nsCOMPtr<nsIDOMEvent> event;
    CreateEvent(NS_LITERAL_STRING("pagetransition"), getter_AddRefs(event));
    nsCOMPtr<nsIDOMPageTransitionEvent> ptEvent = do_QueryInterface(event);
    if (ptEvent && NS_SUCCEEDED(ptEvent->InitPageTransitionEvent(aType, true,
                                                                 true,
                                                                 aPersisted))) {
      event->SetTrusted(true);
      event->SetTarget(this);
      nsEventDispatcher::DispatchDOMEvent(aDispatchTarget, nullptr, event,
                                          nullptr, nullptr);
    }
  }
}

static bool
NotifyPageShow(nsIDocument* aDocument, void* aData)
{
  const bool* aPersistedPtr = static_cast<const bool*>(aData);
  aDocument->OnPageShow(*aPersistedPtr, nullptr);
  return true;
}

void
nsDocument::OnPageShow(bool aPersisted,
                       nsIDOMEventTarget* aDispatchStartTarget)
{
  mVisible = true;

  EnumerateFreezableElements(NotifyActivityChanged, nullptr);
  EnumerateExternalResources(NotifyPageShow, &aPersisted);

  Element* root = GetRootElement();
  if (aPersisted && root) {
    
    nsRefPtr<nsContentList> links = NS_GetContentList(root,
                                                      kNameSpaceID_Unknown,
                                                      NS_LITERAL_STRING("link"));

    PRUint32 linkCount = links->Length(true);
    for (PRUint32 i = 0; i < linkCount; ++i) {
      nsCOMPtr<nsILink> link = do_QueryInterface(links->Item(i, false));
      if (link) {
        link->LinkAdded();
      }
    }
  }

  
  if (!aDispatchStartTarget) {
    
    
    mIsShowing = true;
  }
 
  if (mAnimationController) {
    mAnimationController->OnPageShow();
  }

  if (aPersisted) {
    SetImagesNeedAnimating(true);
  }

  UpdateVisibilityState();

  nsCOMPtr<nsIDOMEventTarget> target = aDispatchStartTarget;
  if (!target) {
    target = do_QueryInterface(GetWindow());
  }
  DispatchPageTransition(target, NS_LITERAL_STRING("pageshow"), aPersisted);
}

static bool
NotifyPageHide(nsIDocument* aDocument, void* aData)
{
  const bool* aPersistedPtr = static_cast<const bool*>(aData);
  aDocument->OnPageHide(*aPersistedPtr, nullptr);
  return true;
}

void
nsDocument::OnPageHide(bool aPersisted,
                       nsIDOMEventTarget* aDispatchStartTarget)
{
  
  
  Element* root = GetRootElement();
  if (aPersisted && root) {
    nsRefPtr<nsContentList> links = NS_GetContentList(root,
                                                      kNameSpaceID_Unknown,
                                                      NS_LITERAL_STRING("link"));

    PRUint32 linkCount = links->Length(true);
    for (PRUint32 i = 0; i < linkCount; ++i) {
      nsCOMPtr<nsILink> link = do_QueryInterface(links->Item(i, false));
      if (link) {
        link->LinkRemoved();
      }
    }
  }

  
  if (!aDispatchStartTarget) {
    
    
    mIsShowing = false;
  }

  if (mAnimationController) {
    mAnimationController->OnPageHide();
  }
  
  if (aPersisted) {
    SetImagesNeedAnimating(false);
  }

  
  nsCOMPtr<nsIDOMEventTarget> target = aDispatchStartTarget;
  if (!target) {
    target = do_QueryInterface(GetWindow());
  }
  DispatchPageTransition(target, NS_LITERAL_STRING("pagehide"), aPersisted);

  mVisible = false;

  UpdateVisibilityState();
  
  EnumerateExternalResources(NotifyPageHide, &aPersisted);
  EnumerateFreezableElements(NotifyActivityChanged, nullptr);

  if (IsFullScreenDoc()) {
    
    
    
    
    
    
    
    
    CleanupFullscreenState();

    
    nsIDocument::ExitFullScreen(false);
  }
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

      nsINode* commonAncestor = nullptr;
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
      nsMutationEvent mutation(true, NS_MUTATION_SUBTREEMODIFIED);
      (new nsAsyncDOMEvent(realTargets[k], mutation))->RunDOMEventWhenSafe();
    }
  }
}

void
nsDocument::AddStyleRelevantLink(Link* aLink)
{
  NS_ASSERTION(aLink, "Passing in a null link.  Expect crashes RSN!");
#ifdef DEBUG
  nsPtrHashKey<Link>* entry = mStyledLinks.GetEntry(aLink);
  NS_ASSERTION(!entry, "Document already knows about this Link!");
  mStyledLinksCleared = false;
#endif
  (void)mStyledLinks.PutEntry(aLink);
}

void
nsDocument::ForgetLink(Link* aLink)
{
  NS_ASSERTION(aLink, "Passing in a null link.  Expect crashes RSN!");
#ifdef DEBUG
  nsPtrHashKey<Link>* entry = mStyledLinks.GetEntry(aLink);
  NS_ASSERTION(entry || mStyledLinksCleared,
               "Document knows nothing about this Link!");
#endif
  (void)mStyledLinks.RemoveEntry(aLink);
}

void
nsDocument::DestroyElementMaps()
{
#ifdef DEBUG
  mStyledLinksCleared = true;
#endif
  mStyledLinks.Clear();
  mIdentifierMap.Clear();
}

static
PLDHashOperator
EnumerateStyledLinks(nsPtrHashKey<Link>* aEntry, void* aArray)
{
  LinkArray* array = static_cast<LinkArray*>(aArray);
  (void)array->AppendElement(aEntry->GetKey());
  return PL_DHASH_NEXT;
}

void
nsDocument::RefreshLinkHrefs()
{
  
  
  
  LinkArray linksToNotify(mStyledLinks.Count());
  (void)mStyledLinks.EnumerateEntries(EnumerateStyledLinks, &linksToNotify);

  
  nsAutoScriptBlocker scriptBlocker;
  for (LinkArray::size_type i = 0; i < linksToNotify.Length(); i++) {
    linksToNotify[i]->ResetLinkState(true);
  }
}

nsresult
nsDocument::CloneDocHelper(nsDocument* clone) const
{
  clone->mIsStaticDocument = mCreatingStaticClone;

  
  nsresult rv = clone->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  
  clone->nsDocument::SetDocumentURI(nsIDocument::GetDocumentURI());
  
  clone->SetPrincipal(NodePrincipal());
  clone->mDocumentBaseURI = mDocumentBaseURI;

  if (mCreatingStaticClone) {
    nsCOMPtr<nsIChannel> channel = GetChannel();
    nsCOMPtr<nsILoadGroup> loadGroup = GetDocumentLoadGroup();
    if (channel && loadGroup) {
      clone->Reset(channel, loadGroup);
    } else {
      nsIURI* uri = static_cast<const nsIDocument*>(this)->GetDocumentURI();
      if (uri) {
        clone->ResetToURI(uri, loadGroup, NodePrincipal());
      }
    }
    nsCOMPtr<nsISupports> container = GetContainer();
    clone->SetContainer(container);
  }

  
  bool hasHadScriptObject = true;
  nsIScriptGlobalObject* scriptObject =
    GetScriptHandlingObject(hasHadScriptObject);
  NS_ENSURE_STATE(scriptObject || !hasHadScriptObject);
  clone->SetScriptHandlingObject(scriptObject);

  
  clone->SetLoadedAsData(true);

  

  
  clone->mCharacterSet = mCharacterSet;
  clone->mCharacterSetSource = mCharacterSetSource;
  clone->mCompatMode = mCompatMode;
  clone->mBidiOptions = mBidiOptions;
  clone->mContentLanguage = mContentLanguage;
  clone->SetContentTypeInternal(GetContentTypeInternal());
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
  if (rs == READYSTATE_UNINITIALIZED) {
    
    
    
    return;
  }
  if (mTiming) {
    switch (rs) {
      case READYSTATE_LOADING:
        mTiming->NotifyDOMLoading(nsIDocument::GetDocumentURI());
        break;
      case READYSTATE_INTERACTIVE:
        mTiming->NotifyDOMInteractive(nsIDocument::GetDocumentURI());
        break;
      case READYSTATE_COMPLETE:
        mTiming->NotifyDOMComplete(nsIDocument::GetDocumentURI());
        break;
      default:
        NS_WARNING("Unexpected ReadyState value");
        break;
    }
  }
  
  if (READYSTATE_LOADING == rs) {
    mLoadingTimeStamp = mozilla::TimeStamp::Now();
  }

  nsRefPtr<nsAsyncDOMEvent> plevent =
    new nsAsyncDOMEvent(this, NS_LITERAL_STRING("readystatechange"), false, false); 
  if (plevent) {
    plevent->RunDOMEventWhenSafe();
  }
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

static bool
SuppressEventHandlingInDocument(nsIDocument* aDocument, void* aData)
{
  aDocument->SuppressEventHandling(*static_cast<PRUint32*>(aData));
  return true;
}

void
nsDocument::SuppressEventHandling(PRUint32 aIncrease)
{
  if (mEventsSuppressed == 0 && aIncrease != 0 && mPresShell &&
      mScriptGlobalObject) {
    RevokeAnimationFrameNotifications();
  }
  mEventsSuppressed += aIncrease;
  EnumerateSubDocuments(SuppressEventHandlingInDocument, &aIncrease);
}

static void
FireOrClearDelayedEvents(nsTArray<nsCOMPtr<nsIDocument> >& aDocuments,
                         bool aFireEvents)
{
  nsIFocusManager* fm = nsFocusManager::GetFocusManager();
  if (!fm)
    return;

  for (PRUint32 i = 0; i < aDocuments.Length(); ++i) {
    if (!aDocuments[i]->EventHandlingSuppressed()) {
      fm->FireDelayedEvents(aDocuments[i]);
      nsCOMPtr<nsIPresShell> shell = aDocuments[i]->GetShell();
      if (shell) {
        shell->FireOrClearDelayedEvents(aFireEvents);
      }
    }
  }
}

void
nsDocument::MaybePreLoadImage(nsIURI* uri, const nsAString &aCrossOriginAttr)
{
  
  
  
  PRInt16 blockingStatus;
  if (nsContentUtils::IsImageInCache(uri) ||
      !nsContentUtils::CanLoadImage(uri, static_cast<nsIDocument *>(this),
                                    this, NodePrincipal(), &blockingStatus)) {
    return;
  }

  nsLoadFlags loadFlags = nsIRequest::LOAD_NORMAL;
  switch (nsGenericElement::StringToCORSMode(aCrossOriginAttr)) {
  case CORS_NONE:
    
    break;
  case CORS_ANONYMOUS:
    loadFlags |= imgILoader::LOAD_CORS_ANONYMOUS;
    break;
  case CORS_USE_CREDENTIALS:
    loadFlags |= imgILoader::LOAD_CORS_USE_CREDENTIALS;
    break;
  default:
    
    MOZ_NOT_REACHED("Unknown CORS mode!");
  }

  
  nsCOMPtr<imgIRequest> request;
  nsresult rv =
    nsContentUtils::LoadImage(uri,
                              this,
                              NodePrincipal(),
                              mDocumentURI, 
                              nullptr,       
                              loadFlags,
                              getter_AddRefs(request));

  
  
  
  if (NS_SUCCEEDED(rv)) {
    mPreloadingImages.AppendObject(request);
  }
}

nsEventStates
nsDocument::GetDocumentState()
{
  if (!mGotDocumentState.HasState(NS_DOCUMENT_STATE_RTL_LOCALE)) {
    if (IsDocumentRightToLeft()) {
      mDocumentState |= NS_DOCUMENT_STATE_RTL_LOCALE;
    }
    mGotDocumentState |= NS_DOCUMENT_STATE_RTL_LOCALE;
  }
  if (!mGotDocumentState.HasState(NS_DOCUMENT_STATE_WINDOW_INACTIVE)) {
    nsIPresShell* shell = GetShell();
    if (shell && shell->GetPresContext() &&
        shell->GetPresContext()->IsTopLevelWindowInactive()) {
      mDocumentState |= NS_DOCUMENT_STATE_WINDOW_INACTIVE;
    }
    mGotDocumentState |= NS_DOCUMENT_STATE_WINDOW_INACTIVE;
  }
  return mDocumentState;
}

namespace {





class StubCSSLoaderObserver MOZ_FINAL : public nsICSSLoaderObserver {
public:
  NS_IMETHOD
  StyleSheetLoaded(nsCSSStyleSheet*, bool, nsresult)
  {
    return NS_OK;
  }
  NS_DECL_ISUPPORTS
};
NS_IMPL_ISUPPORTS1(StubCSSLoaderObserver, nsICSSLoaderObserver)

}

void
nsDocument::PreloadStyle(nsIURI* uri, const nsAString& charset)
{
  
  nsCOMPtr<nsICSSLoaderObserver> obs = new StubCSSLoaderObserver();

  
  CSSLoader()->LoadSheet(uri, NodePrincipal(),
                         NS_LossyConvertUTF16toASCII(charset),
                         obs);
}

nsresult
nsDocument::LoadChromeSheetSync(nsIURI* uri, bool isAgentSheet,
                                nsCSSStyleSheet** sheet)
{
  return CSSLoader()->LoadSheetSync(uri, isAgentSheet, isAgentSheet, sheet);
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
    FireOrClearDelayedEvents(mDocuments, true);
    return NS_OK;
  }

private:
  nsTArray<nsCOMPtr<nsIDocument> > mDocuments;
};

static bool
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
  return true;
}

void
nsDocument::UnsuppressEventHandlingAndFireEvents(bool aFireEvents)
{
  nsTArray<nsCOMPtr<nsIDocument> > documents;
  GetAndUnsuppressSubDocuments(this, &documents);

  if (aFireEvents) {
    NS_DispatchToCurrentThread(new nsDelayedEventDispatcher(documents));
  } else {
    FireOrClearDelayedEvents(documents, false);
  }
}

nsISupports*
nsDocument::GetCurrentContentSink()
{
  return mParser ? mParser->GetContentSink() : nullptr;
}

void
nsDocument::RegisterFileDataUri(const nsACString& aUri)
{
  mFileDataUris.AppendElement(aUri);
}

void
nsDocument::UnregisterFileDataUri(const nsACString& aUri)
{
  mFileDataUris.RemoveElement(aUri);
}

void
nsDocument::SetScrollToRef(nsIURI *aDocumentURI)
{
  if (!aDocumentURI) {
    return;
  }

  nsCAutoString ref;

  
  
  
  

  aDocumentURI->GetSpec(ref);

  nsReadingIterator<char> start, end;

  ref.BeginReading(start);
  ref.EndReading(end);

  if (FindCharInReadable('#', start, end)) {
    ++start; 

    mScrollToRef = Substring(start, end);
  }
}

void
nsDocument::ScrollToRef()
{
  if (mScrolledToRefAlready) {
    return;
  }

  if (mScrollToRef.IsEmpty()) {
    return;
  }

  char* tmpstr = ToNewCString(mScrollToRef);
  if (!tmpstr) {
    return;
  }

  nsUnescape(tmpstr);
  nsCAutoString unescapedRef;
  unescapedRef.Assign(tmpstr);
  nsMemory::Free(tmpstr);

  nsresult rv = NS_ERROR_FAILURE;
  
  
  NS_ConvertUTF8toUTF16 ref(unescapedRef);

  nsCOMPtr<nsIPresShell> shell = GetShell();
  if (shell) {
    
    if (!ref.IsEmpty()) {
      
      rv = shell->GoToAnchor(ref, mChangeScrollPosWhenScrollingToRef);
    } else {
      rv = NS_ERROR_FAILURE;
    }

    
    

    if (NS_FAILED(rv)) {
      const nsACString &docCharset = GetDocumentCharacterSet();

      rv = nsContentUtils::ConvertStringFromCharset(docCharset, unescapedRef, ref);

      if (NS_SUCCEEDED(rv) && !ref.IsEmpty()) {
        rv = shell->GoToAnchor(ref, mChangeScrollPosWhenScrollingToRef);
      }
    }
    if (NS_SUCCEEDED(rv)) {
      mScrolledToRefAlready = true;
    }
  }
}

void
nsDocument::ResetScrolledToRefAlready()
{
  mScrolledToRefAlready = false;
}

void
nsDocument::SetChangeScrollPosWhenScrollingToRef(bool aValue)
{
  mChangeScrollPosWhenScrollingToRef = aValue;
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

bool
nsIDocument::UnregisterFreezableElement(nsIContent* aContent)
{
  if (!mFreezableElements)
    return false;
  if (!mFreezableElements->GetEntry(aContent))
    return false;
  mFreezableElements->RemoveEntry(aContent);
  return true;
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

void
nsIDocument::RegisterPendingLinkUpdate(Link* aLink)
{
  mLinksToUpdate.PutEntry(aLink);
  mHasLinksToUpdate = true;
}

void
nsIDocument::UnregisterPendingLinkUpdate(Link* aLink)
{
  if (!mHasLinksToUpdate)
    return;
    
  mLinksToUpdate.RemoveEntry(aLink);
}
  
static PLDHashOperator
EnumeratePendingLinkUpdates(nsPtrHashKey<Link>* aEntry, void* aData)
{
  aEntry->GetKey()->GetElement()->UpdateLinkState(aEntry->GetKey()->LinkState());
  return PL_DHASH_NEXT;
}

void
nsIDocument::FlushPendingLinkUpdates() 
{
  if (!mHasLinksToUpdate)
    return;
    
  nsAutoScriptBlocker scriptBlocker;
  mLinksToUpdate.EnumerateEntries(EnumeratePendingLinkUpdates, nullptr);
  mLinksToUpdate.Clear();
  mHasLinksToUpdate = false;
}

already_AddRefed<nsIDocument>
nsIDocument::CreateStaticClone(nsISupports* aCloneContainer)
{
  nsCOMPtr<nsIDOMDocument> domDoc = do_QueryInterface(this);
  NS_ENSURE_TRUE(domDoc, nullptr);
  mCreatingStaticClone = true;

  
  nsCOMPtr<nsISupports> originalContainer = GetContainer();
  SetContainer(aCloneContainer);
  nsCOMPtr<nsIDOMNode> clonedNode;
  nsresult rv = domDoc->CloneNode(true, 1, getter_AddRefs(clonedNode));
  SetContainer(originalContainer);

  nsCOMPtr<nsIDocument> clonedDoc;
  if (NS_SUCCEEDED(rv)) {
    clonedDoc = do_QueryInterface(clonedNode);
    if (clonedDoc) {
      if (IsStaticDocument()) {
        clonedDoc->mOriginalDocument = mOriginalDocument;
      } else {
        clonedDoc->mOriginalDocument = this;
      }
      PRInt32 sheetsCount = GetNumberOfStyleSheets();
      for (PRInt32 i = 0; i < sheetsCount; ++i) {
        nsRefPtr<nsCSSStyleSheet> sheet = do_QueryObject(GetStyleSheetAt(i));
        if (sheet) {
          if (sheet->IsApplicable()) {
            nsRefPtr<nsCSSStyleSheet> clonedSheet =
              sheet->Clone(nullptr, nullptr, clonedDoc, nullptr);
            NS_WARN_IF_FALSE(clonedSheet, "Cloning a stylesheet didn't work!");
            if (clonedSheet) {
              clonedDoc->AddStyleSheet(clonedSheet);
            }
          }
        }
      }

      sheetsCount = GetNumberOfCatalogStyleSheets();
      for (PRInt32 i = 0; i < sheetsCount; ++i) {
        nsRefPtr<nsCSSStyleSheet> sheet =
          do_QueryObject(GetCatalogStyleSheetAt(i));
        if (sheet) {
          if (sheet->IsApplicable()) {
            nsRefPtr<nsCSSStyleSheet> clonedSheet =
              sheet->Clone(nullptr, nullptr, clonedDoc, nullptr);
            NS_WARN_IF_FALSE(clonedSheet, "Cloning a stylesheet didn't work!");
            if (clonedSheet) {
              clonedDoc->AddCatalogStyleSheet(clonedSheet);
            }
          }
        }
      }
    }
  }
  mCreatingStaticClone = false;
  return clonedDoc.forget();
}

nsresult
nsIDocument::ScheduleFrameRequestCallback(nsIFrameRequestCallback* aCallback,
                                          PRInt32 *aHandle)
{
  if (mFrameRequestCallbackCounter == PR_INT32_MAX) {
    
    return NS_ERROR_NOT_AVAILABLE;
  }
  PRInt32 newHandle = ++mFrameRequestCallbackCounter;

  bool alreadyRegistered = !mFrameRequestCallbacks.IsEmpty();
  DebugOnly<FrameRequest*> request =
    mFrameRequestCallbacks.AppendElement(FrameRequest(aCallback, newHandle));
  NS_ASSERTION(request, "This is supposed to be infallible!");
  if (!alreadyRegistered && mPresShell && IsEventHandlingEnabled()) {
    mPresShell->GetPresContext()->RefreshDriver()->
      ScheduleFrameRequestCallbacks(this);
  }

  *aHandle = newHandle;
  return NS_OK;
}

void
nsIDocument::CancelFrameRequestCallback(PRInt32 aHandle)
{
  
  if (mFrameRequestCallbacks.RemoveElementSorted(aHandle) &&
      mFrameRequestCallbacks.IsEmpty() &&
      mPresShell && IsEventHandlingEnabled()) {
    mPresShell->GetPresContext()->RefreshDriver()->
      RevokeFrameRequestCallbacks(this);
  }
}

nsresult
nsDocument::GetStateObject(nsIVariant** aState)
{
  
  
  
  
  

  nsCOMPtr<nsIVariant> stateObj;
  if (!mStateObjectCached && mStateObjectContainer) {
    JSContext *cx = nsContentUtils::GetContextFromDocument(this);
    mStateObjectContainer->
      DeserializeToVariant(cx, getter_AddRefs(mStateObjectCached));
  }

  NS_IF_ADDREF(*aState = mStateObjectCached);
  
  return NS_OK;
}

nsDOMNavigationTiming*
nsDocument::GetNavigationTiming() const
{
  return mTiming;
}

nsresult
nsDocument::SetNavigationTiming(nsDOMNavigationTiming* aTiming)
{
  mTiming = aTiming;
  if (!mLoadingTimeStamp.IsNull() && mTiming) {
    mTiming->SetDOMLoadingTimeStamp(nsIDocument::GetDocumentURI(), mLoadingTimeStamp);
  }
  return NS_OK;
}

Element*
nsDocument::FindImageMap(const nsAString& aUseMapValue)
{
  if (aUseMapValue.IsEmpty()) {
    return nullptr;
  }

  nsAString::const_iterator start, end;
  aUseMapValue.BeginReading(start);
  aUseMapValue.EndReading(end);

  PRInt32 hash = aUseMapValue.FindChar('#');
  if (hash < 0) {
    return nullptr;
  }
  
  start.advance(hash + 1);

  if (start == end) {
    return nullptr; 
  }

  const nsAString& mapName = Substring(start, end);

  if (!mImageMaps) {
    mImageMaps = new nsContentList(this, kNameSpaceID_XHTML, nsGkAtoms::map, nsGkAtoms::map);
  }

  PRUint32 i, n = mImageMaps->Length(true);
  for (i = 0; i < n; ++i) {
    nsIContent* map = mImageMaps->GetNodeAt(i);
    if (map->AttrValueIs(kNameSpaceID_None, nsGkAtoms::id, mapName,
                         eCaseMatters) ||
        map->AttrValueIs(kNameSpaceID_None, nsGkAtoms::name, mapName,
                         eIgnoreCase)) {
      return map->AsElement();
    }
  }

  return nullptr;
}

#define DEPRECATED_OPERATION(_op) #_op "Warning",
static const char* kWarnings[] = {
#include "nsDeprecatedOperationList.h"
  nullptr
};
#undef DEPRECATED_OPERATION

void
nsIDocument::WarnOnceAbout(DeprecatedOperations aOperation,
                           bool asError )
{
  PR_STATIC_ASSERT(eDeprecatedOperationCount <= 64);
  if (mWarnedAbout & (1ull << aOperation)) {
    return;
  }
  mWarnedAbout |= (1ull << aOperation);
  uint32_t flags = asError ? nsIScriptError::errorFlag
                           : nsIScriptError::warningFlag;
  nsContentUtils::ReportToConsole(flags,
                                  "DOM Core", this,
                                  nsContentUtils::eDOM_PROPERTIES,
                                  kWarnings[aOperation]);
}

nsresult
nsDocument::AddImage(imgIRequest* aImage)
{
  NS_ENSURE_ARG_POINTER(aImage);

  
  PRUint32 oldCount = 0;
  mImageTracker.Get(aImage, &oldCount);

  
  mImageTracker.Put(aImage, oldCount + 1);

  nsresult rv = NS_OK;

  
  
  if (oldCount == 0 && mLockingImages) {
    rv = aImage->LockImage();
    if (NS_SUCCEEDED(rv))
      rv = aImage->RequestDecode();
  }

  
  
  if (oldCount == 0 && mAnimatingImages) {
    nsresult rv2 = aImage->IncrementAnimationConsumers();
    rv = NS_SUCCEEDED(rv) ? rv2 : rv;
  }

  return rv;
}

static void
NotifyAudioAvailableListener(nsIContent *aContent, void *aUnused)
{
#ifdef MOZ_MEDIA
  nsCOMPtr<nsIDOMHTMLMediaElement> domMediaElem(do_QueryInterface(aContent));
  if (domMediaElem) {
    nsHTMLMediaElement* mediaElem = static_cast<nsHTMLMediaElement*>(aContent);
    mediaElem->NotifyAudioAvailableListener();
  }
#endif
}

void
nsDocument::NotifyAudioAvailableListener()
{
  mHasAudioAvailableListener = true;
  EnumerateFreezableElements(::NotifyAudioAvailableListener, nullptr);
}

nsresult
nsDocument::RemoveImage(imgIRequest* aImage)
{
  NS_ENSURE_ARG_POINTER(aImage);

  
  PRUint32 count = 0;
  DebugOnly<bool> found = mImageTracker.Get(aImage, &count);
  NS_ABORT_IF_FALSE(found, "Removing image that wasn't in the tracker!");
  NS_ABORT_IF_FALSE(count > 0, "Entry in the cache tracker with count 0!");

  
  count--;

  
  
  if (count != 0) {
    mImageTracker.Put(aImage, count);
    return NS_OK;
  }

  mImageTracker.Remove(aImage);

  nsresult rv = NS_OK;

  
  
  if (mLockingImages) {
    rv = aImage->UnlockImage();
  }

  
  if (mAnimatingImages) {
    nsresult rv2 = aImage->DecrementAnimationConsumers();
    rv = NS_SUCCEEDED(rv) ? rv2 : rv;
  }

  
  
  
  aImage->RequestDiscard();

  return rv;
}

nsresult
nsDocument::AddPlugin(nsIObjectLoadingContent* aPlugin)
{
  MOZ_ASSERT(aPlugin);
  if (!mPlugins.PutEntry(aPlugin)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return NS_OK;
}

void
nsDocument::RemovePlugin(nsIObjectLoadingContent* aPlugin)
{
  MOZ_ASSERT(aPlugin);
  mPlugins.RemoveEntry(aPlugin);
}

static bool
AllSubDocumentPluginEnum(nsIDocument* aDocument, void* userArg)
{
  nsTArray<nsIObjectLoadingContent*>* plugins =
    reinterpret_cast< nsTArray<nsIObjectLoadingContent*>* >(userArg);
  MOZ_ASSERT(plugins);
  aDocument->GetPlugins(*plugins);
  return true;
}

static PLDHashOperator
AllPluginEnum(nsPtrHashKey<nsIObjectLoadingContent>* aPlugin, void* userArg)
{
  nsTArray<nsIObjectLoadingContent*>* allPlugins =
    reinterpret_cast< nsTArray<nsIObjectLoadingContent*>* >(userArg);
  MOZ_ASSERT(allPlugins);
  allPlugins->AppendElement(aPlugin->GetKey());
  return PL_DHASH_NEXT;
}

void
nsDocument::GetPlugins(nsTArray<nsIObjectLoadingContent*>& aPlugins)
{
  aPlugins.SetCapacity(aPlugins.Length() + mPlugins.Count());
  mPlugins.EnumerateEntries(AllPluginEnum, &aPlugins);
  EnumerateSubDocuments(AllSubDocumentPluginEnum, &aPlugins);
}

PLDHashOperator LockEnumerator(imgIRequest* aKey,
                               PRUint32 aData,
                               void*    userArg)
{
  aKey->LockImage();
  aKey->RequestDecode();
  return PL_DHASH_NEXT;
}

PLDHashOperator UnlockEnumerator(imgIRequest* aKey,
                                 PRUint32 aData,
                                 void*    userArg)
{
  aKey->UnlockImage();
  return PL_DHASH_NEXT;
}


nsresult
nsDocument::SetImageLockingState(bool aLocked)
{
  if (XRE_GetProcessType() == GeckoProcessType_Content &&
      !Preferences::GetBool("content.image.allow_locking", true)) {
    return NS_OK;
  }

  
  if (mLockingImages == aLocked)
    return NS_OK;

  
  mImageTracker.EnumerateRead(aLocked ? LockEnumerator
                                      : UnlockEnumerator,
                              nullptr);

  
  mLockingImages = aLocked;

  return NS_OK;
}

PLDHashOperator IncrementAnimationEnumerator(imgIRequest* aKey,
                                             PRUint32 aData,
                                             void*    userArg)
{
  aKey->IncrementAnimationConsumers();
  return PL_DHASH_NEXT;
}

PLDHashOperator DecrementAnimationEnumerator(imgIRequest* aKey,
                                             PRUint32 aData,
                                             void*    userArg)
{
  aKey->DecrementAnimationConsumers();
  return PL_DHASH_NEXT;
}

void
nsDocument::SetImagesNeedAnimating(bool aAnimating)
{
  
  if (mAnimatingImages == aAnimating)
    return;

  
  mImageTracker.EnumerateRead(aAnimating ? IncrementAnimationEnumerator
                                         : DecrementAnimationEnumerator,
                              nullptr);

  
  mAnimatingImages = aAnimating;
}

NS_IMETHODIMP
nsDocument::CreateTouch(nsIDOMWindow* aView,
                        nsIDOMEventTarget* aTarget,
                        PRInt32 aIdentifier,
                        PRInt32 aPageX,
                        PRInt32 aPageY,
                        PRInt32 aScreenX,
                        PRInt32 aScreenY,
                        PRInt32 aClientX,
                        PRInt32 aClientY,
                        PRInt32 aRadiusX,
                        PRInt32 aRadiusY,
                        float aRotationAngle,
                        float aForce,
                        nsIDOMTouch** aRetVal)
{
  NS_ADDREF(*aRetVal = new nsDOMTouch(aTarget,
                                      aIdentifier,
                                      aPageX,
                                      aPageY,
                                      aScreenX,
                                      aScreenY,
                                      aClientX,
                                      aClientY,
                                      aRadiusX,
                                      aRadiusY,
                                      aRotationAngle,
                                      aForce));
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::CreateTouchList(nsIVariant* aPoints,
                            nsIDOMTouchList** aRetVal)
{
  nsRefPtr<nsDOMTouchList> retval = new nsDOMTouchList();
  if (aPoints) {
    PRUint16 type;
    aPoints->GetDataType(&type);
    if (type == nsIDataType::VTYPE_INTERFACE ||
        type == nsIDataType::VTYPE_INTERFACE_IS) {
      nsCOMPtr<nsISupports> data;
      aPoints->GetAsISupports(getter_AddRefs(data));
      nsCOMPtr<nsIDOMTouch> point = do_QueryInterface(data);
      if (point) {
        retval->Append(point);
      }
    } else if (type == nsIDataType::VTYPE_ARRAY) {
      PRUint16 valueType;
      nsIID iid;
      PRUint32 valueCount;
      void* rawArray;
      aPoints->GetAsArray(&valueType, &iid, &valueCount, &rawArray);
      if (valueType == nsIDataType::VTYPE_INTERFACE ||
          valueType == nsIDataType::VTYPE_INTERFACE_IS) {
        nsISupports** values = static_cast<nsISupports**>(rawArray);
        for (PRUint32 i = 0; i < valueCount; ++i) {
          nsCOMPtr<nsISupports> supports = dont_AddRef(values[i]);
          nsCOMPtr<nsIDOMTouch> point = do_QueryInterface(supports);
          if (point) {
            retval->Append(point);
          }
        }
      }
      nsMemory::Free(rawArray);
    }
  }

  *aRetVal = retval.forget().get();
  return NS_OK;
}

static void
DispatchFullScreenChange(nsIDocument* aTarget)
{
  nsRefPtr<nsAsyncDOMEvent> e =
    new nsAsyncDOMEvent(aTarget,
                        NS_LITERAL_STRING("mozfullscreenchange"),
                        true,
                        false);
  e->PostDOMEvent();
}

NS_IMETHODIMP
nsDocument::MozCancelFullScreen()
{
  if (!nsContentUtils::IsRequestFullScreenAllowed()) {
    return NS_OK;
  }
  RestorePreviousFullScreenState();
  return NS_OK;
}






class nsSetWindowFullScreen : public nsRunnable {
public:
  nsSetWindowFullScreen(nsIDocument* aDoc, bool aValue)
    : mDoc(aDoc), mValue(aValue) {}

  NS_IMETHOD Run()
  {
    if (mDoc->GetWindow()) {
      mDoc->GetWindow()->SetFullScreenInternal(mValue, false);
    }
    return NS_OK;
  }

private:
  nsCOMPtr<nsIDocument> mDoc;
  bool mValue;
};

static void
SetWindowFullScreen(nsIDocument* aDoc, bool aValue)
{
  nsContentUtils::AddScriptRunner(new nsSetWindowFullScreen(aDoc, aValue));
}

class nsCallExitFullScreen : public nsRunnable {
public:
  NS_IMETHOD Run()
  {
    nsDocument::ExitFullScreen();
    return NS_OK;
  }
};


void
nsIDocument::ExitFullScreen(bool aRunAsync)
{
  if (aRunAsync) {
    NS_DispatchToCurrentThread(new nsCallExitFullScreen());
    return;
  }
  nsDocument::ExitFullScreen();
}




static bool
HasCrossProcessParent(nsIDocument* aDocument)
{
  if (XRE_GetProcessType() != GeckoProcessType_Content) {
    return false;
  }
  if (aDocument->GetParentDocument() != nullptr) {
    return false;
  }
  nsPIDOMWindow* win = aDocument->GetWindow();
  if (!win) {
    return false;
  }
  nsCOMPtr<nsIDocShell> docShell = win->GetDocShell();
  if (!docShell) {
    return false;
  }
  bool isBrowserElement = false;
  docShell->GetIsBrowserElement(&isBrowserElement);
  return isBrowserElement;
}

static bool
ResetFullScreen(nsIDocument* aDocument, void* aData)
{
  if (aDocument->IsFullScreenDoc()) {
    static_cast<nsDocument*>(aDocument)->CleanupFullscreenState();
    NS_ASSERTION(!aDocument->IsFullScreenDoc(), "Should reset full-screen");
    nsTArray<nsIDocument*>* changed = reinterpret_cast<nsTArray<nsIDocument*>*>(aData);
    changed->AppendElement(aDocument);

    if (HasCrossProcessParent(aDocument)) {
      
      
      nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
      os->NotifyObservers(aDocument, "ask-parent-to-exit-fullscreen", nullptr);
    }

    
    
    
    
    nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
    os->NotifyObservers(aDocument, "ask-children-to-exit-fullscreen", nullptr);

    aDocument->EnumerateSubDocuments(ResetFullScreen, aData);
  }
  return true;
}


void
nsDocument::ExitFullScreen()
{
  
  nsCOMPtr<nsIDocument> root(do_QueryReferent(sFullScreenRootDoc));
  if (!root) {
    
    return;
  }
  NS_ASSERTION(root->IsFullScreenDoc(),
    "Full-screen root should be a full-screen doc...");

  
  
  
  
  
  nsAutoTArray<nsIDocument*, 8> changed;

  
  nsCOMPtr<Element> pointerLockedElement =
    do_QueryReferent(nsEventStateManager::sPointerLockedElement);
  if (pointerLockedElement) {
    UnlockPointer();
  }

  
  ResetFullScreen(root, static_cast<void*>(&changed));

  
  
  
  for (PRUint32 i = 0; i < changed.Length(); ++i) {
    DispatchFullScreenChange(changed[changed.Length() - i - 1]);
  }

  
  
  
  sFullScreenRootDoc = nullptr;
  sFullScreenDoc = nullptr;

  
  SetWindowFullScreen(root, false);
}

void
nsDocument::RestorePreviousFullScreenState()
{
  NS_ASSERTION(!IsFullScreenDoc() || sFullScreenDoc != nullptr,
               "Should have a full-screen doc when full-screen!");

  if (!IsFullScreenDoc() || !GetWindow() || !sFullScreenDoc) {
    return;
  }

  
  nsCOMPtr<Element> pointerLockedElement =
    do_QueryReferent(nsEventStateManager::sPointerLockedElement);
  if (pointerLockedElement) {
    UnlockPointer();
  }

  nsCOMPtr<nsIDocument> fullScreenDoc(do_QueryReferent(sFullScreenDoc));

  
  
  
  nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
  os->NotifyObservers(fullScreenDoc, "ask-children-to-exit-fullscreen", nullptr);

  
  nsIDocument* doc = fullScreenDoc;
  while (doc != this) {
    NS_ASSERTION(doc->IsFullScreenDoc(), "Should be full-screen doc");
    static_cast<nsDocument*>(doc)->CleanupFullscreenState();
    UnlockPointer();
    DispatchFullScreenChange(doc);
    doc = doc->GetParentDocument();
  }

  
  NS_ASSERTION(doc == this, "Must have reached this doc.");
  while (doc != nullptr) {
    static_cast<nsDocument*>(doc)->FullScreenStackPop();
    UnlockPointer();
    DispatchFullScreenChange(doc);
    if (static_cast<nsDocument*>(doc)->mFullScreenStack.IsEmpty()) {
      if (HasCrossProcessParent(doc)) {
        
        
        nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
        os->NotifyObservers(doc, "ask-parent-to-rollback-fullscreen", nullptr);
      }
      
      
      
      static_cast<nsDocument*>(doc)->CleanupFullscreenState();
      doc = doc->GetParentDocument();
    } else {
      
      
      if (fullScreenDoc != doc) {
        
        
        
        
        if (!nsContentUtils::HaveEqualPrincipals(fullScreenDoc, doc) ||
            (!nsContentUtils::IsSitePermAllow(doc->NodePrincipal(), "fullscreen") &&
             !static_cast<nsDocument*>(doc)->mIsApprovedForFullscreen)) {
          nsRefPtr<nsAsyncDOMEvent> e =
            new nsAsyncDOMEvent(doc,
                                NS_LITERAL_STRING("MozEnteredDomFullscreen"),
                                true,
                                true);
          e->PostDOMEvent();
        }
      }

      if (!nsContentUtils::HaveEqualPrincipals(doc, fullScreenDoc)) {
        
        
        
        nsAutoString origin;
        nsContentUtils::GetUTFOrigin(doc->NodePrincipal(), origin);
        nsIDocument* root = nsContentUtils::GetRootDocument(doc);
        nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
        os->NotifyObservers(root, "fullscreen-origin-change", origin.get());
      }

      sFullScreenDoc = do_GetWeakReference(doc);
      break;
    }
  }

  if (doc == nullptr) {
    
    
    DebugOnly< nsCOMPtr<nsIDocument> > root(do_QueryReferent(sFullScreenRootDoc));
    NS_ASSERTION(!root->IsFullScreenDoc(), "Should have cleared all docs' stacks");
    sFullScreenDoc = nullptr;
    sFullScreenRootDoc = nullptr;
    SetWindowFullScreen(this, false);
  }
}

bool
nsDocument::IsFullScreenDoc()
{
  return GetFullScreenElement() != nullptr;
}

class nsCallRequestFullScreen : public nsRunnable
{
public:
  nsCallRequestFullScreen(Element* aElement)
    : mElement(aElement),
      mDoc(aElement->OwnerDoc()),
      mWasCallerChrome(nsContentUtils::IsCallerChrome())
  {
  }

  NS_IMETHOD Run()
  {
    nsDocument* doc = static_cast<nsDocument*>(mDoc.get());
    doc->RequestFullScreen(mElement,
                           mWasCallerChrome,
                            true);
    return NS_OK;
  }

  nsRefPtr<Element> mElement;
  nsCOMPtr<nsIDocument> mDoc;
  bool mWasCallerChrome;
};

void
nsDocument::AsyncRequestFullScreen(Element* aElement)
{
  NS_ASSERTION(aElement,
    "Must pass non-null element to nsDocument::AsyncRequestFullScreen");
  if (!aElement) {
    return;
  }
  
  nsCOMPtr<nsIRunnable> event(new nsCallRequestFullScreen(aElement));
  NS_DispatchToCurrentThread(event);
}

static void
LogFullScreenDenied(bool aLogFailure, const char* aMessage, nsIDocument* aDoc)
{
  if (!aLogFailure) {
    return;
  }
  nsRefPtr<nsAsyncDOMEvent> e =
    new nsAsyncDOMEvent(aDoc,
                        NS_LITERAL_STRING("mozfullscreenerror"),
                        true,
                        false);
  e->PostDOMEvent();
  nsContentUtils::ReportToConsole(nsIScriptError::warningFlag,
                                  "DOM", aDoc,
                                  nsContentUtils::eDOM_PROPERTIES,
                                  aMessage);
}

nsresult
nsDocument::AddFullscreenApprovedObserver()
{
  nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
  NS_ENSURE_TRUE(os, NS_ERROR_FAILURE);

  nsresult res = os->AddObserver(this, "fullscreen-approved", true);
  NS_ENSURE_SUCCESS(res, res);

  return NS_OK;
}

nsresult
nsDocument::RemoveFullscreenApprovedObserver()
{
  nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
  NS_ENSURE_TRUE(os, NS_ERROR_FAILURE);

  nsresult res = os->RemoveObserver(this, "fullscreen-approved");
  NS_ENSURE_SUCCESS(res, res);

  return NS_OK;
}

void
nsDocument::CleanupFullscreenState()
{
  if (!mFullScreenStack.IsEmpty()) {
    
    
    
    Element* top = FullScreenStackTop();
    NS_ASSERTION(top, "Should have a top when full-screen stack isn't empty");
    if (top) {
      nsEventStateManager::SetFullScreenState(top, false);
    }
    mFullScreenStack.Clear();
  }
  SetApprovedForFullscreen(false);
  RemoveFullscreenApprovedObserver();
}

bool
nsDocument::FullScreenStackPush(Element* aElement)
{
  NS_ASSERTION(aElement, "Must pass non-null to FullScreenStackPush()");
  Element* top = FullScreenStackTop();
  if (top == aElement || !aElement) {
    return false;
  }
  if (top) {
    
    
    
    nsEventStateManager::SetFullScreenState(top, false);
  }
  nsEventStateManager::SetFullScreenState(aElement, true);
  mFullScreenStack.AppendElement(do_GetWeakReference(aElement));
  NS_ASSERTION(GetFullScreenElement() == aElement, "Should match");
  return true;
}

void
nsDocument::FullScreenStackPop()
{
  if (mFullScreenStack.IsEmpty()) {
    return;
  }

  
  Element* top = FullScreenStackTop();
  nsEventStateManager::SetFullScreenState(top, false);

  
  
  
  PRUint32 last = mFullScreenStack.Length() - 1;
  mFullScreenStack.RemoveElementAt(last);

  
  
  
  while (!mFullScreenStack.IsEmpty()) {
    Element* element = FullScreenStackTop();
    if (!element || !element->IsInDoc() || element->OwnerDoc() != this) {
      NS_ASSERTION(!element->IsFullScreenAncestor(),
                   "Should have already removed full-screen styles");
      PRUint32 last = mFullScreenStack.Length() - 1;
      mFullScreenStack.RemoveElementAt(last);
    } else {
      
      
      nsEventStateManager::SetFullScreenState(element, true);
      break;
    }
  }
}

Element*
nsDocument::FullScreenStackTop()
{
  if (mFullScreenStack.IsEmpty()) {
    return nullptr;
  }
  PRUint32 last = mFullScreenStack.Length() - 1;
  nsCOMPtr<Element> element(do_QueryReferent(mFullScreenStack[last]));
  NS_ASSERTION(element, "Should have full-screen element!");
  NS_ASSERTION(element->IsInDoc(), "Full-screen element should be in doc");
  NS_ASSERTION(element->OwnerDoc() == this, "Full-screen element should be in this doc");
  return element;
}


static bool
IsInActiveTab(nsIDocument* aDoc)
{
  nsCOMPtr<nsISupports> container = aDoc->GetContainer();
  nsCOMPtr<nsIDocShell> docshell = do_QueryInterface(container);
  if (!docshell) {
    return false;
  }

  bool isActive = false;
  docshell->GetIsActive(&isActive);
  if (!isActive) {
    return false;
  }

  nsCOMPtr<nsIDocShellTreeItem> dsti = do_QueryInterface(container);
  if (!dsti) {
    return false;
  }
  nsCOMPtr<nsIDocShellTreeItem> rootItem;
  dsti->GetRootTreeItem(getter_AddRefs(rootItem));
  if (!rootItem) {
    return false;
  }
  nsCOMPtr<nsIDOMWindow> rootWin = do_GetInterface(rootItem);
  if (!rootWin) {
    return false;
  }

  nsIFocusManager* fm = nsFocusManager::GetFocusManager();
  if (!fm) {
    return false;
  }

  nsCOMPtr<nsIDOMWindow> activeWindow;
  fm->GetActiveWindow(getter_AddRefs(activeWindow));
  if (!activeWindow) {
    return false;
  }

  return activeWindow == rootWin;
}

nsresult nsDocument::RemoteFrameFullscreenChanged(nsIDOMElement* aFrameElement,
                                                  const nsAString& aOrigin)
{
  
  
  
  nsCOMPtr<nsIContent> content(do_QueryInterface(aFrameElement));
  RequestFullScreen(content->AsElement(),
                     false,
                     false);

  
  
  
  
  
  
  if (!aOrigin.IsEmpty()) {
    nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
    os->NotifyObservers(nsContentUtils::GetRootDocument(this),
                        "fullscreen-origin-change",
                        PromiseFlatString(aOrigin).get());
  }

  return NS_OK;
}

nsresult nsDocument::RemoteFrameFullscreenReverted()
{
  RestorePreviousFullScreenState();
  return NS_OK;
}

void
nsDocument::RequestFullScreen(Element* aElement,
                              bool aWasCallerChrome,
                              bool aNotifyOnOriginChange)
{
  NS_ASSERTION(aElement,
    "Must pass non-null element to nsDocument::RequestFullScreen");
  if (!aElement || aElement == GetFullScreenElement()) {
    return;
  }
  if (!aElement->IsInDoc()) {
    LogFullScreenDenied(true, "FullScreenDeniedNotInDocument", this);
    return;
  }
  if (aElement->OwnerDoc() != this) {
    LogFullScreenDenied(true, "FullScreenDeniedMovedDocument", this);
    return;
  }
  if (!GetWindow()) {
    LogFullScreenDenied(true, "FullScreenDeniedLostWindow", this);
    return;
  }
  if (!IsFullScreenEnabled(aWasCallerChrome, true)) {
    
    return;
  }
  if (GetFullScreenElement() &&
      !nsContentUtils::ContentIsDescendantOf(aElement, GetFullScreenElement())) {
    
    
    LogFullScreenDenied(true, "FullScreenDeniedNotDescendant", this);
    return;
  }
  if (!nsContentUtils::IsChromeDoc(this) && !IsInActiveTab(this)) {
    LogFullScreenDenied(true, "FullScreenDeniedNotFocusedTab", this);
    return;
  }
  
  nsIFocusManager* fm = nsFocusManager::GetFocusManager();
  if (!fm) {
    NS_WARNING("Failed to retrieve focus manager in full-screen request.");
    return;
  }
  nsCOMPtr<nsIDOMElement> focusedElement;
  fm->GetFocusedElement(getter_AddRefs(focusedElement));
  if (focusedElement) {
    nsCOMPtr<nsIContent> content = do_QueryInterface(focusedElement);
    if (nsContentUtils::HasPluginWithUncontrolledEventDispatch(content)) {
      LogFullScreenDenied(true, "FullScreenDeniedFocusedPlugin", this);
      return;
    }
  }

  AddFullscreenApprovedObserver();

  
  
  
  
  
  nsAutoTArray<nsIDocument*, 8> changed;

  
  
  nsIDocument* fullScreenRootDoc = nsContentUtils::GetRootDocument(this);
  sFullScreenRootDoc = do_GetWeakReference(fullScreenRootDoc);

  
  
  if (sFullScreenDoc) {
    UnlockPointer();
  }

  
  
  nsCOMPtr<Element> pointerLockedElement =
    do_QueryReferent(nsEventStateManager::sPointerLockedElement);
  if (pointerLockedElement) {
    UnlockPointer();
  }

  
  
  
  DebugOnly<bool> x = FullScreenStackPush(aElement);
  NS_ASSERTION(x, "Full-screen state of requesting doc should always change!");
  changed.AppendElement(this);

  
  
  
  
  
  nsIDocument* child = this;
  while (true) {
    nsIDocument* parent = child->GetParentDocument();
    if (!parent) {
      break;
    }
    Element* element = parent->FindContentForSubDocument(child)->AsElement();
    if (static_cast<nsDocument*>(parent)->FullScreenStackPush(element)) {
      changed.AppendElement(parent);
      child = parent;
    } else {
      
      
      
      
      break;
    }
  }

  
  
  
  for (PRUint32 i = 0; i < changed.Length(); ++i) {
    DispatchFullScreenChange(changed[changed.Length() - i - 1]);
  }

  
  
  
  
  
  if (!mIsApprovedForFullscreen) {
    mIsApprovedForFullscreen =
      GetWindow()->IsInAppOrigin() ||
      nsContentUtils::IsSitePermAllow(NodePrincipal(), "fullscreen");
  }

  
  
  
  nsCOMPtr<nsIDocument> previousFullscreenDoc(do_QueryReferent(sFullScreenDoc));
  
  
  
  if (!mIsApprovedForFullscreen ||
      !nsContentUtils::HaveEqualPrincipals(previousFullscreenDoc, this)) {
    nsRefPtr<nsAsyncDOMEvent> e =
      new nsAsyncDOMEvent(this,
                          NS_LITERAL_STRING("MozEnteredDomFullscreen"),
                          true,
                          true);
    e->PostDOMEvent();
  }

  
  sFullScreenDoc = do_GetWeakReference(static_cast<nsIDocument*>(this));

#ifdef DEBUG
  
  
  NS_ASSERTION(GetFullScreenElement() == aElement,
               "Full-screen element should be the requested element!");
  NS_ASSERTION(IsFullScreenDoc(), "Should be full-screen doc");
  nsCOMPtr<nsIDOMElement> fse;
  GetMozFullScreenElement(getter_AddRefs(fse));
  nsCOMPtr<nsIContent> c(do_QueryInterface(fse));
  NS_ASSERTION(c->AsElement() == aElement,
    "GetMozFullScreenElement should match GetFullScreenElement()");
#endif

  
  
  
  
  
  
  if (aNotifyOnOriginChange &&
      !nsContentUtils::HaveEqualPrincipals(previousFullscreenDoc, this)) {
    nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
    nsIDocument* root = nsContentUtils::GetRootDocument(this);
    nsAutoString origin;
    nsContentUtils::GetUTFOrigin(NodePrincipal(), origin);
    os->NotifyObservers(root, "fullscreen-origin-change", origin.get());
  }

  
  
  
  
  
  
  
  SetWindowFullScreen(this, true);
}

NS_IMETHODIMP
nsDocument::GetMozFullScreenElement(nsIDOMElement **aFullScreenElement)
{
  NS_ENSURE_ARG_POINTER(aFullScreenElement);
  *aFullScreenElement = nullptr;
  if (IsFullScreenDoc()) {
    
    NS_ENSURE_STATE(GetFullScreenElement());
    CallQueryInterface(GetFullScreenElement(), aFullScreenElement);
  }
  return NS_OK;
}

Element*
nsDocument::GetFullScreenElement()
{
  Element* element = FullScreenStackTop();
  NS_ASSERTION(!element || element->IsFullScreenAncestor(),
    "Should have full-screen styles applied!");
  return element;
}

NS_IMETHODIMP
nsDocument::GetMozFullScreen(bool *aFullScreen)
{
  NS_ENSURE_ARG_POINTER(aFullScreen);
  *aFullScreen = IsFullScreenDoc();
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetMozFullScreenEnabled(bool *aFullScreen)
{
  NS_ENSURE_ARG_POINTER(aFullScreen);
  *aFullScreen = IsFullScreenEnabled(nsContentUtils::IsCallerChrome(), false);
  return NS_OK;
}

static bool
HasFullScreenSubDocument(nsIDocument* aDoc, void* aData)
{
  if (aDoc->IsFullScreenDoc()) {
    
    
    *static_cast<bool*>(aData) = true;
    return false;
  }
  return true;
}

static bool
HasFullScreenSubDocument(nsIDocument* aDoc)
{
  bool result = false;
  aDoc->EnumerateSubDocuments(&HasFullScreenSubDocument, static_cast<void*>(&result));
  return result;
}

bool
nsDocument::IsFullScreenEnabled(bool aCallerIsChrome, bool aLogFailure)
{
  if (nsContentUtils::IsFullScreenApiEnabled() && aCallerIsChrome) {
    
    
    
    
    return true;
  }

  if (!nsContentUtils::IsFullScreenApiEnabled()) {
    LogFullScreenDenied(aLogFailure, "FullScreenDeniedDisabled", this);
    return false;
  }
  if (!IsVisible()) {
    LogFullScreenDenied(aLogFailure, "FullScreenDeniedHidden", this);
    return false;
  }
  if (HasFullScreenSubDocument(this)) {
    LogFullScreenDenied(aLogFailure, "FullScreenDeniedSubDocFullScreen", this);
    return false;
  }

  
  
  nsINode* node = static_cast<nsINode*>(this);
  do {
    nsIContent* content = static_cast<nsIContent*>(node);
    if (content->IsHTML(nsGkAtoms::iframe) &&
        !content->HasAttr(kNameSpaceID_None, nsGkAtoms::mozallowfullscreen)) {
      
      
      
      LogFullScreenDenied(aLogFailure, "FullScreenDeniedIframeDisallowed", this);
      return false;
    }
    node = nsContentUtils::GetCrossDocParentNode(node);
  } while (node);

  return true;
}

static void
DispatchPointerLockChange(nsIDocument* aTarget)
{
  nsRefPtr<nsAsyncDOMEvent> e =
    new nsAsyncDOMEvent(aTarget,
                        NS_LITERAL_STRING("mozpointerlockchange"),
                        true,
                        false);
  e->PostDOMEvent();
}

static void
DispatchPointerLockError(nsIDocument* aTarget)
{
  nsRefPtr<nsAsyncDOMEvent> e =
    new nsAsyncDOMEvent(aTarget,
                        NS_LITERAL_STRING("mozpointerlockerror"),
                        true,
                        false);
  e->PostDOMEvent();
}



class nsAsyncPointerLockRequest : public nsRunnable
{
public:
  NS_IMETHOD Run()
  {
    sInstance = nullptr;
    if (mDocument && mElement) {
      mDocument->RequestPointerLock(mElement);
    }
    return NS_OK;
  }

  static void Request(Element* aElement, nsIDocument* aDocument)
  {
    if (sInstance) {
      
      
      sInstance->mElement = aElement;
      sInstance->mDocument = aDocument;
    } else {
      
      
      sInstance = new nsAsyncPointerLockRequest(aElement, aDocument);
      NS_DispatchToCurrentThread(sInstance);
    }
  }

  static void Cancel()
  {
    if (sInstance) {
      
      
      
      sInstance->mElement = nullptr;
      sInstance->mDocument = nullptr;
    }
  }

private:
  nsAsyncPointerLockRequest(Element* aElement, nsIDocument* aDocument)
    : mElement(aElement),
      mDocument(aDocument)
  {
    MOZ_COUNT_CTOR(nsAsyncPointerLockRequest);
  }

  ~nsAsyncPointerLockRequest()
  {
    MOZ_COUNT_DTOR(nsAsyncPointerLockRequest);
  }

  
  
  
  
  static nsAsyncPointerLockRequest* sInstance;

  
  nsCOMPtr<Element> mElement;
  nsCOMPtr<nsIDocument> mDocument;
};

nsAsyncPointerLockRequest* nsAsyncPointerLockRequest::sInstance = nullptr;
nsWeakPtr nsDocument::sPendingPointerLockDoc;
nsWeakPtr nsDocument::sPendingPointerLockElement;


void
nsDocument::ClearPendingPointerLockRequest(bool aDispatchErrorEvents)
{
  nsAsyncPointerLockRequest::Cancel();

  if (!sPendingPointerLockDoc) {
    
    return;
  }
  nsCOMPtr<nsIDocument> doc(do_QueryReferent(sPendingPointerLockDoc));
  if (aDispatchErrorEvents) {
    DispatchPointerLockError(doc);
  }
  nsCOMPtr<Element> element(do_QueryReferent(sPendingPointerLockElement));
#ifdef DEBUG
  nsCOMPtr<Element> pointerLockedElement =
    do_QueryReferent(nsEventStateManager::sPointerLockedElement);
  NS_ASSERTION(pointerLockedElement != element,
    "We shouldn't be clearing pointer locked flag on pointer locked element!");
#endif
  if (element) {
    element->ClearPointerLock();
  }
  sPendingPointerLockDoc = nullptr;
  sPendingPointerLockElement = nullptr;
}


nsresult
nsDocument::SetPendingPointerLockRequest(Element* aElement)
{
  
  ClearPendingPointerLockRequest(true);

  NS_ENSURE_TRUE(aElement != nullptr, NS_ERROR_FAILURE);

  sPendingPointerLockDoc = do_GetWeakReference(aElement->OwnerDoc());
  sPendingPointerLockElement = do_GetWeakReference(aElement);

  
  
  aElement->SetPointerLock();

  return NS_OK;
}

void
nsDocument::SetApprovedForFullscreen(bool aIsApproved)
{
  mIsApprovedForFullscreen = aIsApproved;
}

nsresult
nsDocument::Observe(nsISupports *aSubject,
                    const char *aTopic,
                    const PRUnichar *aData)
{
  if (strcmp("fullscreen-approved", aTopic) == 0) {
    nsCOMPtr<nsIDocument> subject(do_QueryInterface(aSubject));
    if (subject != this) {
      return NS_OK;
    }
    SetApprovedForFullscreen(true);
    nsCOMPtr<nsIDocument> doc(do_QueryReferent(sPendingPointerLockDoc));
    if (this == doc) {
      
      
      nsCOMPtr<Element> element(do_QueryReferent(sPendingPointerLockElement));
      nsDocument::ClearPendingPointerLockRequest(false);
      nsAsyncPointerLockRequest::Request(element, this);
    }
  }
  return NS_OK;
}

void
nsDocument::RequestPointerLock(Element* aElement)
{
  NS_ASSERTION(aElement,
    "Must pass non-null element to nsDocument::RequestPointerLock");

  nsCOMPtr<Element> pointerLockedElement =
    do_QueryReferent(nsEventStateManager::sPointerLockedElement);
  if (aElement == pointerLockedElement) {
    DispatchPointerLockChange(this);
    return;
  }

  if (!ShouldLockPointer(aElement)) {
    DispatchPointerLockError(this);
    return;
  }

  if (!mIsApprovedForFullscreen) {
    
    
    if (NS_FAILED(SetPendingPointerLockRequest(aElement))) {
      NS_WARNING("Failed to make pointer lock request pending!");
      DispatchPointerLockError(this);
    }
    return;
  }

  
  nsDocument::ClearPendingPointerLockRequest(true);

  if (!SetPointerLock(aElement, NS_STYLE_CURSOR_NONE)) {
    DispatchPointerLockError(this);
    return;
  }

  aElement->SetPointerLock();
  nsEventStateManager::sPointerLockedElement = do_GetWeakReference(aElement);
  nsEventStateManager::sPointerLockedDoc =
    do_GetWeakReference(static_cast<nsIDocument*>(this));
  DispatchPointerLockChange(this);
}

bool
nsDocument::ShouldLockPointer(Element* aElement)
{
  
  if (!Preferences::GetBool("full-screen-api.pointer-lock.enabled")) {
    NS_WARNING("ShouldLockPointer(): Pointer Lock pref not enabled");
    return false;
  }

  if (aElement != GetFullScreenElement()) {
    NS_WARNING("ShouldLockPointer(): Element not in fullscreen");
    return false;
  }

  if (!aElement->IsInDoc()) {
    NS_WARNING("ShouldLockPointer(): Element without Document");
    return false;
  }

  
  nsCOMPtr<nsIDocument> ownerDoc = aElement->OwnerDoc();
  if (!ownerDoc) {
    return false;
  }
  if (!nsCOMPtr<nsISupports>(ownerDoc->GetContainer())) {
    return false;
  }
  nsCOMPtr<nsPIDOMWindow> ownerWindow = ownerDoc->GetWindow();
  if (!ownerWindow) {
    return false;
  }
  nsCOMPtr<nsPIDOMWindow> ownerInnerWindow = ownerDoc->GetInnerWindow();
  if (!ownerInnerWindow) {
    return false;
  }
  if (ownerWindow->GetCurrentInnerWindow() != ownerInnerWindow) {
    return false;
  }

  return true;
}

bool
nsDocument::SetPointerLock(Element* aElement, int aCursorStyle)
{
  
  nsCOMPtr<nsPIDOMWindow> window = GetWindow();
  if (!window) {
    NS_WARNING("SetPointerLock(): No Window");
    return false;
  }

  nsIDocShell *docShell = window->GetDocShell();
  if (!docShell) {
    NS_WARNING("SetPointerLock(): No DocShell (window already closed?)");
    return false;
  }

  nsRefPtr<nsPresContext> presContext;
  docShell->GetPresContext(getter_AddRefs(presContext));
  if (!presContext) {
    NS_WARNING("SetPointerLock(): Unable to get presContext in \
                domWindow->GetDocShell()->GetPresContext()");
    return false;
  }

  nsCOMPtr<nsIPresShell> shell = presContext->PresShell();
  if (!shell) {
    NS_WARNING("SetPointerLock(): Unable to find presContext->PresShell()");
    return false;
  }

  nsIFrame* rootFrame = shell->GetRootFrame();
  if (!rootFrame) {
    NS_WARNING("SetPointerLock(): Unable to get root frame");
    return false;
  }

  nsCOMPtr<nsIWidget> widget = rootFrame->GetNearestWidget();
  if (!widget) {
    NS_WARNING("SetPointerLock(): Unable to find widget in \
                shell->GetRootFrame()->GetNearestWidget();");
    return false;
  }

  if (aElement && (aElement->OwnerDoc() != this)) {
    NS_WARNING("SetPointerLock(): Element not in this document.");
    return false;
  }

  
  nsRefPtr<nsEventStateManager> esm = presContext->EventStateManager();
  esm->SetCursor(aCursorStyle, nullptr, false,
                 0.0f, 0.0f, widget, true);
  esm->SetPointerLock(widget, aElement);

  return true;
}

void
nsDocument::UnlockPointer()
{
  
  
  ClearPendingPointerLockRequest(true);

  if (!nsEventStateManager::sIsPointerLocked) {
    return;
  }

  nsCOMPtr<nsIDocument> pointerLockedDoc =
    do_QueryReferent(nsEventStateManager::sPointerLockedDoc);
  if (!pointerLockedDoc) {
    return;
  }
  nsDocument* doc = static_cast<nsDocument*>(pointerLockedDoc.get());
  if (!doc->SetPointerLock(nullptr, NS_STYLE_CURSOR_AUTO)) {
    return;
  }

  nsCOMPtr<Element> pointerLockedElement =
    do_QueryReferent(nsEventStateManager::sPointerLockedElement);
  if (!pointerLockedElement) {
    return;
  }

  nsEventStateManager::sPointerLockedElement = nullptr;
  nsEventStateManager::sPointerLockedDoc = nullptr;
  pointerLockedElement->ClearPointerLock();
  DispatchPointerLockChange(pointerLockedDoc);
}

void
nsIDocument::UnlockPointer()
{
  nsDocument::UnlockPointer();
}

NS_IMETHODIMP
nsDocument::MozExitPointerLock()
{
  UnlockPointer();
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetMozPointerLockElement(nsIDOMElement** aPointerLockedElement)
{
  NS_ENSURE_ARG_POINTER(aPointerLockedElement);
  *aPointerLockedElement = nullptr;
  nsCOMPtr<Element> pointerLockedElement =
    do_QueryReferent(nsEventStateManager::sPointerLockedElement);
  if (!pointerLockedElement) {
    return NS_OK;
  }

  
  nsCOMPtr<nsIDocument> pointerLockedDoc =
    do_QueryReferent(nsEventStateManager::sPointerLockedDoc);
  nsDocument* doc = static_cast<nsDocument*>(pointerLockedDoc.get());
  if (doc != this) {
    return NS_OK;
  }
  nsCOMPtr<nsIDOMNode> pointerLockedNode =
    do_QueryInterface(pointerLockedElement);
  nsresult rv = nsContentUtils::CheckSameOrigin(this, pointerLockedNode.get());
  if (NS_FAILED(rv)) {
    return NS_OK;
  }

  return CallQueryInterface(pointerLockedElement, aPointerLockedElement);
}

#define EVENT(name_, id_, type_, struct_)                                 \
  NS_IMETHODIMP nsDocument::GetOn##name_(JSContext *cx, jsval *vp) {      \
    return nsINode::GetOn##name_(cx, vp);                                 \
  }                                                                       \
  NS_IMETHODIMP nsDocument::SetOn##name_(JSContext *cx, const jsval &v) { \
    return nsINode::SetOn##name_(cx, v);                                  \
  }
#define TOUCH_EVENT EVENT
#define DOCUMENT_ONLY_EVENT EVENT
#include "nsEventNameList.h"
#undef DOCUMENT_ONLY_EVENT
#undef TOUCH_EVENT
#undef EVENT

void
nsDocument::UpdateVisibilityState()
{
  VisibilityState oldState = mVisibilityState;
  mVisibilityState = GetVisibilityState();
  if (oldState != mVisibilityState) {
    nsContentUtils::DispatchTrustedEvent(this, static_cast<nsIDocument*>(this),
                                         NS_LITERAL_STRING("mozvisibilitychange"),
                                          true,
                                          false);
  }
}

nsDocument::VisibilityState
nsDocument::GetVisibilityState() const
{
  
  
  
  
  
  
  
  if (!IsVisible() || !mWindow || !mWindow->GetOuterWindow() ||
      mWindow->GetOuterWindow()->IsBackground()) {
    return eHidden;
  }

  return eVisible;
}

 void
nsDocument::PostVisibilityUpdateEvent()
{
  nsCOMPtr<nsIRunnable> event =
    NS_NewRunnableMethod(this, &nsDocument::UpdateVisibilityState);
  NS_DispatchToMainThread(event);
}

NS_IMETHODIMP
nsDocument::GetMozHidden(bool* aHidden)
{
  *aHidden = mVisibilityState != eVisible;
  return NS_OK;
}

NS_IMETHODIMP
nsDocument::GetMozVisibilityState(nsAString& aState)
{
  
  static const char states[][8] = {
    "hidden",
    "visible"
  };
  PR_STATIC_ASSERT(NS_ARRAY_LENGTH(states) == eVisibilityStateCount);
  aState.AssignASCII(states[mVisibilityState]);
  return NS_OK;
}

 void
nsIDocument::DocSizeOfExcludingThis(nsWindowSizes* aWindowSizes) const
{
  aWindowSizes->mDOMOther +=
    nsINode::SizeOfExcludingThis(aWindowSizes->mMallocSizeOf);

  if (mPresShell) {
    mPresShell->SizeOfIncludingThis(aWindowSizes->mMallocSizeOf,
                                    &aWindowSizes->mArenaStats,
                                    &aWindowSizes->mLayoutPresShell,
                                    &aWindowSizes->mLayoutStyleSets,
                                    &aWindowSizes->mLayoutTextRuns,
                                    &aWindowSizes->mLayoutPresContext);
  }

  aWindowSizes->mPropertyTables +=
    mPropertyTable.SizeOfExcludingThis(aWindowSizes->mMallocSizeOf);
  for (PRUint32 i = 0, count = mExtraPropertyTables.Length();
       i < count; ++i) {
    aWindowSizes->mPropertyTables +=
      mExtraPropertyTables[i]->SizeOfExcludingThis(aWindowSizes->mMallocSizeOf);
  }

  
  
  
}

void
nsIDocument::DocSizeOfIncludingThis(nsWindowSizes* aWindowSizes) const
{
  aWindowSizes->mDOMOther += aWindowSizes->mMallocSizeOf(this);
  DocSizeOfExcludingThis(aWindowSizes);
}

static size_t
SizeOfStyleSheetsElementIncludingThis(nsIStyleSheet* aStyleSheet,
                                      nsMallocSizeOfFun aMallocSizeOf,
                                      void* aData)
{
  return aStyleSheet->SizeOfIncludingThis(aMallocSizeOf);
}

size_t
nsDocument::SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf) const
{
  
  
  
  
  MOZ_NOT_REACHED("nsDocument::SizeOfExcludingThis");
  return 0;
}

void
nsDocument::DocSizeOfExcludingThis(nsWindowSizes* aWindowSizes) const
{
  nsIDocument::DocSizeOfExcludingThis(aWindowSizes);

  for (nsIContent* node = nsINode::GetFirstChild();
       node;
       node = node->GetNextNode(this))
  {
    size_t nodeSize = node->SizeOfIncludingThis(aWindowSizes->mMallocSizeOf);
    size_t* p;

    switch (node->NodeType()) {
    case nsIDOMNode::ELEMENT_NODE:
      p = &aWindowSizes->mDOMElementNodes;
      break;
    case nsIDOMNode::TEXT_NODE:
      p = &aWindowSizes->mDOMTextNodes;
      break;
    case nsIDOMNode::CDATA_SECTION_NODE:
      p = &aWindowSizes->mDOMCDATANodes;
      break;
    case nsIDOMNode::COMMENT_NODE:
      p = &aWindowSizes->mDOMCommentNodes;
      break;
    default:
      p = &aWindowSizes->mDOMOther;
      break;
    }

    *p += nodeSize;
  }

  aWindowSizes->mStyleSheets +=
    mStyleSheets.SizeOfExcludingThis(SizeOfStyleSheetsElementIncludingThis,
                                     aWindowSizes->mMallocSizeOf); 
  aWindowSizes->mDOMOther +=
    mAttrStyleSheet ?
    mAttrStyleSheet->DOMSizeOfIncludingThis(aWindowSizes->mMallocSizeOf) :
    0;

  aWindowSizes->mDOMOther +=
    mStyledLinks.SizeOfExcludingThis(NULL, aWindowSizes->mMallocSizeOf);

  aWindowSizes->mDOMOther +=
    mIdentifierMap.SizeOfExcludingThis(nsIdentifierMapEntry::SizeOfExcludingThis,
                                       aWindowSizes->mMallocSizeOf);

  
  
  
}

bool
MarkDocumentTreeToBeInSyncOperation(nsIDocument* aDoc, void* aData)
{
  nsCOMArray<nsIDocument>* documents =
    static_cast<nsCOMArray<nsIDocument>*>(aData);
  if (aDoc) {
    aDoc->SetIsInSyncOperation(true);
    documents->AppendObject(aDoc);
    aDoc->EnumerateSubDocuments(MarkDocumentTreeToBeInSyncOperation, aData);
  }
  return true;
}

nsAutoSyncOperation::nsAutoSyncOperation(nsIDocument* aDoc)
{
  mMicroTaskLevel = nsContentUtils::MicroTaskLevel();
  nsContentUtils::SetMicroTaskLevel(0);
  if (aDoc) {
    nsPIDOMWindow* win = aDoc->GetWindow();
    if (win) {
      nsCOMPtr<nsIDOMWindow> topWindow;
      win->GetTop(getter_AddRefs(topWindow));
      nsCOMPtr<nsPIDOMWindow> top = do_QueryInterface(topWindow);
      if (top) {                               
        nsCOMPtr<nsIDocument> doc = do_QueryInterface(top->GetExtantDocument());
        MarkDocumentTreeToBeInSyncOperation(doc, &mDocuments);
      }
    }
  }
}

nsAutoSyncOperation::~nsAutoSyncOperation()
{
  for (PRInt32 i = 0; i < mDocuments.Count(); ++i) {
    mDocuments[i]->SetIsInSyncOperation(false);
  }
  nsContentUtils::SetMicroTaskLevel(mMicroTaskLevel);
}
