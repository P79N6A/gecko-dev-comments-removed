




#include "mozilla/ArrayUtils.h"

#include "nsCOMPtr.h"
#include "nsNetUtil.h"
#include "nsXBLService.h"
#include "nsXBLWindowKeyHandler.h"
#include "nsIInputStream.h"
#include "nsNameSpaceManager.h"
#include "nsIURI.h"
#include "nsIDOMElement.h"
#include "nsIURL.h"
#include "nsIChannel.h"
#include "nsXPIDLString.h"
#include "plstr.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIXMLContentSink.h"
#include "nsContentCID.h"
#include "mozilla/dom/XMLDocument.h"
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
#include "nsError.h"

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
#include "mozilla/Attributes.h"
#include "mozilla/EventListenerManager.h"
#include "mozilla/Preferences.h"
#include "mozilla/dom/Event.h"
#include "mozilla/dom/Element.h"

using namespace mozilla;
using namespace mozilla::dom;

#define NS_MAX_XBL_BINDING_RECURSION 20

nsXBLService* nsXBLService::gInstance = nullptr;

static bool
IsAncestorBinding(nsIDocument* aDocument,
                  nsIURI* aChildBindingURI,
                  nsIContent* aChild)
{
  NS_ASSERTION(aDocument, "expected a document");
  NS_ASSERTION(aChildBindingURI, "expected a binding URI");
  NS_ASSERTION(aChild, "expected a child content");

  uint32_t bindingRecursion = 0;
  for (nsIContent *bindingParent = aChild->GetBindingParent();
       bindingParent;
       bindingParent = bindingParent->GetBindingParent()) {
    nsXBLBinding* binding = bindingParent->GetXBLBinding();
    if (!binding) {
      continue;
    }

    if (binding->PrototypeBinding()->CompareBindingURI(aChildBindingURI)) {
      ++bindingRecursion;
      if (bindingRecursion < NS_MAX_XBL_BINDING_RECURSION) {
        continue;
      }
      nsAutoCString spec;
      aChildBindingURI->GetSpec(spec);
      NS_ConvertUTF8toUTF16 bindingURI(spec);
      const char16_t* params[] = { bindingURI.get() };
      nsContentUtils::ReportToConsole(nsIScriptError::warningFlag,
                                      NS_LITERAL_CSTRING("XBL"), aDocument,
                                      nsContentUtils::eXBL_PROPERTIES,
                                      "TooDeepBindingRecursion",
                                      params, ArrayLength(params));
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

  void DocumentLoaded(nsIDocument* aBindingDoc)
  {
    
    
    nsIDocument* doc = mBoundElement->GetCurrentDoc();
    if (!doc)
      return;

    
    bool ready = false;
    nsXBLService::GetInstance()->BindingReady(mBoundElement, mBindingURI, &ready);
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

  nsXBLBindingRequest(nsIURI* aURI, nsIContent* aBoundElement)
    : mBindingURI(aURI),
      mBoundElement(aBoundElement)
  {
  }
};




class nsXBLStreamListener MOZ_FINAL : public nsIStreamListener,
                                      public nsIDOMEventListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSIDOMEVENTLISTENER

  nsXBLStreamListener(nsIDocument* aBoundDocument,
                      nsIXMLContentSink* aSink,
                      nsIDocument* aBindingDocument);

  void AddRequest(nsXBLBindingRequest* aRequest) { mBindingRequests.AppendElement(aRequest); }
  bool HasRequest(nsIURI* aURI, nsIContent* aBoundElement);

private:
  ~nsXBLStreamListener();

  nsCOMPtr<nsIStreamListener> mInner;
  nsAutoTArray<nsXBLBindingRequest*, 8> mBindingRequests;

  nsCOMPtr<nsIWeakReference> mBoundDocument;
  nsCOMPtr<nsIXMLContentSink> mSink; 
  nsCOMPtr<nsIDocument> mBindingDocument; 
};


NS_IMPL_ISUPPORTS(nsXBLStreamListener,
                  nsIStreamListener,
                  nsIRequestObserver,
                  nsIDOMEventListener)

nsXBLStreamListener::nsXBLStreamListener(nsIDocument* aBoundDocument,
                                         nsIXMLContentSink* aSink,
                                         nsIDocument* aBindingDocument)
: mSink(aSink), mBindingDocument(aBindingDocument)
{
  
  mBoundDocument = do_GetWeakReference(aBoundDocument);
}

nsXBLStreamListener::~nsXBLStreamListener()
{
  for (uint32_t i = 0; i < mBindingRequests.Length(); i++) {
    nsXBLBindingRequest* req = mBindingRequests.ElementAt(i);
    delete req;
  }
}

NS_IMETHODIMP
nsXBLStreamListener::OnDataAvailable(nsIRequest *request, nsISupports* aCtxt,
                                     nsIInputStream* aInStr,
                                     uint64_t aSourceOffset, uint32_t aCount)
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
                                       nullptr,
                                       getter_AddRefs(mInner),
                                       true,
                                       sink);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  doc->AddEventListener(NS_LITERAL_STRING("load"), this, false);

  return mInner->OnStartRequest(request, aCtxt);
}

