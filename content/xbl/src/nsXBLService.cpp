








































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
#include "nsGkAtoms.h"
#include "nsIMemory.h"
#include "nsIObserverService.h"
#include "nsIDOMNodeList.h"
#include "nsXBLContentSink.h"
#include "nsXBLBinding.h"
#include "nsXBLPrototypeBinding.h"
#include "nsIXBLDocumentInfo.h"
#include "nsCRT.h"
#include "nsContentUtils.h"
#include "nsSyncLoadService.h"
#include "nsIDOM3Node.h"
#include "nsContentPolicyUtils.h"
#include "nsTArray.h"

#include "nsIPresShell.h"
#include "nsIDocumentObserver.h"
#include "nsFrameManager.h"
#include "nsStyleContext.h"
#include "nsIScriptSecurityManager.h"
#include "nsIScriptError.h"

#ifdef MOZ_XUL
#include "nsXULPrototypeCache.h"
#endif
#include "nsIDOMLoadListener.h"
#include "nsIDOMEventGroup.h"

#define NS_MAX_XBL_BINDING_RECURSION 20

static PRBool IsChromeOrResourceURI(nsIURI* aURI)
{
  PRBool isChrome = PR_FALSE;
  PRBool isResource = PR_FALSE;
  if (NS_SUCCEEDED(aURI->SchemeIs("chrome", &isChrome)) && 
      NS_SUCCEEDED(aURI->SchemeIs("resource", &isResource)))
      return (isChrome || isResource);
  return PR_FALSE;
}

static PRBool
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
    PRBool equal;
    nsresult rv;
    nsCOMPtr<nsIURL> childBindingURL = do_QueryInterface(aChildBindingURI);
    nsCAutoString childRef;
    if (childBindingURL &&
        NS_SUCCEEDED(childBindingURL->GetRef(childRef)) &&
        childRef.IsEmpty()) {
      
      
      
      
      

      
      
      
      
      nsCOMPtr<nsIURI> compareURI;
      rv = binding->PrototypeBinding()->BindingURI()->Clone(getter_AddRefs(compareURI));
      NS_ENSURE_SUCCESS(rv, PR_TRUE); 

      nsCOMPtr<nsIURL> compareURL = do_QueryInterface(compareURI, &rv);
      NS_ENSURE_SUCCESS(rv, PR_TRUE); 
      
      rv = compareURL->SetRef(EmptyCString());
      NS_ENSURE_SUCCESS(rv, PR_TRUE); 

      rv = compareURL->Equals(aChildBindingURI, &equal);
    } else {
      
      rv = binding->PrototypeBinding()->BindingURI()->Equals(aChildBindingURI,
                                                             &equal);
    }

    NS_ENSURE_SUCCESS(rv, PR_TRUE); 

    if (equal) {
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
                                      params, NS_ARRAY_LENGTH(params),
                                      aDocument->GetDocumentURI(),
                                      EmptyString(), 0, 0,
                                      nsIScriptError::warningFlag,
                                      "XBL");
      return PR_TRUE;
    }
  }

  return PR_FALSE;
}

