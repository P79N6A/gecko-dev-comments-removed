




































#ifndef nsAccDocManager_h_
#define nsAccDocManager_h_

#include "nsAccessible.h"

#include "nsIDocument.h"
#include "nsIDOMEventListener.h"
#include "nsIWebProgress.h"
#include "nsIWebProgressListener.h"
#include "nsWeakReference.h"

class nsDocAccessible;




class nsAccDocManager : public nsIWebProgressListener,
                        public nsIDOMEventListener,
                        public nsSupportsWeakReference
{
public:
  virtual ~nsAccDocManager() { };

  NS_DECL_ISUPPORTS
  NS_DECL_NSIWEBPROGRESSLISTENER
  NS_DECL_NSIDOMEVENTLISTENER

  


  nsDocAccessible *GetDocAccessible(nsIDocument *aDocument);

  



  nsAccessible *FindAccessibleInCache(void *aUniqueID) const;

protected:
  nsAccDocManager() { };

  


  PRBool Init();

  


  void Shutdown();

  


  void ShutdownDocAccessible(nsIDocument *aDocument);

  




  void ShutdownDocAccessiblesInTree(nsIDocument *aDocument);

private:
  nsAccDocManager(const nsAccDocManager&);
  nsAccDocManager& operator =(const nsAccDocManager&);

private:
  










  void HandleDOMDocumentLoad(nsIDocument *aDocument,
                             PRUint32 aLoadEventType,
                             PRBool aMarkAsLoaded = PR_FALSE);

  















  PRBool IsEventTargetDocument(nsIDocument *aDocument) const;

  


  void AddListeners(nsIDocument *aDocument, PRBool aAddPageShowListener);
  void RemoveListeners(nsIDocument *aDocument);

  


  nsDocAccessible *CreateDocOrRootAccessible(nsIDocument *aDocument);

  


  void ShutdownDocAccessiblesInTree(nsIDocShellTreeItem *aTreeItem,
                                    nsIDocument *aDocument);

  typedef nsRefPtrHashtable<nsVoidPtrHashKey, nsDocAccessible>
    nsDocAccessibleHashtable;

  


  static PLDHashOperator
    ClearDocCacheEntry(const void* aKey,
                       nsRefPtr<nsDocAccessible>& aDocAccessible,
                       void* aUserArg);

  


  void ClearDocCache()
  {
    mDocAccessibleCache.Enumerate(ClearDocCacheEntry, static_cast<void*>(this));
  }

  struct nsSearchAccessibleInCacheArg
  {
    nsAccessible *mAccessible;
    void *mUniqueID;
  };

  static PLDHashOperator
    SearchAccessibleInDocCache(const void* aKey,
                               nsDocAccessible* aDocAccessible,
                               void* aUserArg);

  nsDocAccessibleHashtable mDocAccessibleCache;
};






#ifdef DEBUG_ACCDOCMGR


#define DEBUG_ACCDOCMGR_DOCLOAD
#define DEBUG_ACCDOCMGR_DOCCREATE
#define DEBUG_ACCDOCMGR_DOCDESTROY


#define NS_LOG_ACCDOC_ADDRESS(aDocument, aDocAcc)                              \
  printf("DOM id: 0x%x, acc id: 0x%x",                                         \
         reinterpret_cast<PRInt32>(static_cast<void*>(aDocument)),             \
         reinterpret_cast<PRInt32>(aDocAcc));

#define NS_LOG_ACCDOC_URI(aDocument)                                           \
  nsIURI *uri = aDocument->GetDocumentURI();                                   \
  nsCAutoString spec;                                                          \
  uri->GetSpec(spec);                                                          \
  printf("uri: %s", spec);

#define NS_LOG_ACCDOC_TYPE(aDocument)                                          \
  PRBool isContent = nsCoreUtils::IsContentDocument(aDocument);                \
  printf("%s document", (isContent ? "content" : "chrome"));

