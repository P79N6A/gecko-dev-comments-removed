








































#include "mozilla/Util.h"

#include "nsCOMPtr.h"
#include "nsNetUtil.h"
#include "nsXBLService.h"
#include "nsXBLWindowKeyHandler.h"
#include "nsIInputStream.h"
#include "nsINameSpaceManager.h"
#include "nsHashtable.h"
#include "nsIURI.h"
#include "nsIDOMElement.h"
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
#include "nsIXMLContentSink.h"
#include "nsContentCID.h"
#include "nsXMLDocument.h"
#include "mozilla/FunctionTimer.h"
#include "nsGkAtoms.h"
#include "nsIMemory.h"
#include "nsIObserverService.h"
#include "nsIDOMNodeList.h"
#include "nsXBLContentSink.h"
#include "nsXBLBinding.h"
#include "nsXBLPrototypeBinding.h"
#include "nsXBLDocumentInfo.h"
#include "nsCRT.h"
#include "nsContentUtils.h"
#include "nsSyncLoadService.h"
#include "nsContentPolicyUtils.h"
#include "nsTArray.h"
#include "nsContentErrors.h"

#include "nsIPresShell.h"
#include "nsIDocumentObserver.h"
#include "nsFrameManager.h"
#include "nsStyleContext.h"
#include "nsIScriptSecurityManager.h"
#include "nsIScriptError.h"
#include "nsXBLSerialize.h"

#ifdef MOZ_XUL
#include "nsXULPrototypeCache.h"
#endif
#include "nsIDOMEventListener.h"
#include "mozilla/Preferences.h"
#include "mozilla/dom/Element.h"

using namespace mozilla;

#define NS_MAX_XBL_BINDING_RECURSION 20

static bool
IsAncestorBinding(nsIDocument* aDocument,
                  nsIURI* aChildBindingURI,
                  nsIContent* aChild)
{
  NS_ASSERTION(aDocument, "expected a document");
  NS_ASSERTION(aChildBindingURI, "expected a binding URI");
  NS_ASSERTION(aChild, "expected a child content");

  PRUint32 bindingRecursion = 0;
  nsBindingManager* bindingManager = aDocument->BindingManager();
  for (nsIContent *bindingParent = aChild->GetBindingParent();
       bindingParent;
       bindingParent = bindingParent->GetBindingParent()) {
    nsXBLBinding* binding = bindingManager->GetBinding(bindingParent);
    if (!binding) {
      continue;
    }

    if (binding->PrototypeBinding()->CompareBindingURI(aChildBindingURI)) {
      ++bindingRecursion;
      if (bindingRecursion < NS_MAX_XBL_BINDING_RECURSION) {
        continue;
      }
      nsCAutoString spec;
      aChildBindingURI->GetSpec(spec);
      NS_ConvertUTF8toUTF16 bindingURI(spec);
      const PRUnichar* params[] = { bindingURI.get() };
      nsContentUtils::ReportToConsole(nsContentUtils::eXBL_PROPERTIES,
                                      "TooDeepBindingRecursion",
                                      params, ArrayLength(params),
                                      nsnull,
                                      EmptyString(), 0, 0,
                                      nsIScriptError::warningFlag,
                                      "XBL", aDocument);
      return true;
    }
  }

  return false;
}


class nsXBLBindingRequest
{
public:
  nsCOMPtr<nsIURI> mBindingURI;
  nsCOMPtr<nsIContent> mBoundElement;

  static nsXBLBindingRequest*
  Create(nsFixedSizeAllocator& aPool, nsIURI* aURI, nsIContent* aBoundElement) {
    void* place = aPool.Alloc(sizeof(nsXBLBindingRequest));
    return place ? ::new (place) nsXBLBindingRequest(aURI, aBoundElement) : nsnull;
  }

  static void
  Destroy(nsFixedSizeAllocator& aPool, nsXBLBindingRequest* aRequest) {
    aRequest->~nsXBLBindingRequest();
    aPool.Free(aRequest, sizeof(*aRequest));
  }

  void DocumentLoaded(nsIDocument* aBindingDoc)
  {
    
    
    nsIDocument* doc = mBoundElement->GetCurrentDoc();
    if (!doc)
      return;

    
    bool ready = false;
    gXBLService->BindingReady(mBoundElement, mBindingURI, &ready);

    if (!ready)
      return;

    
    
    
    
    
    
    
    
    
    nsIPresShell *shell = doc->GetShell();
    if (shell) {
      nsIFrame* childFrame = mBoundElement->GetPrimaryFrame();
      if (!childFrame) {
        
        nsStyleContext* sc =
          shell->FrameManager()->GetUndisplayedContent(mBoundElement);

        if (!sc) {
          shell->RecreateFramesFor(mBoundElement);
        }
      }
    }
  }