NS_IMETHODIMP
nsXBLStreamListener::OnStopRequest(nsIRequest* request, nsISupports* aCtxt, nsresult aStatus)
{
  nsresult rv = NS_OK;
  if (mInner) {
     rv = mInner->OnStopRequest(request, aCtxt, aStatus);
  }

  
  
  mInner = nullptr;

  return rv;
}

bool
nsXBLStreamListener::HasRequest(nsIURI* aURI, nsIContent* aElt)
{
  
  uint32_t count = mBindingRequests.Length();
  for (uint32_t i = 0; i < count; i++) {
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
  uint32_t i;
  uint32_t count = mBindingRequests.Length();

  
  
  Event* event = aEvent->InternalDOMEvent();
  EventTarget* target = event->GetCurrentTarget();
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
      
      NS_WARNING("XBL doc with no root element - this usually shouldn't happen");
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
      nsContentUtils::ReportToConsole(nsIScriptError::warningFlag,
                                      NS_LITERAL_CSTRING("XBL"), nullptr,
                                      nsContentUtils::eXBL_PROPERTIES,
                                      "MalformedXBL",
                                      nullptr, 0, documentURI);
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




bool nsXBLService::gAllowDataURIs = false;


NS_IMPL_ISUPPORTS(nsXBLService, nsISupportsWeakReference)

void
nsXBLService::Init()
{
  gInstance = new nsXBLService();
  NS_ADDREF(gInstance);
}


nsXBLService::nsXBLService(void)
{
  Preferences::AddBoolVarCache(&gAllowDataURIs, "layout.debug.enable_data_xbl");
}

nsXBLService::~nsXBLService(void)
{
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




nsresult
nsXBLService::LoadBindings(nsIContent* aContent, nsIURI* aURL,
                           nsIPrincipal* aOriginPrincipal,
                           nsXBLBinding** aBinding, bool* aResolveStyle)
{
  NS_PRECONDITION(aOriginPrincipal, "Must have an origin principal");

  *aBinding = nullptr;
  *aResolveStyle = false;

  nsresult rv;

  nsCOMPtr<nsIDocument> document = aContent->OwnerDoc();

  nsAutoCString urlspec;
  if (nsContentUtils::GetWrapperSafeScriptFilename(document, aURL, urlspec)) {
    
    

    return NS_OK;
  }

  nsXBLBinding *binding = aContent->GetXBLBinding();
  if (binding) {
    if (binding->MarkedForDeath()) {
      FlushStyleBindings(aContent);
      binding = nullptr;
    }
    else {
      
      if (binding->PrototypeBinding()->CompareBindingURI(aURL))
        return NS_OK;
      FlushStyleBindings(aContent);
      binding = nullptr;
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
    nsAutoCString spec;
    aURL->GetSpec(spec);
    nsAutoCString str(NS_LITERAL_CSTRING("Failed to locate XBL binding. XBL is now using id instead of name to reference bindings. Make sure you have switched over.  The invalid binding name is: ") + spec);
    NS_ERROR(str.get());
#endif
    return NS_OK;
  }

  if (::IsAncestorBinding(document, aURL, aContent)) {
    return NS_ERROR_ILLEGAL_VALUE;
  }

  
  if (binding) {
    
    binding->RootBinding()->SetBaseBinding(newBinding);
  }
  else {
    
    aContent->SetXBLBinding(newBinding);
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

  nsXBLBinding *binding = aContent->GetXBLBinding();
  if (binding) {
    
    binding->ChangeDocument(document, nullptr);

    aContent->SetXBLBinding(nullptr); 
  }

  return NS_OK;
}








nsresult
nsXBLService::AttachGlobalKeyHandler(EventTarget* aTarget)
{
  
  
  nsCOMPtr<EventTarget> piTarget = aTarget;
  nsCOMPtr<nsIContent> contentNode(do_QueryInterface(aTarget));
  if (contentNode) {
    
    nsCOMPtr<nsIDocument> doc = contentNode->GetCurrentDoc();
    if (doc)
      piTarget = doc; 
  }

  EventListenerManager* manager = piTarget->GetOrCreateListenerManager();

  if (!piTarget || !manager)
    return NS_ERROR_FAILURE;

  
  if (contentNode && contentNode->GetProperty(nsGkAtoms::listener))
    return NS_OK;

  nsCOMPtr<nsIDOMElement> elt(do_QueryInterface(contentNode));

  
  nsRefPtr<nsXBLWindowKeyHandler> handler =
    NS_NewXBLWindowKeyHandler(elt, piTarget);

  
  manager->AddEventListenerByType(handler, NS_LITERAL_STRING("keydown"),
                                  TrustedEventsAtSystemGroupBubble());
  manager->AddEventListenerByType(handler, NS_LITERAL_STRING("keyup"),
                                  TrustedEventsAtSystemGroupBubble());
  manager->AddEventListenerByType(handler, NS_LITERAL_STRING("keypress"),
                                  TrustedEventsAtSystemGroupBubble());

  
  
  manager->AddEventListenerByType(handler, NS_LITERAL_STRING("keydown"),
                                  TrustedEventsAtSystemGroupCapture());
  manager->AddEventListenerByType(handler, NS_LITERAL_STRING("keyup"),
                                  TrustedEventsAtSystemGroupCapture());
  manager->AddEventListenerByType(handler, NS_LITERAL_STRING("keypress"),
                                  TrustedEventsAtSystemGroupCapture());

  if (contentNode)
    return contentNode->SetProperty(nsGkAtoms::listener,
                                    handler.forget().take(),
                                    nsPropertyTable::SupportsDtorFunc, true);

  
  
  return NS_OK;
}






nsresult
nsXBLService::DetachGlobalKeyHandler(EventTarget* aTarget)
{
  nsCOMPtr<EventTarget> piTarget = aTarget;
  nsCOMPtr<nsIContent> contentNode(do_QueryInterface(aTarget));
  if (!contentNode) 
    return NS_ERROR_FAILURE;

  
  nsCOMPtr<nsIDocument> doc = contentNode->GetCurrentDoc();
  if (doc)
    piTarget = do_QueryInterface(doc);

  EventListenerManager* manager = piTarget->GetOrCreateListenerManager();

  if (!piTarget || !manager)
    return NS_ERROR_FAILURE;

  nsIDOMEventListener* handler =
    static_cast<nsIDOMEventListener*>(contentNode->GetProperty(nsGkAtoms::listener));
  if (!handler)
    return NS_ERROR_FAILURE;

  manager->RemoveEventListenerByType(handler, NS_LITERAL_STRING("keydown"),
                                     TrustedEventsAtSystemGroupBubble());
  manager->RemoveEventListenerByType(handler, NS_LITERAL_STRING("keyup"),
                                     TrustedEventsAtSystemGroupBubble());
  manager->RemoveEventListenerByType(handler, NS_LITERAL_STRING("keypress"),
                                     TrustedEventsAtSystemGroupBubble());

  manager->RemoveEventListenerByType(handler, NS_LITERAL_STRING("keydown"),
                                     TrustedEventsAtSystemGroupCapture());
  manager->RemoveEventListenerByType(handler, NS_LITERAL_STRING("keyup"),
                                     TrustedEventsAtSystemGroupCapture());
  manager->RemoveEventListenerByType(handler, NS_LITERAL_STRING("keypress"),
                                     TrustedEventsAtSystemGroupCapture());

  contentNode->DeleteProperty(nsGkAtoms::listener);

  return NS_OK;
}



nsresult
nsXBLService::BindingReady(nsIContent* aBoundElement,
                           nsIURI* aURI,
                           bool* aIsReady)
{
  
  return GetBinding(aBoundElement, aURI, true, nullptr, aIsReady, nullptr);
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

static bool
MayBindToContent(nsXBLPrototypeBinding* aProtoBinding, nsIContent* aBoundElement,
                 nsIURI* aURI)
{
  
  if (aProtoBinding->BindToUntrustedContent()) {
    return true;
  }

  
  
  if (aBoundElement->IsXUL() || aBoundElement->OwnerDoc()->IsXUL()) {
    return true;
  }

  
  
  
  
  if (aBoundElement->IsInAnonymousSubtree()) {
    return true;
  }

  
  nsCOMPtr<nsIDocument> bindingDoc = aProtoBinding->XBLDocumentInfo()->GetDocument();
  NS_ENSURE_TRUE(bindingDoc, false);
  if (aBoundElement->NodePrincipal()->Subsumes(bindingDoc->NodePrincipal())) {
    return true;
  }

  
  
  
  
  if (nsContentUtils::AllowXULXBLForPrincipal(aBoundElement->NodePrincipal())) {
    bool isDataURI = false;
    nsresult rv = aURI->SchemeIs("data", &isDataURI);
    NS_ENSURE_SUCCESS(rv, false);
    if (isDataURI) {
      return true;
    }
  }

  
  return false;
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
    *aResult = nullptr;

  if (!aURI)
    return NS_ERROR_FAILURE;

  nsAutoCString ref;
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

  if (!protoBinding) {
#ifdef DEBUG
    nsAutoCString uriSpec;
    aURI->GetSpec(uriSpec);
    nsAutoCString doc;
    boundDocument->GetDocumentURI()->GetSpec(doc);
    nsAutoCString message("Unable to locate an XBL binding for URI ");
    message += uriSpec;
    message += " in document ";
    message += doc;
    NS_WARNING(message.get());
#endif
    return NS_ERROR_FAILURE;
  }

  
  
  if (!MayBindToContent(protoBinding, aBoundElement, aURI)) {
#ifdef DEBUG
    nsAutoCString uriSpec;
    aURI->GetSpec(uriSpec);
    nsAutoCString message("Permission denied to apply binding ");
    message += uriSpec;
    message += " to unprivileged content. Set bindToUntrustedContent=true on "
               "the binding to override this restriction.";
    NS_WARNING(message.get());
#endif
   return NS_ERROR_FAILURE;
  }

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
      uint32_t count = aDontExtendURIs.Length();
      for (uint32_t index = 0; index < count; ++index) {
        bool equal;
        rv = aDontExtendURIs[index]->Equals(baseBindingURI, &equal);
        NS_ENSURE_SUCCESS(rv, rv);
        if (equal) {
          nsAutoCString spec, basespec;
          protoBinding->BindingURI()->GetSpec(spec);
          NS_ConvertUTF8toUTF16 protoSpec(spec);
          baseBindingURI->GetSpec(basespec);
          NS_ConvertUTF8toUTF16 baseSpecUTF16(basespec);
          const char16_t* params[] = { protoSpec.get(), baseSpecUTF16.get() };
          nsContentUtils::ReportToConsole(nsIScriptError::warningFlag,
                                          NS_LITERAL_CSTRING("XBL"), nullptr,
                                          nsContentUtils::eXBL_PROPERTIES,
                                          "CircularExtendsBinding",
                                          params, ArrayLength(params),
                                          boundDocument->GetDocumentURI());
          return NS_ERROR_ILLEGAL_VALUE;
        }
      }
    }
  }

  nsRefPtr<nsXBLBinding> baseBinding;
  if (baseBindingURI) {
    nsIContent* child = protoBinding->GetBindingElement();
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

nsresult
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
                                                           true, false);
        NS_ENSURE_SUCCESS(rv, NS_ERROR_XBL_BLOCKED);
      }

      
      NS_ENSURE_TRUE(aBoundDocument->AllowXULXBL(),
                     NS_ERROR_XBL_BLOCKED);
    }
  }

  *aResult = nullptr;
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
    
    nsBindingManager *bindingManager = nullptr;

    if (aBoundDocument) {
      bindingManager = aBoundDocument->BindingManager();
      info = bindingManager->GetXBLDocumentInfo(documentURI);
      if (aBoundDocument->IsStaticDocument() &&
          IsChromeOrResourceURI(aBindingURI)) {
        aForceSyncLoad = true;
      }
    }

    NodeInfo *ni = nullptr;
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
          nsXBLBindingRequest* req = new nsXBLBindingRequest(aBindingURI, aBoundElement);
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
                           aBindingURI, aOriginPrincipal, aForceSyncLoad,
                           getter_AddRefs(document));

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

  info.forget(aResult);

  return NS_OK;
}