#define NS_LOG_ACCDOC_SHELLSTATE(aDocument)                                    \
  nsCOMPtr<nsISupports> container = aDocument->GetContainer();                 \
  nsCOMPtr<nsIDocShell> docShell = do_QueryInterface(container);               \
  PRUint32 busyFlags = nsIDocShell::BUSY_FLAGS_NONE;                           \
  docShell->GetBusyFlags(&busyFlags);                                          \
  nsCAutoString docShellBusy;                                                  \
  if (busyFlags == nsIDocShell::BUSY_FLAGS_NONE)                               \
    docShellBusy.AppendLiteral("'none'");                                      \
  if (busyFlags & nsIDocShell::BUSY_FLAGS_BUSY)                                \
    docShellBusy.AppendLiteral("'busy'");                                      \
  if (busyFlags & nsIDocShell::BUSY_FLAGS_BEFORE_PAGE_LOAD)                    \
    docShellBusy.AppendLiteral(", 'before page load'");                        \
  if (busyFlags & nsIDocShell::BUSY_FLAGS_PAGE_LOADING)                        \
    docShellBusy.AppendLiteral(", 'page loading'");                            \
  printf("docshell busy: %s", docShellBusy.get());

#define NS_LOG_ACCDOC_DOCSTATES(aDocument)                                     \
  const char *docState = 0;                                                    \
  nsIDocument::ReadyState docStateFlag = aDocument->GetReadyStateEnum();       \
  switch (docStateFlag) {                                                      \
    case nsIDocument::READYSTATE_UNINITIALIZED:                                \
     docState = "uninitialized";                                               \
      break;                                                                   \
    case nsIDocument::READYSTATE_LOADING:                                      \
      docState = "loading";                                                    \
      break;                                                                   \
    case nsIDocument::READYSTATE_INTERACTIVE:                                  \
      docState = "interactive";                                                \
      break;                                                                   \
    case nsIDocument::READYSTATE_COMPLETE:                                     \
      docState = "complete";                                                   \
      break;                                                                   \
  }                                                                            \
  printf("doc state: %s", docState);                                           \
  printf(", %sinitial", aDocument->IsInitialDocument() ? "" : "not ");         \
  printf(", %sshowing", aDocument->IsShowing() ? "" : "not ");                 \
  printf(", %svisible", aDocument->IsVisible() ? "" : "not ");                 \
  printf(", %sactive", aDocument->IsActive() ? "" : "not ");

#define NS_LOG_ACCDOC_DOCPRESSHELL(aDocument)                                  \
  nsIPresShell *ps = aDocument->GetPrimaryShell();                             \
  printf("presshell: 0x%x", reinterpret_cast<PRInt32>(ps));                    \
  nsIScrollableFrame *sf = ps ?                                                \
    ps->GetRootScrollFrameAsScrollableExternal() : nsnull;                     \
  printf(", root scroll frame: 0x%x", reinterpret_cast<PRInt32>(sf));

#define NS_LOG_ACCDOC_DOCLOADGROUP(aDocument)                                  \
  nsCOMPtr<nsILoadGroup> loadGroup = aDocument->GetDocumentLoadGroup();        \
  printf("load group: 0x%x", reinterpret_cast<PRInt32>(loadGroup.get()));

#define NS_LOG_ACCDOC_DOCPARENT(aDocument)                                     \
  nsIDocument *parentDoc = aDocument->GetParentDocument();                     \
  printf("parent id: 0x%x",                                                    \
         reinterpret_cast<PRInt32>(parentDoc));                                \
  if (parentDoc) {                                                             \
    printf("\n    parent ");                                                   \
    NS_LOG_ACCDOC_URI(parentDoc)                                               \
    printf("\n");                                                              \
  }