PRBool CheckTagNameWhiteList(PRInt32 aNameSpaceID, nsIAtom *aTagName)
{
  static nsIContent::AttrValuesArray kValidXULTagNames[] =  {
    &nsGkAtoms::autorepeatbutton, &nsGkAtoms::box, &nsGkAtoms::browser,
    &nsGkAtoms::button, &nsGkAtoms::hbox, &nsGkAtoms::image, &nsGkAtoms::menu,
    &nsGkAtoms::menubar, &nsGkAtoms::menuitem, &nsGkAtoms::menupopup,
    &nsGkAtoms::row, &nsGkAtoms::slider, &nsGkAtoms::spacer,
    &nsGkAtoms::splitter, &nsGkAtoms::text, &nsGkAtoms::tree, nsnull};

  PRUint32 i;
  if (aNameSpaceID == kNameSpaceID_XUL) {
    for (i = 0; kValidXULTagNames[i]; ++i) {
      if (aTagName == *(kValidXULTagNames[i])) {
        return PR_TRUE;
      }
    }
  }
#ifdef MOZ_SVG
  else if (aNameSpaceID == kNameSpaceID_SVG &&
           aTagName == nsGkAtoms::generic) {
    return PR_TRUE;
  }
#endif

  return PR_FALSE;
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

    
    PRBool ready = PR_FALSE;
    gXBLService->BindingReady(mBoundElement, mBindingURI, &ready);

    if (!ready)
      return;

    
    
    
    
    
    
    
    
    
    nsIPresShell *shell = doc->GetPrimaryShell();
    if (shell) {
      nsIFrame* childFrame = shell->GetPrimaryFrameFor(mBoundElement);
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




class nsXBLStreamListener : public nsIStreamListener, public nsIDOMLoadListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIREQUESTOBSERVER

  NS_IMETHOD Load(nsIDOMEvent* aEvent);
  NS_IMETHOD BeforeUnload(nsIDOMEvent* aEvent) { return NS_OK; }
  NS_IMETHOD Unload(nsIDOMEvent* aEvent) { return NS_OK; }
  NS_IMETHOD Abort(nsIDOMEvent* aEvent) { return NS_OK; }
  NS_IMETHOD Error(nsIDOMEvent* aEvent) { return NS_OK; }
  NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent) { return NS_OK; }

  nsXBLStreamListener(nsXBLService* aXBLService,
                      nsIDocument* aBoundDocument,
                      nsIXMLContentSink* aSink,
                      nsIDocument* aBindingDocument);
  ~nsXBLStreamListener();

  void AddRequest(nsXBLBindingRequest* aRequest) { mBindingRequests.AppendElement(aRequest); }
  PRBool HasRequest(nsIURI* aURI, nsIContent* aBoundElement);

private:
  nsXBLService* mXBLService; 

  nsCOMPtr<nsIStreamListener> mInner;
  nsAutoTArray<nsXBLBindingRequest*, 8> mBindingRequests;
  
  nsCOMPtr<nsIWeakReference> mBoundDocument;
  nsCOMPtr<nsIXMLContentSink> mSink; 
  nsCOMPtr<nsIDocument> mBindingDocument; 
};


NS_IMPL_ISUPPORTS4(nsXBLStreamListener, nsIStreamListener, nsIRequestObserver, nsIDOMLoadListener, nsIDOMEventListener)

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
                                       PR_TRUE,
                                       sink);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  nsCOMPtr<nsIDOMEventTarget> target(do_QueryInterface(doc));
  target->AddEventListener(NS_LITERAL_STRING("load"), this, PR_FALSE);
  
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

PRBool
nsXBLStreamListener::HasRequest(nsIURI* aURI, nsIContent* aElt)
{
  
  PRUint32 count = mBindingRequests.Length();
  for (PRUint32 i = 0; i < count; i++) {
    nsXBLBindingRequest* req = mBindingRequests.ElementAt(i);
    PRBool eq;
    if (req->mBoundElement == aElt &&
        NS_SUCCEEDED(req->mBindingURI->Equals(aURI, &eq)) && eq)
      return PR_TRUE;
  }

  return PR_FALSE;
}

nsresult
nsXBLStreamListener::Load(nsIDOMEvent* aEvent)
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

    if (!bindingDocument->GetRootContent()) {
      NS_WARNING("*** XBL doc with no root element! Something went horribly wrong! ***");
      return NS_ERROR_FAILURE;
    }

    
    nsBindingManager *xblDocBindingManager = bindingDocument->BindingManager();
    nsCOMPtr<nsIXBLDocumentInfo> info =
      xblDocBindingManager->GetXBLDocumentInfo(documentURI);
    xblDocBindingManager->RemoveXBLDocumentInfo(info); 
    if (!info) {
      NS_ERROR("An XBL file is malformed.  Did you forget the XBL namespace on the bindings tag?");
      return NS_ERROR_FAILURE;
    }

    
#ifdef MOZ_XUL
    if (IsChromeOrResourceURI(documentURI)) {
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

  target->RemoveEventListener(NS_LITERAL_STRING("load"), (nsIDOMLoadListener*)this, PR_FALSE);

  return rv;
}