nsresult
nsXBLService::FetchBindingDocument(nsIContent* aBoundElement, nsIDocument* aBoundDocument,
                                   nsIURI* aDocumentURI, nsIURI* aBindingURI,
                                   nsIPrincipal* aOriginPrincipal, bool aForceSyncLoad,
                                   nsIDocument** aResult)
{
  nsresult rv = NS_OK;
  
  *aResult = nullptr;

  
  
  nsCOMPtr<nsILoadGroup> loadGroup;
  if (aBoundDocument)
    loadGroup = aBoundDocument->GetDocumentLoadGroup();

  
  
  if (IsChromeOrResourceURI(aDocumentURI))
    aForceSyncLoad = true;

  
  nsCOMPtr<nsIDocument> doc;
  rv = NS_NewXMLDocument(getter_AddRefs(doc));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIXMLContentSink> xblSink;
  rv = NS_NewXBLContentSink(getter_AddRefs(xblSink), doc, aDocumentURI, nullptr);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  
  
  nsCOMPtr<nsIPrincipal> requestingPrincipal = aOriginPrincipal ? aOriginPrincipal
                                                                : nsContentUtils::GetSystemPrincipal();
  nsCOMPtr<nsIChannel> channel;
  
  
  rv = NS_NewChannelInternal(getter_AddRefs(channel),
                             aDocumentURI,
                             aBoundDocument,
                             requestingPrincipal,
                             nsILoadInfo::SEC_NORMAL,
                             nsIContentPolicy::TYPE_OTHER,
                             nullptr,   
                             loadGroup);

  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIInterfaceRequestor> sameOriginChecker = nsContentUtils::GetSameOriginChecker();
  NS_ENSURE_TRUE(sameOriginChecker, NS_ERROR_OUT_OF_MEMORY);

  channel->SetNotificationCallbacks(sameOriginChecker);

  if (!aForceSyncLoad) {
    
    nsXBLStreamListener* xblListener =
      new nsXBLStreamListener(aBoundDocument, xblSink, doc);
    NS_ENSURE_TRUE(xblListener,NS_ERROR_OUT_OF_MEMORY);

    
    nsBindingManager *bindingManager;
    if (aBoundDocument)
      bindingManager = aBoundDocument->BindingManager();
    else
      bindingManager = nullptr;

    if (bindingManager)
      bindingManager->PutLoadingDocListener(aDocumentURI, xblListener);

    
    nsXBLBindingRequest* req = new nsXBLBindingRequest(aBindingURI,
                                                       aBoundElement);
    xblListener->AddRequest(req);

    
    rv = channel->AsyncOpen(xblListener, nullptr);
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
                              nullptr,
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