#define NS_LOG_ACCDOC_SHELLLOADTYPE(aDocShell)                                 \
  {                                                                            \
    printf("load type: ");                                                     \
    PRUint32 loadType;                                                         \
    docShell->GetLoadType(&loadType);                                          \
    switch (loadType) {                                                        \
      case LOAD_NORMAL:                                                        \
        printf("normal; ");                                                    \
        break;                                                                 \
      case LOAD_NORMAL_REPLACE:                                                \
        printf("normal replace; ");                                            \
        break;                                                                 \
      case LOAD_NORMAL_EXTERNAL:                                               \
        printf("normal external; ");                                           \
        break;                                                                 \
      case LOAD_HISTORY:                                                       \
        printf("history; ");                                                   \
        break;                                                                 \
      case LOAD_NORMAL_BYPASS_CACHE:                                           \
        printf("normal bypass cache; ");                                       \
        break;                                                                 \
      case LOAD_NORMAL_BYPASS_PROXY:                                           \
        printf("normal bypass proxy; ");                                       \
        break;                                                                 \
      case LOAD_NORMAL_BYPASS_PROXY_AND_CACHE:                                 \
        printf("normal bypass proxy and cache; ");                             \
        break;                                                                 \
      case LOAD_RELOAD_NORMAL:                                                 \
        printf("reload normal; ");                                             \
        break;                                                                 \
      case LOAD_RELOAD_BYPASS_CACHE:                                           \
        printf("reload bypass cache; ");                                       \
        break;                                                                 \
      case LOAD_RELOAD_BYPASS_PROXY:                                           \
        printf("reload bypass proxy; ");                                       \
        break;                                                                 \
      case LOAD_RELOAD_BYPASS_PROXY_AND_CACHE:                                 \
        printf("reload bypass proxy and cache; ");                             \
        break;                                                                 \
      case LOAD_LINK:                                                          \
        printf("link; ");                                                      \
        break;                                                                 \
      case LOAD_REFRESH:                                                       \
        printf("refresh; ");                                                   \
        break;                                                                 \
      case LOAD_RELOAD_CHARSET_CHANGE:                                         \
        printf("reload charset change; ");                                     \
        break;                                                                 \
      case LOAD_BYPASS_HISTORY:                                                \
        printf("bypass history; ");                                            \
        break;                                                                 \
      case LOAD_STOP_CONTENT:                                                  \
        printf("stop content; ");                                              \
        break;                                                                 \
      case LOAD_STOP_CONTENT_AND_REPLACE:                                      \
        printf("stop content and replace; ");                                  \
        break;                                                                 \
      case LOAD_PUSHSTATE:                                                     \
        printf("load pushstate; ");                                            \
        break;                                                                 \
      case LOAD_ERROR_PAGE:                                                    \
        printf("error page;");                                                 \
        break;                                                                 \
      default:                                                                 \
        printf("unknown");                                                     \
    }                                                                          \
  }

#define NS_LOG_ACCDOC_DOCINFO_BEGIN                                            \
  printf("  {\n");
#define NS_LOG_ACCDOC_DOCINFO_BODY(aDocument, aDocAcc)                         \
  {                                                                            \
    printf("    ");                                                            \
    NS_LOG_ACCDOC_ADDRESS(aDocument, aDocAcc)                                  \
    printf("\n    ");                                                          \
    NS_LOG_ACCDOC_URI(aDocument)                                               \
    printf("\n    ");                                                          \
    NS_LOG_ACCDOC_SHELLSTATE(aDocument)                                        \
    printf("; ");                                                              \
    NS_LOG_ACCDOC_TYPE(aDocument)                                              \
    printf("\n    ");                                                          \
    NS_LOG_ACCDOC_DOCSTATES(aDocument)                                         \
    printf("\n    ");                                                          \
    NS_LOG_ACCDOC_DOCPRESSHELL(aDocument)                                      \
    printf("\n    ");                                                          \
    NS_LOG_ACCDOC_DOCLOADGROUP(aDocument)                                      \
    printf(", ");                                                              \
    NS_LOG_ACCDOC_DOCPARENT(aDocument)                                         \
    printf("\n");                                                              \
  }
#define NS_LOG_ACCDOC_DOCINFO_END                                              \
  printf("  }\n");

#define NS_LOG_ACCDOC_DOCINFO(aDocument, aDocAcc)                              \
  NS_LOG_ACCDOC_DOCINFO_BEGIN                                                  \
  NS_LOG_ACCDOC_DOCINFO_BODY(aDocument, aDocAcc)                               \
  NS_LOG_ACCDOC_DOCINFO_END

#define NS_GET_ACCDOC_EVENTTYPE(aEvent)                                        \
  nsCAutoString strEventType;                                                  \
  PRUint32 type = aEvent->GetEventType();                                      \
  if (type == nsIAccessibleEvent::EVENT_DOCUMENT_LOAD_STOPPED) {               \
    strEventType.AssignLiteral("load stopped");                                \
  } else if (type == nsIAccessibleEvent::EVENT_DOCUMENT_LOAD_COMPLETE) {       \
    strEventType.AssignLiteral("load complete");                               \
  } else if (type == nsIAccessibleEvent::EVENT_DOCUMENT_RELOAD) {              \
      strEventType.AssignLiteral("reload");                                    \
  } else if (type == nsIAccessibleEvent::EVENT_STATE_CHANGE) {                 \
    nsCOMPtr<nsIAccessibleStateChangeEvent> event(do_QueryObject(aEvent));     \
    PRUint32 state = 0;                                                        \
    event->GetState(&state);                                                   \
    if (state == nsIAccessibleStates::STATE_BUSY) {                            \
      PRBool isEnabled;                                                        \
      event->IsEnabled(&isEnabled);                                            \
      strEventType.AssignLiteral("busy ");                                     \
      if (isEnabled)                                                           \
        strEventType.AppendLiteral("true");                                    \
      else                                                                     \
        strEventType.AppendLiteral("false");                                   \
    }                                                                          \
  }