PRUint32 nsXBLService::gRefCnt = 0;
PRBool nsXBLService::gAllowDataURIs = PR_FALSE;

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
  
  nsContentUtils::AddBoolPrefVarCache("layout.debug.enable_data_xbl",
                                      &gAllowDataURIs);
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



NS_IMETHODIMP
nsXBLService::LoadBindings(nsIContent* aContent, nsIURI* aURL,
                           nsIPrincipal* aOriginPrincipal, PRBool aAugmentFlag,
                           nsXBLBinding** aBinding, PRBool* aResolveStyle) 
{
  NS_PRECONDITION(aOriginPrincipal, "Must have an origin principal");
  
  *aBinding = nsnull;
  *aResolveStyle = PR_FALSE;

  nsresult rv;

  nsCOMPtr<nsIDocument> document = aContent->GetOwnerDoc();

  
  if (!document)
    return NS_OK;

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
        
        nsIURI* uri = styleBinding->PrototypeBinding()->BindingURI();
        PRBool equal;
        if (NS_SUCCEEDED(uri->Equals(aURL, &equal)) && equal)
          return NS_OK;
        FlushStyleBindings(aContent);
        binding = nsnull;
      }
    }
  }

  PRBool ready;
  nsRefPtr<nsXBLBinding> newBinding;
  if (NS_FAILED(rv = GetBinding(aContent, aURL, PR_FALSE, aOriginPrincipal,
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
      baseBinding->SetIsStyleBinding(PR_FALSE);
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

  
  newBinding->SetBoundElement(aContent);

  
  newBinding->GenerateAnonymousContent();

  
  newBinding->InstallEventHandlers();

  
  rv = newBinding->InstallImplementation();
  NS_ENSURE_SUCCESS(rv, rv);

  
  *aResolveStyle = newBinding->HasStyleSheets();
  
  newBinding.swap(*aBinding);

  return NS_OK; 
}

nsresult
nsXBLService::FlushStyleBindings(nsIContent* aContent)
{
  nsCOMPtr<nsIDocument> document = aContent->GetOwnerDoc();

  
  if (! document)
    return NS_OK;

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
  nsIDocument* document = aContent->GetOwnerDoc();
  if (document) {
    *aResult = document->BindingManager()->ResolveTag(aContent, aNameSpaceID);
    NS_IF_ADDREF(*aResult);
  }
  else {
    *aNameSpaceID = aContent->GetNameSpaceID();
    NS_ADDREF(*aResult = aContent->Tag());
  }

  return NS_OK;
}









NS_IMETHODIMP
nsXBLService::AttachGlobalKeyHandler(nsPIDOMEventTarget* aTarget)
{
  
  
  nsCOMPtr<nsPIDOMEventTarget> piTarget = aTarget;
  nsCOMPtr<nsIContent> contentNode(do_QueryInterface(aTarget));
  if (contentNode) {
    
    nsCOMPtr<nsIDocument> doc = contentNode->GetCurrentDoc();
    if (doc)
      piTarget = do_QueryInterface(doc); 
  }
    
  if (!piTarget)
    return NS_ERROR_FAILURE;

  
  if (contentNode && contentNode->GetProperty(nsGkAtoms::listener))
    return NS_OK;
    
  nsCOMPtr<nsIDOMElement> elt(do_QueryInterface(contentNode));

  
  nsXBLWindowKeyHandler* handler;
  NS_NewXBLWindowKeyHandler(elt, piTarget, &handler); 
  if (!handler)
    return NS_ERROR_FAILURE;

  
  nsCOMPtr<nsIDOMEventGroup> systemGroup;
  piTarget->GetSystemEventGroup(getter_AddRefs(systemGroup));
  nsCOMPtr<nsIDOM3EventTarget> target = do_QueryInterface(piTarget);

  target->AddGroupedEventListener(NS_LITERAL_STRING("keydown"), handler,
                                  PR_FALSE, systemGroup);
  target->AddGroupedEventListener(NS_LITERAL_STRING("keyup"), handler, 
                                  PR_FALSE, systemGroup);
  target->AddGroupedEventListener(NS_LITERAL_STRING("keypress"), handler, 
                                  PR_FALSE, systemGroup);

  if (contentNode)
    return contentNode->SetProperty(nsGkAtoms::listener, handler,
                                    nsPropertyTable::SupportsDtorFunc, PR_TRUE);

  
  
  NS_RELEASE(handler);
  return NS_OK;
}






NS_IMETHODIMP
nsXBLService::DetachGlobalKeyHandler(nsPIDOMEventTarget* aTarget)
{
  nsCOMPtr<nsPIDOMEventTarget> piTarget = aTarget;
  nsCOMPtr<nsIContent> contentNode(do_QueryInterface(aTarget));
  if (!contentNode) 
    return NS_ERROR_FAILURE;

  
  nsCOMPtr<nsIDocument> doc = contentNode->GetCurrentDoc();
  if (doc)
    piTarget = do_QueryInterface(doc);
  if (!piTarget)
    return NS_ERROR_FAILURE;

  nsIDOMEventListener* handler =
    static_cast<nsIDOMEventListener*>(contentNode->GetProperty(nsGkAtoms::listener));
  if (!handler)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMEventGroup> systemGroup;
  piTarget->GetSystemEventGroup(getter_AddRefs(systemGroup));
  nsCOMPtr<nsIDOM3EventTarget> target = do_QueryInterface(piTarget);

  target->RemoveGroupedEventListener(NS_LITERAL_STRING("keydown"), handler,
                                     PR_FALSE, systemGroup);
  target->RemoveGroupedEventListener(NS_LITERAL_STRING("keyup"), handler, 
                                     PR_FALSE, systemGroup);
  target->RemoveGroupedEventListener(NS_LITERAL_STRING("keypress"), handler, 
                                     PR_FALSE, systemGroup);

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
                                         PRBool* aIsReady)
{
  
  return GetBinding(aBoundElement, aURI, PR_TRUE, nsnull, aIsReady, nsnull);
}

nsresult
nsXBLService::GetBinding(nsIContent* aBoundElement, nsIURI* aURI, 
                         PRBool aPeekOnly, nsIPrincipal* aOriginPrincipal,
                         PRBool* aIsReady, nsXBLBinding** aResult)
{
  
  nsAutoTArray<nsIURI*, 6> uris;
  return GetBinding(aBoundElement, aURI, aPeekOnly, aOriginPrincipal, aIsReady,
                    aResult, uris);
}

nsresult
nsXBLService::GetBinding(nsIContent* aBoundElement, nsIURI* aURI, 
                         PRBool aPeekOnly, nsIPrincipal* aOriginPrincipal,
                         PRBool* aIsReady, nsXBLBinding** aResult,
                         nsTArray<nsIURI*>& aDontExtendURIs)
{
  NS_ASSERTION(aPeekOnly || aResult,
               "Must have non-null out param if not just peeking to see "
               "whether the binding is ready");
  
  if (aResult)
    *aResult = nsnull;

  if (!aURI)
    return NS_ERROR_FAILURE;

  NS_ENSURE_TRUE(aDontExtendURIs.AppendElement(aURI), NS_ERROR_OUT_OF_MEMORY);

  nsCAutoString ref;
  nsCOMPtr<nsIURL> url(do_QueryInterface(aURI));
  if (url)
    url->GetRef(ref);

  nsCOMPtr<nsIDocument> boundDocument = aBoundElement->GetOwnerDoc();

  nsCOMPtr<nsIXBLDocumentInfo> docInfo;
  nsresult rv = LoadBindingDocumentInfo(aBoundElement, boundDocument, aURI,
                                        aOriginPrincipal,
                                        PR_FALSE, getter_AddRefs(docInfo));
  NS_ENSURE_SUCCESS(rv, rv);
  
  if (!docInfo)
    return NS_ERROR_FAILURE;

  
  nsCOMPtr<nsIDocument> doc;
  docInfo->GetDocument(getter_AddRefs(doc));
  PRBool allowScripts;
  docInfo->GetScriptAccess(&allowScripts);

  nsXBLPrototypeBinding* protoBinding;
  docInfo->GetPrototypeBinding(ref, &protoBinding);

  NS_ASSERTION(protoBinding, "Unable to locate an XBL binding.");
  if (!protoBinding)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIContent> child = protoBinding->GetBindingElement();

  
  PRBool ready = protoBinding->LoadResources();
  if (!ready) {
    
    
    protoBinding->AddResourceListener(aBoundElement);
    return NS_ERROR_FAILURE; 
  }

  
  nsRefPtr<nsXBLBinding> baseBinding;
  PRBool hasBase = protoBinding->HasBasePrototype();
  nsXBLPrototypeBinding* baseProto = protoBinding->GetBasePrototype();
  if (baseProto) {
    
    
    rv = GetBinding(aBoundElement, baseProto->BindingURI(), aPeekOnly,
                    child->NodePrincipal(), aIsReady,
                    getter_AddRefs(baseBinding), aDontExtendURIs);
    if (NS_FAILED(rv))
      return rv; 
  }
  else if (hasBase) {
    
    nsAutoString display, extends;
    child->GetAttr(kNameSpaceID_None, nsGkAtoms::display, display);
    child->GetAttr(kNameSpaceID_None, nsGkAtoms::extends, extends);
    PRBool hasDisplay = !display.IsEmpty();
    PRBool hasExtends = !extends.IsEmpty();
    
    nsAutoString value(extends);
         
    if (!hasExtends) 
      protoBinding->SetHasBasePrototype(PR_FALSE);
    else {
      
      nsAutoString prefix;
      PRInt32 offset;
      if (hasDisplay) {
        offset = display.FindChar(':');
        if (-1 != offset) {
          display.Left(prefix, offset);
          display.Cut(0, offset+1);
        }
      }
      else if (hasExtends) {
        offset = extends.FindChar(':');
        if (-1 != offset) {
          extends.Left(prefix, offset);
          extends.Cut(0, offset+1);
          display = extends;
        }
      }

      nsAutoString nameSpace;

      if (!prefix.IsEmpty()) {
        nsCOMPtr<nsIAtom> prefixAtom = do_GetAtom(prefix);

        nsCOMPtr<nsIDOM3Node> node(do_QueryInterface(child));

        if (node) {
          node->LookupNamespaceURI(prefix, nameSpace);

          if (!nameSpace.IsEmpty()) {
            if (!hasDisplay) {
              
              
              protoBinding->SetHasBasePrototype(PR_FALSE);
              
            }

            PRInt32 nameSpaceID =
              nsContentUtils::NameSpaceManager()->GetNameSpaceID(nameSpace);

            nsCOMPtr<nsIAtom> tagName = do_GetAtom(display);
            
            if (!CheckTagNameWhiteList(nameSpaceID, tagName)) {
              const PRUnichar* params[] = { display.get() };
              nsContentUtils::ReportToConsole(nsContentUtils::eXBL_PROPERTIES,
                                              "InvalidExtendsBinding",
                                              params, NS_ARRAY_LENGTH(params),
                                              doc->GetDocumentURI(),
                                              EmptyString(), 0, 0,
                                              nsIScriptError::errorFlag,
                                              "XBL");
              NS_ASSERTION(!IsChromeOrResourceURI(aURI),
                           "Invalid extends value");
              return NS_ERROR_ILLEGAL_VALUE;
            }

            protoBinding->SetBaseTag(nameSpaceID, tagName);
          }
        }
      }

      if (hasExtends && (hasDisplay || nameSpace.IsEmpty())) {
        
        
        nsCOMPtr<nsIURI> bindingURI;
        rv = NS_NewURI(getter_AddRefs(bindingURI), value,
                       doc->GetDocumentCharacterSet().get(),
                       doc->GetBaseURI());
        NS_ENSURE_SUCCESS(rv, rv);
        
        PRUint32 count = aDontExtendURIs.Length();
        for (PRUint32 index = 0; index < count; ++index) {
          PRBool equal;
          rv = aDontExtendURIs[index]->Equals(bindingURI, &equal);
          NS_ENSURE_SUCCESS(rv, rv);
          if (equal) {
            nsCAutoString spec;
            protoBinding->BindingURI()->GetSpec(spec);
            NS_ConvertUTF8toUTF16 protoSpec(spec);
            const PRUnichar* params[] = { protoSpec.get(), value.get() };
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

        
        
        rv = GetBinding(aBoundElement, bindingURI, aPeekOnly,
                        child->NodePrincipal(), aIsReady,
                        getter_AddRefs(baseBinding), aDontExtendURIs);
        if (NS_FAILED(rv))
          return rv; 
        if (!aPeekOnly) {
          
          baseProto = baseBinding->PrototypeBinding();
          protoBinding->SetBasePrototype(baseProto);
          child->UnsetAttr(kNameSpaceID_None, nsGkAtoms::extends, PR_FALSE);
          child->UnsetAttr(kNameSpaceID_None, nsGkAtoms::display, PR_FALSE);
        }
      }
    }
  }

  *aIsReady = PR_TRUE;
  if (!aPeekOnly) {
    
    nsXBLBinding *newBinding = new nsXBLBinding(protoBinding);
    NS_ENSURE_TRUE(newBinding, NS_ERROR_OUT_OF_MEMORY);

    if (baseBinding)
      newBinding->SetBaseBinding(baseBinding);

    NS_ADDREF(*aResult = newBinding);
  }

  return NS_OK;
}

static PRBool SchemeIs(nsIURI* aURI, const char* aScheme)
{
  nsCOMPtr<nsIURI> baseURI = NS_GetInnermostURI(aURI);
  NS_ENSURE_TRUE(baseURI, PR_FALSE);

  PRBool isScheme = PR_FALSE;
  return NS_SUCCEEDED(baseURI->SchemeIs(aScheme, &isScheme)) && isScheme;
}

NS_IMETHODIMP
nsXBLService::LoadBindingDocumentInfo(nsIContent* aBoundElement,
                                      nsIDocument* aBoundDocument,
                                      nsIURI* aBindingURI,
                                      nsIPrincipal* aOriginPrincipal,
                                      PRBool aForceSyncLoad,
                                      nsIXBLDocumentInfo** aResult)
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
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    PRBool isSystem;
    rv = nsContentUtils::GetSecurityManager()->
      IsSystemPrincipal(aOriginPrincipal, &isSystem);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!isSystem &&
        !(gAllowDataURIs && SchemeIs(aBindingURI, "data")) &&
        !SchemeIs(aBindingURI, "chrome")) {
      rv = aBoundDocument->NodePrincipal()->CheckMayLoad(aBindingURI,
                                                         PR_TRUE);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  *aResult = nsnull;
  nsCOMPtr<nsIXBLDocumentInfo> info;

  nsCOMPtr<nsIURI> documentURI;
  rv = aBindingURI->Clone(getter_AddRefs(documentURI));
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsCOMPtr<nsIURL> documentURL(do_QueryInterface(documentURI));
  if (documentURL)
    documentURL->SetRef(EmptyCString());

#ifdef MOZ_XUL
  
  nsXULPrototypeCache* cache = nsXULPrototypeCache::GetInstance();
  PRBool useXULCache = cache && cache->IsEnabled(); 

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
                   aBoundElement->IsNodeOfType(nsINode::eHTML)))) &&
        !aForceSyncLoad) {
      
      
      
      
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
     
    if (!info) {
      
      
      
      
      PRBool chrome;
      if (NS_SUCCEEDED(documentURI->SchemeIs("chrome", &chrome)) && chrome)
        aForceSyncLoad = PR_TRUE;

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
        if (useXULCache && IsChromeOrResourceURI(documentURI)) {
          cache->PutXBLDocumentInfo(info);
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
                                   PRBool aForceSyncLoad, nsIDocument** aResult)
{
  nsresult rv = NS_OK;
  
  *aResult = nsnull;

  
  
  nsCOMPtr<nsILoadGroup> loadGroup;
  if (aBoundDocument)
    loadGroup = aBoundDocument->GetDocumentLoadGroup();

  
  
  if (IsChromeOrResourceURI(aDocumentURI))
    aForceSyncLoad = PR_TRUE;

  
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
                              PR_TRUE,
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

  
  
  nsCOMPtr<nsIObserverService> os = do_GetService("@mozilla.org/observer-service;1");
  if (os)
    os->AddObserver(result, "memory-pressure", PR_TRUE);

  return NS_OK;
}