  static nsIXBLService* gXBLService;
  static int gRefCnt;

protected:
  nsXBLBindingRequest(nsIURI* aURI, nsIContent* aBoundElement)
    : mBindingURI(aURI),
      mBoundElement(aBoundElement)
  {
    gRefCnt++;
    if (gRefCnt == 1) {
      CallGetService("@mozilla.org/xbl;1", &gXBLService);
    }
  }

  ~nsXBLBindingRequest()
  {
    gRefCnt--;
    if (gRefCnt == 0) {
      NS_IF_RELEASE(gXBLService);
    }
  }

private:
  
  
  static void* operator new(size_t) CPP_THROW_NEW { return 0; }
  static void operator delete(void*, size_t) {}
};

static const size_t kBucketSizes[] = {
  sizeof(nsXBLBindingRequest)
};

static const PRInt32 kNumBuckets = sizeof(kBucketSizes)/sizeof(size_t);
static const PRInt32 kNumElements = 64;
static const PRInt32 kInitialSize = (NS_SIZE_IN_HEAP(sizeof(nsXBLBindingRequest))) * kNumElements;

nsIXBLService* nsXBLBindingRequest::gXBLService = nsnull;
int nsXBLBindingRequest::gRefCnt = 0;




class nsXBLStreamListener : public nsIStreamListener, public nsIDOMEventListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSIDOMEVENTLISTENER

  nsXBLStreamListener(nsXBLService* aXBLService,
                      nsIDocument* aBoundDocument,
                      nsIXMLContentSink* aSink,
                      nsIDocument* aBindingDocument);
  ~nsXBLStreamListener();

  void AddRequest(nsXBLBindingRequest* aRequest) { mBindingRequests.AppendElement(aRequest); }
  bool HasRequest(nsIURI* aURI, nsIContent* aBoundElement);

private:
  nsXBLService* mXBLService; 

  nsCOMPtr<nsIStreamListener> mInner;
  nsAutoTArray<nsXBLBindingRequest*, 8> mBindingRequests;
  
  nsCOMPtr<nsIWeakReference> mBoundDocument;
  nsCOMPtr<nsIXMLContentSink> mSink; 
  nsCOMPtr<nsIDocument> mBindingDocument; 
};


NS_IMPL_ISUPPORTS3(nsXBLStreamListener,
                   nsIStreamListener,
                   nsIRequestObserver,
                   nsIDOMEventListener)

nsXBLStreamListener::nsXBLStreamListener(nsXBLService* aXBLService,
                                         nsIDocument* aBoundDocument,
                                         nsIXMLContentSink* aSink,
                                         nsIDocument* aBindingDocument)
: mSink(aSink), mBindingDocument(aBindingDocument)
{
  
  mXBLService = aXBLService;
  mBoundDocument = do_GetWeakReference(aBoundDocument);
}

nsXBLStreamListener::~nsXBLStreamListener()
{
  for (PRUint32 i = 0; i < mBindingRequests.Length(); i++) {
    nsXBLBindingRequest* req = mBindingRequests.ElementAt(i);
    nsXBLBindingRequest::Destroy(mXBLService->mPool, req);
  }
}