#define NS_LOG_ACCDOC_TEXT(aMsg)                                               \
  printf("  " aMsg "\n");


#ifdef DEBUG_ACCDOCMGR_DOCLOAD

#define NS_LOG_ACCDOCLOAD_REQUEST(aRequest)                                    \
  if (aRequest) {                                                              \
    nsCAutoString name;                                                        \
    aRequest->GetName(name);                                                   \
    printf("    request spec: %s\n", name.get());                              \
    PRUint32 loadFlags = 0;                                                    \
    aRequest->GetLoadFlags(&loadFlags);                                        \
    printf("    request load flags: %x; ", loadFlags);                         \
    if (loadFlags & nsIChannel::LOAD_DOCUMENT_URI)                             \
      printf("document uri; ");                                                \
    if (loadFlags & nsIChannel::LOAD_RETARGETED_DOCUMENT_URI)                  \
      printf("retargeted document uri; ");                                     \
    if (loadFlags & nsIChannel::LOAD_REPLACE)                                  \
      printf("replace; ");                                                     \
    if (loadFlags & nsIChannel::LOAD_INITIAL_DOCUMENT_URI)                     \
      printf("initial document uri; ");                                        \
    if (loadFlags & nsIChannel::LOAD_TARGETED)                                 \
      printf("targeted; ");                                                    \
    if (loadFlags & nsIChannel::LOAD_CALL_CONTENT_SNIFFERS)                    \
      printf("call content sniffers; ");                                       \
    if (loadFlags & nsIChannel::LOAD_CLASSIFY_URI)                             \
      printf("classify uri; ");                                                \
  } else {                                                                     \
    printf("    no request");                                                  \
  }

#define NS_LOG_ACCDOCLOAD(aMsg, aWebProgress, aRequest, aStateFlags)           \
  {                                                                            \
    printf("\nA11Y DOCLOAD: " aMsg "\n");                                      \
                                                                               \
    nsCOMPtr<nsIDOMWindow> DOMWindow;                                          \
    aWebProgress->GetDOMWindow(getter_AddRefs(DOMWindow));                     \
    if (DOMWindow) {                                                           \
      nsCOMPtr<nsIDOMDocument> DOMDocument;                                    \
      DOMWindow->GetDocument(getter_AddRefs(DOMDocument));                     \
      if (DOMDocument) {                                                       \
        nsCOMPtr<nsIDocument> document(do_QueryInterface(DOMDocument));        \
        nsDocAccessible *docAcc =                                              \
          mDocAccessibleCache.GetWeak(static_cast<void*>(document));           \
        NS_LOG_ACCDOC_DOCINFO(document, docAcc)                                \
                                                                               \
        printf("  {\n");                                                       \
        nsCOMPtr<nsIWebNavigation> webNav(do_GetInterface(DOMWindow));         \
        nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(webNav));             \
        printf("    ");                                                        \
        NS_LOG_ACCDOC_SHELLLOADTYPE(docShell)                                  \
        printf("\n");                                                          \
        NS_LOG_ACCDOCLOAD_REQUEST(aRequest)                                    \
        printf("\n");                                                          \
        printf("    state flags: %x", aStateFlags);                            \
        PRBool isDocLoading;                                                   \
        aWebProgress->GetIsLoadingDocument(&isDocLoading);                     \
        printf(", document is %sloading\n", (isDocLoading ? "" : "not "));     \
        printf("  }\n");                                                       \
      }                                                                        \
    }                                                                          \
  }

#define NS_LOG_ACCDOCLOAD2(aMsg, aDocument)                                    \
  {                                                                            \
    printf("\nA11Y DOCLOAD: " aMsg "\n");                                      \
    nsDocAccessible *docAcc =                                                  \
      mDocAccessibleCache.GetWeak(static_cast<void*>(aDocument));              \
    NS_LOG_ACCDOC_DOCINFO(aDocument, docAcc)                                   \
  }

#define NS_LOG_ACCDOCLOAD_FIREEVENT(aEvent)                                    \
  {                                                                            \
    NS_GET_ACCDOC_EVENTTYPE(aEvent)                                            \
    if (!strEventType.IsEmpty())                                               \
      printf("  fire: %s\n", strEventType.get());                              \
  }

#define NS_LOG_ACCDOCLOAD_HANDLEEVENT(aEvent)                                  \
  {                                                                            \
    NS_GET_ACCDOC_EVENTTYPE(aEvent)                                            \
    nsCOMPtr<nsIDocument> doc(do_QueryInterface(aEvent->GetNode()));           \
    if (doc && !strEventType.IsEmpty()) {                                      \
      printf("\nA11Y DOCEVENT: handled '%s' event ", strEventType.get());      \
      nsDocAccessible *docAcc = aEvent->GetDocAccessible();                    \
      NS_LOG_ACCDOC_DOCINFO(doc, docAcc)                                       \
      printf("\n");                                                            \
    }                                                                          \
  }

#define NS_LOG_ACCDOCLOAD_TEXT(aMsg)                                           \
    NS_LOG_ACCDOC_TEXT(aMsg)

#endif 


#ifdef DEBUG_ACCDOCMGR_DOCCREATE
#define NS_LOG_ACCDOCCREATE_FOR(aMsg, aDocument, aDocAcc)                      \
  printf("\nA11Y DOCCREATE: " aMsg "\n");                                      \
  NS_LOG_ACCDOC_DOCINFO(aDocument, aDocAcc)

#define NS_LOG_ACCDOCCREATE(aMsg, aDocument)                                   \
  {                                                                            \
    nsDocAccessible *docAcc =                                                  \
      mDocAccessibleCache.GetWeak(static_cast<void*>(aDocument));              \
    NS_LOG_ACCDOCCREATE_FOR(aMsg, aDocument, docAcc)                           \
  }

#define NS_LOG_ACCDOCCREATE_TEXT(aMsg)                                         \
    NS_LOG_ACCDOC_TEXT(aMsg)

#endif 


#ifdef DEBUG_ACCDOCMGR_DOCDESTROY
#define NS_LOG_ACCDOCDESTROY_FOR(aMsg, aDocument, aDocAcc)                     \
  printf("\nA11Y DOCDESTROY: " aMsg "\n");                                     \
  NS_LOG_ACCDOC_DOCINFO(aDocument, aDocAcc)

#define NS_LOG_ACCDOCDESTROY(aMsg, aDocument)                                  \
  nsDocAccessible *docAcc =                                                    \
    mDocAccessibleCache.GetWeak(static_cast<void*>(aDocument));                \
  NS_LOG_ACCDOCDESTROY_FOR(aMsg, aDocument, docAcc)

#define NS_LOG_ACCDOCDESTROY_TEXT(aMsg)                                       \
    NS_LOG_ACCDOC_TEXT(aMsg)

#endif 

#endif 

#ifndef DEBUG_ACCDOCMGR_DOCLOAD
#define NS_LOG_ACCDOCLOAD(aMsg, aWebProgress, aRequest, aStateFlags)
#define NS_LOG_ACCDOCLOAD2(aMsg, aDocument)
#define NS_LOG_ACCDOCLOAD_EVENT(aMsg, aEvent)
#define NS_LOG_ACCDOCLOAD_FIREEVENT(aEvent)
#define NS_LOG_ACCDOCLOAD_HANDLEEVENT(aEvent)
#define NS_LOG_ACCDOCLOAD_TEXT(aMsg)
#endif

#ifndef DEBUG_ACCDOCMGR_DOCCREATE
#define NS_LOG_ACCDOCCREATE_FOR(aMsg, aDocument, aDocAcc)
#define NS_LOG_ACCDOCCREATE(aMsg, aDocument)
#define NS_LOG_ACCDOCCREATE_TEXT(aMsg)
#endif

#ifndef DEBUG_ACCDOCMGR_DOCDESTROY
#define NS_LOG_ACCDOCDESTROY_FOR(aMsg, aDocument, aDocAcc)
#define NS_LOG_ACCDOCDESTROY(aMsg, aDocument)
#define NS_LOG_ACCDOCDESTROY_TEXT(aMsg)
#endif

#endif 