NS_IMETHODIMP
nsXBLStreamListener::OnDataAvailable(nsIRequest *request, nsISupports* aCtxt, nsIInputStream* aInStr, 
                                     PRUint32 aSourceOffset, PRUint32 aCount)
{
  if (mInner)
    return mInner->OnDataAvailable(request, aCtxt, aInStr, aSourceOffset, aCount);
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsXBLStreamListener::OnStartRequest(nsIRequest* request, nsISupports* aCtxt)
{
  
  nsCOMPtr<nsIXMLContentSink> sink;
  mSink.swap(sink);
  nsCOMPtr<nsIDocument> doc;
  mBindingDocument.swap(doc);

  nsCOMPtr<nsIChannel> channel = do_QueryInterface(request);
  NS_ENSURE_TRUE(channel, NS_ERROR_UNEXPECTED);

  nsCOMPtr<nsILoadGroup> group;
  request->GetLoadGroup(getter_AddRefs(group));

  nsresult rv = doc->StartDocumentLoad("loadAsInteractiveData",
                                       channel,
                                       group,
                                       nsnull,
                                       getter_AddRefs(mInner),
                                       true,
                                       sink);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  nsCOMPtr<nsIDOMEventTarget> target(do_QueryInterface(doc));
  target->AddEventListener(NS_LITERAL_STRING("load"), this, false);
  
  return mInner->OnStartRequest(request, aCtxt);
}

NS_IMETHODIMP 
nsXBLStreamListener::OnStopRequest(nsIRequest* request, nsISupports* aCtxt, nsresult aStatus)
{
  nsresult rv = NS_OK;
  if (mInner) {
     rv = mInner->OnStopRequest(request, aCtxt, aStatus);
  }

  
  
  mInner = nsnull;

  return rv;
}

bool
nsXBLStreamListener::HasRequest(nsIURI* aURI, nsIContent* aElt)
{
  
  PRUint32 count = mBindingRequests.Length();
  for (PRUint32 i = 0; i < count; i++) {
    nsXBLBindingRequest* req = mBindingRequests.ElementAt(i);
    bool eq;
    if (req->mBoundElement == aElt &&
        NS_SUCCEEDED(req->mBindingURI->Equals(aURI, &eq)) && eq)
      return true;
  }

  return false;
}

nsresult
nsXBLStreamListener::HandleEvent(nsIDOMEvent* aEvent)
{
  nsresult rv = NS_OK;
  PRUint32 i;
  PRUint32 count = mBindingRequests.Length();

  
  
  nsCOMPtr<nsIDOMEventTarget> target;
  aEvent->GetCurrentTarget(getter_AddRefs(target));
  nsCOMPtr<nsIDocument> bindingDocument = do_QueryInterface(target);
  NS_ASSERTION(bindingDocument, "Event not targeted at document?!");

  
  nsCOMPtr<nsIDocument> doc(do_QueryReferent(mBoundDocument));
  if (!doc) {
    NS_WARNING("XBL load did not complete until after document went away! Modal dialog bug?\n");
  }
  else {
    
    
    
    
    
    
    if (count > 0) {
      nsXBLBindingRequest* req = mBindingRequests.ElementAt(0);
      nsIDocument* document = req->mBoundElement->GetCurrentDoc();
      if (document)
        document->FlushPendingNotifications(Flush_ContentAndNotify);
    }

    
    nsBindingManager *bindingManager = doc->BindingManager();
    nsIURI* documentURI = bindingDocument->GetDocumentURI();
    bindingManager->RemoveLoadingDocListener(documentURI);

    if (!bindingDocument->GetRootElement()) {
      
      NS_WARNING("*** XBL doc with no root element! Something went horribly wrong! ***");
      return NS_ERROR_FAILURE;
    }

    
    nsBindingManager *xblDocBindingManager = bindingDocument->BindingManager();
    nsRefPtr<nsXBLDocumentInfo> info =
      xblDocBindingManager->GetXBLDocumentInfo(documentURI);
    xblDocBindingManager->RemoveXBLDocumentInfo(info); 
    if (!info) {
      if (nsXBLService::IsChromeOrResourceURI(documentURI)) {
        NS_WARNING("An XBL file is malformed. Did you forget the XBL namespace on the bindings tag?");
      }
      nsContentUtils::ReportToConsole(nsContentUtils::eXBL_PROPERTIES,
                                      "MalformedXBL",
                                      nsnull, 0, documentURI,
                                      EmptyString(), 0, 0,
                                      nsIScriptError::warningFlag,
                                      "XBL");
      return NS_ERROR_FAILURE;
    }

    
#ifdef MOZ_XUL
    if (nsXBLService::IsChromeOrResourceURI(documentURI)) {
      nsXULPrototypeCache* cache = nsXULPrototypeCache::GetInstance();
      if (cache && cache->IsEnabled())
        cache->PutXBLDocumentInfo(info);
    }
#endif
  
    bindingManager->PutXBLDocumentInfo(info);

    
    
    for (i = 0; i < count; i++) {
      nsXBLBindingRequest* req = mBindingRequests.ElementAt(i);
      req->DocumentLoaded(bindingDocument);
    }
  }

  target->RemoveEventListener(NS_LITERAL_STRING("load"), this, false);

  return rv;
}




PRUint32 nsXBLService::gRefCnt = 0;
bool nsXBLService::gAllowDataURIs = false;

nsHashtable* nsXBLService::gClassTable = nsnull;

JSCList  nsXBLService::gClassLRUList = JS_INIT_STATIC_CLIST(&nsXBLService::gClassLRUList);
PRUint32 nsXBLService::gClassLRUListLength = 0;
PRUint32 nsXBLService::gClassLRUListQuota = 64;


NS_IMPL_ISUPPORTS3(nsXBLService, nsIXBLService, nsIObserver, nsISupportsWeakReference)


nsXBLService::nsXBLService(void)
{
  mPool.Init("XBL Binding Requests", kBucketSizes, kNumBuckets, kInitialSize);

  gRefCnt++;
  if (gRefCnt == 1) {
    gClassTable = new nsHashtable();
  }

  Preferences::AddBoolVarCache(&gAllowDataURIs, "layout.debug.enable_data_xbl");
}

nsXBLService::~nsXBLService(void)
{
  gRefCnt--;
  if (gRefCnt == 0) {
    
    FlushMemory();

    
    
    
    gClassLRUListLength = gClassLRUListQuota = 0;

    
    
    delete gClassTable;
    gClassTable = nsnull;
  }
}


bool
nsXBLService::IsChromeOrResourceURI(nsIURI* aURI)
{
  bool isChrome = false;
  bool isResource = false;
  if (NS_SUCCEEDED(aURI->SchemeIs("chrome", &isChrome)) && 
      NS_SUCCEEDED(aURI->SchemeIs("resource", &isResource)))
      return (isChrome || isResource);
  return false;
}




NS_IMETHODIMP
nsXBLService::LoadBindings(nsIContent* aContent, nsIURI* aURL,
                           nsIPrincipal* aOriginPrincipal, bool aAugmentFlag,
                           nsXBLBinding** aBinding, bool* aResolveStyle) 
{
  NS_PRECONDITION(aOriginPrincipal, "Must have an origin principal");
  
  *aBinding = nsnull;
  *aResolveStyle = false;

  nsresult rv;

  nsCOMPtr<nsIDocument> document = aContent->OwnerDoc();

  nsCAutoString urlspec;
  if (nsContentUtils::GetWrapperSafeScriptFilename(document, aURL, urlspec)) {
    
    

    return NS_OK;
  }

  nsBindingManager *bindingManager = document->BindingManager();
  
  nsXBLBinding *binding = bindingManager->GetBinding(aContent);
  if (binding && !aAugmentFlag) {
    nsXBLBinding *styleBinding = binding->GetFirstStyleBinding();
    if (styleBinding) {
      if (binding->MarkedForDeath()) {
        FlushStyleBindings(aContent);
        binding = nsnull;
      }
      else {
        
        if (styleBinding->PrototypeBinding()->CompareBindingURI(aURL))
          return NS_OK;
        FlushStyleBindings(aContent);
        binding = nsnull;
      }
    }
  }

  bool ready;
  nsRefPtr<nsXBLBinding> newBinding;
  if (NS_FAILED(rv = GetBinding(aContent, aURL, false, aOriginPrincipal,
                                &ready, getter_AddRefs(newBinding)))) {
    return rv;
  }

  if (!newBinding) {
#ifdef DEBUG
    nsCAutoString spec;
    aURL->GetSpec(spec);
    nsCAutoString str(NS_LITERAL_CSTRING("Failed to locate XBL binding. XBL is now using id instead of name to reference bindings. Make sure you have switched over.  The invalid binding name is: ") + spec);
    NS_ERROR(str.get());
#endif
    return NS_OK;
  }

  if (::IsAncestorBinding(document, aURL, aContent)) {
    return NS_ERROR_ILLEGAL_VALUE;
  }

  if (aAugmentFlag) {
    nsXBLBinding *baseBinding;
    nsXBLBinding *nextBinding = newBinding;
    do {
      baseBinding = nextBinding;
      nextBinding = baseBinding->GetBaseBinding();
      baseBinding->SetIsStyleBinding(false);
    } while (nextBinding);

    
    
    
    baseBinding->SetBaseBinding(binding);
    bindingManager->SetBinding(aContent, newBinding);
  }
  else {
    
    if (binding) {
      
      binding->RootBinding()->SetBaseBinding(newBinding);
    }
    else {
      
      bindingManager->SetBinding(aContent, newBinding);
    }
  }

  {
    nsAutoScriptBlocker scriptBlocker;

    
    newBinding->SetBoundElement(aContent);

    
    newBinding->GenerateAnonymousContent();

    
    newBinding->InstallEventHandlers();

    
    rv = newBinding->InstallImplementation();
    NS_ENSURE_SUCCESS(rv, rv);

    
    *aResolveStyle = newBinding->HasStyleSheets();
  
    newBinding.swap(*aBinding);
  }

  return NS_OK; 
}

nsresult
nsXBLService::FlushStyleBindings(nsIContent* aContent)
{
  nsCOMPtr<nsIDocument> document = aContent->OwnerDoc();

  nsBindingManager *bindingManager = document->BindingManager();
  
  nsXBLBinding *binding = bindingManager->GetBinding(aContent);
  
  if (binding) {
    nsXBLBinding *styleBinding = binding->GetFirstStyleBinding();

    if (styleBinding) {
      
      styleBinding->ChangeDocument(document, nsnull);
    }

    if (styleBinding == binding) 
      bindingManager->SetBinding(aContent, nsnull); 
  }
   
  return NS_OK;
}

NS_IMETHODIMP
nsXBLService::ResolveTag(nsIContent* aContent, PRInt32* aNameSpaceID,
                         nsIAtom** aResult)
{
  nsIDocument* document = aContent->OwnerDoc();
  *aResult = document->BindingManager()->ResolveTag(aContent, aNameSpaceID);
  NS_IF_ADDREF(*aResult);

  return NS_OK;
}









NS_IMETHODIMP
nsXBLService::AttachGlobalKeyHandler(nsIDOMEventTarget* aTarget)
{
  
  
  nsCOMPtr<nsIDOMEventTarget> piTarget = aTarget;
  nsCOMPtr<nsIContent> contentNode(do_QueryInterface(aTarget));
  if (contentNode) {
    
    nsCOMPtr<nsIDocument> doc = contentNode->GetCurrentDoc();
    if (doc)
      piTarget = doc; 
  }

  nsEventListenerManager* manager = piTarget->GetListenerManager(true);
    
  if (!piTarget || !manager)
    return NS_ERROR_FAILURE;

  
  if (contentNode && contentNode->GetProperty(nsGkAtoms::listener))
    return NS_OK;
    
  nsCOMPtr<nsIDOMElement> elt(do_QueryInterface(contentNode));

  
  nsXBLWindowKeyHandler* handler;
  NS_NewXBLWindowKeyHandler(elt, piTarget, &handler); 
  if (!handler)
    return NS_ERROR_FAILURE;

  
  manager->AddEventListenerByType(handler, NS_LITERAL_STRING("keydown"),
                                  NS_EVENT_FLAG_BUBBLE |
                                  NS_EVENT_FLAG_SYSTEM_EVENT);
  manager->AddEventListenerByType(handler, NS_LITERAL_STRING("keyup"),
                                  NS_EVENT_FLAG_BUBBLE |
                                  NS_EVENT_FLAG_SYSTEM_EVENT);
  manager->AddEventListenerByType(handler, NS_LITERAL_STRING("keypress"),
                                  NS_EVENT_FLAG_BUBBLE |
                                  NS_EVENT_FLAG_SYSTEM_EVENT);

  if (contentNode)
    return contentNode->SetProperty(nsGkAtoms::listener, handler,
                                    nsPropertyTable::SupportsDtorFunc, true);

  
  
  NS_RELEASE(handler);
  return NS_OK;
}






NS_IMETHODIMP
nsXBLService::DetachGlobalKeyHandler(nsIDOMEventTarget* aTarget)
{
  nsCOMPtr<nsIDOMEventTarget> piTarget = aTarget;
  nsCOMPtr<nsIContent> contentNode(do_QueryInterface(aTarget));
  if (!contentNode) 
    return NS_ERROR_FAILURE;

  
  nsCOMPtr<nsIDocument> doc = contentNode->GetCurrentDoc();
  if (doc)
    piTarget = do_QueryInterface(doc);

  nsEventListenerManager* manager = piTarget->GetListenerManager(true);
    
  if (!piTarget || !manager)
    return NS_ERROR_FAILURE;

  nsIDOMEventListener* handler =
    static_cast<nsIDOMEventListener*>(contentNode->GetProperty(nsGkAtoms::listener));
  if (!handler)
    return NS_ERROR_FAILURE;

  manager->RemoveEventListenerByType(handler, NS_LITERAL_STRING("keydown"),
                                     NS_EVENT_FLAG_BUBBLE |
                                     NS_EVENT_FLAG_SYSTEM_EVENT);
  manager->RemoveEventListenerByType(handler, NS_LITERAL_STRING("keyup"),
                                     NS_EVENT_FLAG_BUBBLE |
                                     NS_EVENT_FLAG_SYSTEM_EVENT);
  manager->RemoveEventListenerByType(handler, NS_LITERAL_STRING("keypress"),
                                     NS_EVENT_FLAG_BUBBLE |
                                     NS_EVENT_FLAG_SYSTEM_EVENT);

  contentNode->DeleteProperty(nsGkAtoms::listener);

  return NS_OK;
}

NS_IMETHODIMP
nsXBLService::Observe(nsISupports* aSubject, const char* aTopic, const PRUnichar* aSomeData)
{
  if (nsCRT::strcmp(aTopic, "memory-pressure") == 0)
    FlushMemory();

  return NS_OK;
}

nsresult
nsXBLService::FlushMemory()
{
  while (!JS_CLIST_IS_EMPTY(&gClassLRUList)) {
    JSCList* lru = gClassLRUList.next;
    nsXBLJSClass* c = static_cast<nsXBLJSClass*>(lru);

    JS_REMOVE_AND_INIT_LINK(lru);
    delete c;
    gClassLRUListLength--;
  }
  return NS_OK;
}



NS_IMETHODIMP nsXBLService::BindingReady(nsIContent* aBoundElement, 
                                         nsIURI* aURI, 
                                         bool* aIsReady)
{
  
  return GetBinding(aBoundElement, aURI, true, nsnull, aIsReady, nsnull);
}

nsresult
nsXBLService::GetBinding(nsIContent* aBoundElement, nsIURI* aURI, 
                         bool aPeekOnly, nsIPrincipal* aOriginPrincipal,
                         bool* aIsReady, nsXBLBinding** aResult)
{
  
  nsAutoTArray<nsIURI*, 6> uris;
  return GetBinding(aBoundElement, aURI, aPeekOnly, aOriginPrincipal, aIsReady,
                    aResult, uris);
}

nsresult
nsXBLService::GetBinding(nsIContent* aBoundElement, nsIURI* aURI, 
                         bool aPeekOnly, nsIPrincipal* aOriginPrincipal,
                         bool* aIsReady, nsXBLBinding** aResult,
                         nsTArray<nsIURI*>& aDontExtendURIs)
{
  NS_ASSERTION(aPeekOnly || aResult,
               "Must have non-null out param if not just peeking to see "
               "whether the binding is ready");
  
  if (aResult)
    *aResult = nsnull;

  if (!aURI)
    return NS_ERROR_FAILURE;

  nsCAutoString ref;
  aURI->GetRef(ref);

  nsCOMPtr<nsIDocument> boundDocument = aBoundElement->OwnerDoc();

  nsRefPtr<nsXBLDocumentInfo> docInfo;
  nsresult rv = LoadBindingDocumentInfo(aBoundElement, boundDocument, aURI,
                                        aOriginPrincipal,
                                        false, getter_AddRefs(docInfo));
  NS_ENSURE_SUCCESS(rv, rv);
  
  if (!docInfo)
    return NS_ERROR_FAILURE;

  nsXBLPrototypeBinding* protoBinding = docInfo->GetPrototypeBinding(ref);

  NS_WARN_IF_FALSE(protoBinding, "Unable to locate an XBL binding");
  if (!protoBinding)
    return NS_ERROR_FAILURE;

  NS_ENSURE_TRUE(aDontExtendURIs.AppendElement(protoBinding->BindingURI()),
                 NS_ERROR_OUT_OF_MEMORY);
  nsCOMPtr<nsIURI> altBindingURI = protoBinding->AlternateBindingURI();
  if (altBindingURI) {
    NS_ENSURE_TRUE(aDontExtendURIs.AppendElement(altBindingURI),
                   NS_ERROR_OUT_OF_MEMORY);
  }

  
  bool ready = protoBinding->LoadResources();
  if (!ready) {
    
    
    protoBinding->AddResourceListener(aBoundElement);
    return NS_ERROR_FAILURE; 
  }

  rv = protoBinding->ResolveBaseBinding();
  NS_ENSURE_SUCCESS(rv, rv);

  nsIURI* baseBindingURI;
  nsXBLPrototypeBinding* baseProto = protoBinding->GetBasePrototype();
  if (baseProto) {
    baseBindingURI = baseProto->BindingURI();
  }
  else {
    baseBindingURI = protoBinding->GetBaseBindingURI();
    if (baseBindingURI) {
      PRUint32 count = aDontExtendURIs.Length();
      for (PRUint32 index = 0; index < count; ++index) {
        bool equal;
        rv = aDontExtendURIs[index]->Equals(baseBindingURI, &equal);
        NS_ENSURE_SUCCESS(rv, rv);
        if (equal) {
          nsCAutoString spec, basespec;
          protoBinding->BindingURI()->GetSpec(spec);
          NS_ConvertUTF8toUTF16 protoSpec(spec);
          baseBindingURI->GetSpec(basespec);
          NS_ConvertUTF8toUTF16 baseSpecUTF16(basespec);
          const PRUnichar* params[] = { protoSpec.get(), baseSpecUTF16.get() };
          nsContentUtils::ReportToConsole(nsContentUtils::eXBL_PROPERTIES,
                                          "CircularExtendsBinding",
                                          params, NS_ARRAY_LENGTH(params),
                                          boundDocument->GetDocumentURI(),
                                          EmptyString(), 0, 0,
                                          nsIScriptError::warningFlag,
                                          "XBL");
          return NS_ERROR_ILLEGAL_VALUE;
        }
      }
    }
  }

  nsRefPtr<nsXBLBinding> baseBinding;
  if (baseBindingURI) {
    nsCOMPtr<nsIContent> child = protoBinding->GetBindingElement();
    rv = GetBinding(aBoundElement, baseBindingURI, aPeekOnly,
                    child->NodePrincipal(), aIsReady,
                    getter_AddRefs(baseBinding), aDontExtendURIs);
    if (NS_FAILED(rv))
      return rv; 
  }

  *aIsReady = true;

  if (!aPeekOnly) {
    
    nsXBLBinding *newBinding = new nsXBLBinding(protoBinding);
    NS_ENSURE_TRUE(newBinding, NS_ERROR_OUT_OF_MEMORY);

    if (baseBinding) {
      if (!baseProto) {
        protoBinding->SetBasePrototype(baseBinding->PrototypeBinding());
      }
       newBinding->SetBaseBinding(baseBinding);
    }

    NS_ADDREF(*aResult = newBinding);
  }

  return NS_OK;
}

static bool SchemeIs(nsIURI* aURI, const char* aScheme)
{
  nsCOMPtr<nsIURI> baseURI = NS_GetInnermostURI(aURI);
  NS_ENSURE_TRUE(baseURI, false);

  bool isScheme = false;
  return NS_SUCCEEDED(baseURI->SchemeIs(aScheme, &isScheme)) && isScheme;
}

static bool
IsSystemOrChromeURLPrincipal(nsIPrincipal* aPrincipal)
{
  if (nsContentUtils::IsSystemPrincipal(aPrincipal)) {
    return true;
  }
  
  nsCOMPtr<nsIURI> uri;
  aPrincipal->GetURI(getter_AddRefs(uri));
  NS_ENSURE_TRUE(uri, false);
  
  bool isChrome = false;
  return NS_SUCCEEDED(uri->SchemeIs("chrome", &isChrome)) && isChrome;
}

NS_IMETHODIMP
nsXBLService::LoadBindingDocumentInfo(nsIContent* aBoundElement,
                                      nsIDocument* aBoundDocument,
                                      nsIURI* aBindingURI,
                                      nsIPrincipal* aOriginPrincipal,
                                      bool aForceSyncLoad,
                                      nsXBLDocumentInfo** aResult)
{
  NS_PRECONDITION(aBindingURI, "Must have a binding URI");
  NS_PRECONDITION(!aOriginPrincipal || aBoundDocument,
                  "If we're doing a security check, we better have a document!");
  
  nsresult rv;
  if (aOriginPrincipal) {
    
    
    
    
    
    
    
    
    rv = nsContentUtils::
      CheckSecurityBeforeLoad(aBindingURI, aOriginPrincipal,
                              nsIScriptSecurityManager::ALLOW_CHROME,
                              gAllowDataURIs,
                              nsIContentPolicy::TYPE_XBL,
                              aBoundDocument);
    NS_ENSURE_SUCCESS(rv, NS_ERROR_XBL_BLOCKED);

    if (!IsSystemOrChromeURLPrincipal(aOriginPrincipal)) {
      
      
      if (!(gAllowDataURIs && SchemeIs(aBindingURI, "data")) &&
          !SchemeIs(aBindingURI, "chrome")) {
        rv = aBoundDocument->NodePrincipal()->CheckMayLoad(aBindingURI,
                                                           true);
        NS_ENSURE_SUCCESS(rv, NS_ERROR_XBL_BLOCKED);
      }

      
      NS_ENSURE_TRUE(aBoundDocument->AllowXULXBL(),
                     NS_ERROR_XBL_BLOCKED);
    }
  }

  *aResult = nsnull;
  nsRefPtr<nsXBLDocumentInfo> info;

  nsCOMPtr<nsIURI> documentURI;
  rv = aBindingURI->CloneIgnoringRef(getter_AddRefs(documentURI));
  NS_ENSURE_SUCCESS(rv, rv);

#ifdef MOZ_XUL
  
  nsXULPrototypeCache* cache = nsXULPrototypeCache::GetInstance();
  bool useXULCache = cache && cache->IsEnabled(); 

  if (useXULCache) {
    
    
    
    info = cache->GetXBLDocumentInfo(documentURI); 
  }
#endif

  if (!info) {
    
    nsBindingManager *bindingManager = nsnull;

    if (aBoundDocument) {
      bindingManager = aBoundDocument->BindingManager();
      info = bindingManager->GetXBLDocumentInfo(documentURI);
    }

    nsINodeInfo *ni = nsnull;
    if (aBoundElement)
      ni = aBoundElement->NodeInfo();

    if (!info && bindingManager &&
        (!ni || !(ni->Equals(nsGkAtoms::scrollbar, kNameSpaceID_XUL) ||
                  ni->Equals(nsGkAtoms::thumb, kNameSpaceID_XUL) ||
                  ((ni->Equals(nsGkAtoms::input) ||
                    ni->Equals(nsGkAtoms::select)) &&
                   aBoundElement->IsHTML()))) && !aForceSyncLoad) {
      
      
      
      
      nsCOMPtr<nsIStreamListener> listener;
      if (bindingManager)
        listener = bindingManager->GetLoadingDocListener(documentURI);
      if (listener) {
        nsXBLStreamListener* xblListener =
          static_cast<nsXBLStreamListener*>(listener.get());
        
        if (!xblListener->HasRequest(aBindingURI, aBoundElement)) {
          nsXBLBindingRequest* req = nsXBLBindingRequest::Create(mPool, aBindingURI, aBoundElement);
          xblListener->AddRequest(req);
        }
        return NS_OK;
      }
    }

#ifdef MOZ_XUL
    
    bool useStartupCache = useXULCache && IsChromeOrResourceURI(documentURI);
    if (!info && useStartupCache) {
      rv = nsXBLDocumentInfo::ReadPrototypeBindings(documentURI, getter_AddRefs(info));
      if (NS_SUCCEEDED(rv)) {
        cache->PutXBLDocumentInfo(info);

        if (bindingManager) {
          
          bindingManager->PutXBLDocumentInfo(info);
        }
      }
    }
#endif

    if (!info) {
      
      
      
      
      bool chrome;
      if (NS_SUCCEEDED(documentURI->SchemeIs("chrome", &chrome)) && chrome)
        aForceSyncLoad = true;

      nsCOMPtr<nsIDocument> document;
      FetchBindingDocument(aBoundElement, aBoundDocument, documentURI,
                           aBindingURI, aForceSyncLoad, getter_AddRefs(document));

      if (document) {
        nsBindingManager *xblDocBindingManager = document->BindingManager();
        info = xblDocBindingManager->GetXBLDocumentInfo(documentURI);
        if (!info) {
          NS_ERROR("An XBL file is malformed.  Did you forget the XBL namespace on the bindings tag?");
          return NS_ERROR_FAILURE;
        }
        xblDocBindingManager->RemoveXBLDocumentInfo(info); 

        
#ifdef MOZ_XUL
        if (useStartupCache) {
          cache->PutXBLDocumentInfo(info);

          
          info->WritePrototypeBindings();
        }
#endif
        
        if (bindingManager) {
          
          bindingManager->PutXBLDocumentInfo(info);
        }
      }
    }
  }

  if (!info)
    return NS_OK;
 
  *aResult = info;
  NS_IF_ADDREF(*aResult);

  return NS_OK;
}

nsresult
nsXBLService::FetchBindingDocument(nsIContent* aBoundElement, nsIDocument* aBoundDocument,
                                   nsIURI* aDocumentURI, nsIURI* aBindingURI, 
                                   bool aForceSyncLoad, nsIDocument** aResult)
{
  NS_TIME_FUNCTION;

  nsresult rv = NS_OK;
  
  *aResult = nsnull;

  
  
  nsCOMPtr<nsILoadGroup> loadGroup;
  if (aBoundDocument)
    loadGroup = aBoundDocument->GetDocumentLoadGroup();

  
  
  if (IsChromeOrResourceURI(aDocumentURI))
    aForceSyncLoad = true;

  
  nsCOMPtr<nsIDocument> doc;
  rv = NS_NewXMLDocument(getter_AddRefs(doc));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIXMLContentSink> xblSink;
  rv = NS_NewXBLContentSink(getter_AddRefs(xblSink), doc, aDocumentURI, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIChannel> channel;
  rv = NS_NewChannel(getter_AddRefs(channel), aDocumentURI, nsnull, loadGroup);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIInterfaceRequestor> sameOriginChecker = nsContentUtils::GetSameOriginChecker();
  NS_ENSURE_TRUE(sameOriginChecker, NS_ERROR_OUT_OF_MEMORY);

  channel->SetNotificationCallbacks(sameOriginChecker);

  if (!aForceSyncLoad) {
    
    nsXBLStreamListener* xblListener =
      new nsXBLStreamListener(this, aBoundDocument, xblSink, doc);
    NS_ENSURE_TRUE(xblListener,NS_ERROR_OUT_OF_MEMORY);

    
    nsBindingManager *bindingManager;
    if (aBoundDocument)
      bindingManager = aBoundDocument->BindingManager();
    else
      bindingManager = nsnull;

    if (bindingManager)
      bindingManager->PutLoadingDocListener(aDocumentURI, xblListener);

    
    nsXBLBindingRequest* req = nsXBLBindingRequest::Create(mPool,
                                                           aBindingURI,
                                                           aBoundElement);
    xblListener->AddRequest(req);

    
    rv = channel->AsyncOpen(xblListener, nsnull);
    if (NS_FAILED(rv)) {
      
      if (bindingManager) {
        bindingManager->RemoveLoadingDocListener(aDocumentURI);
      }
    }
    return NS_OK;
  }

  nsCOMPtr<nsIStreamListener> listener;
  rv = doc->StartDocumentLoad("loadAsInteractiveData",
                              channel,
                              loadGroup,
                              nsnull,
                              getter_AddRefs(listener),
                              true,
                              xblSink);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIInputStream> in;
  rv = channel->Open(getter_AddRefs(in));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = nsSyncLoadService::PushSyncStreamToListener(in, listener, channel);
  NS_ENSURE_SUCCESS(rv, rv);

  doc.swap(*aResult);

  return NS_OK;
}



nsresult NS_NewXBLService(nsIXBLService** aResult);

nsresult
NS_NewXBLService(nsIXBLService** aResult)
{
  nsXBLService* result = new nsXBLService;
  if (! result)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aResult = result);

  
  
  nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
  if (os)
    os->AddObserver(result, "memory-pressure", true);

  return NS_OK;
}

