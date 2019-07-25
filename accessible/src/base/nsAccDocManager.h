




































#ifndef nsAccDocManager_h_
#define nsAccDocManager_h_

#include "nsIDocument.h"
#include "nsIDOMEventListener.h"
#include "nsRefPtrHashtable.h"
#include "nsIWebProgress.h"
#include "nsIWebProgressListener.h"
#include "nsWeakReference.h"

class nsAccessible;
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

  



  nsAccessible* FindAccessibleInCache(nsINode* aNode) const;

  


  inline nsDocAccessible* GetDocAccessibleFromCache(nsIDocument* aDocument) const
  {
    return mDocAccessibleCache.GetWeak(aDocument);
  }

  


  inline void NotifyOfDocumentShutdown(nsIDocument* aDocument)
  {
    mDocAccessibleCache.Remove(aDocument);
  }

protected:
  nsAccDocManager() { };

  


  PRBool Init();

  


  void Shutdown();

private:
  nsAccDocManager(const nsAccDocManager&);
  nsAccDocManager& operator =(const nsAccDocManager&);

private:
  







  void HandleDOMDocumentLoad(nsIDocument *aDocument,
                             PRUint32 aLoadEventType);

  















  PRBool IsEventTargetDocument(nsIDocument *aDocument) const;

  


  void AddListeners(nsIDocument *aDocument, PRBool aAddPageShowListener);

  


  nsDocAccessible *CreateDocOrRootAccessible(nsIDocument *aDocument);

  typedef nsRefPtrHashtable<nsPtrHashKey<const nsIDocument>, nsDocAccessible>
    nsDocAccessibleHashtable;

  


  static PLDHashOperator
    GetFirstEntryInDocCache(const nsIDocument* aKey,
                            nsDocAccessible* aDocAccessible,
                            void* aUserArg);

  


  void ClearDocCache();

  struct nsSearchAccessibleInCacheArg
  {
    nsAccessible *mAccessible;
    nsINode* mNode;
  };

  static PLDHashOperator
    SearchAccessibleInDocCache(const nsIDocument* aKey,
                               nsDocAccessible* aDocAccessible,
                               void* aUserArg);

  nsDocAccessibleHashtable mDocAccessibleCache;
};




#ifdef DEBUG_ACCDOCMGR


#define DEBUG_ACCDOCMGR_DOCLOAD
#define DEBUG_ACCDOCMGR_DOCCREATE
#define DEBUG_ACCDOCMGR_DOCDESTROY


#define NS_LOG_ACCDOC_ADDRESS(aDocument, aDocAcc)                              \
  printf("DOM id: %p, acc id: %p", aDocument, aDocAcc);

#define NS_LOG_ACCDOC_URI(aDocument)                                           \
  nsIURI *uri = aDocument->GetDocumentURI();                                   \
  nsCAutoString spec;                                                          \
  uri->GetSpec(spec);                                                          \
  printf("uri: %s", spec);

#define NS_LOG_ACCDOC_TYPE(aDocument)                                          \
  if (aDocument->IsActive()) {                                                 \
    PRBool isContent = nsCoreUtils::IsContentDocument(aDocument);              \
    printf("%s document", (isContent ? "content" : "chrome"));                 \
  } else {                                                                     \
    printf("document type: [failed]");                                         \
  }

#define NS_LOG_ACCDOC_SHELLSTATE(aDocument)                                    \
  nsCAutoString docShellBusy;                                                  \
  nsCOMPtr<nsISupports> container = aDocument->GetContainer();                 \
  if (container) {                                                             \
    nsCOMPtr<nsIDocShell> docShell = do_QueryInterface(container);             \
    PRUint32 busyFlags = nsIDocShell::BUSY_FLAGS_NONE;                         \
    docShell->GetBusyFlags(&busyFlags);                                        \
    if (busyFlags == nsIDocShell::BUSY_FLAGS_NONE)                             \
      docShellBusy.AppendLiteral("'none'");                                    \
    if (busyFlags & nsIDocShell::BUSY_FLAGS_BUSY)                              \
      docShellBusy.AppendLiteral("'busy'");                                    \
    if (busyFlags & nsIDocShell::BUSY_FLAGS_BEFORE_PAGE_LOAD)                  \
      docShellBusy.AppendLiteral(", 'before page load'");                      \
    if (busyFlags & nsIDocShell::BUSY_FLAGS_PAGE_LOADING)                      \
      docShellBusy.AppendLiteral(", 'page loading'");                          \
  }                                                                            \
  else {                                                                       \
    docShellBusy.AppendLiteral("[failed]");                                    \
  }                                                                            \
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
  nsIPresShell *ps = aDocument->GetShell();                                    \
  printf("presshell: %p", ps);                                                 \
  nsIScrollableFrame *sf = ps ?                                                \
    ps->GetRootScrollFrameAsScrollableExternal() : nsnull;                     \
  printf(", root scroll frame: %p", sf);

#define NS_LOG_ACCDOC_DOCLOADGROUP(aDocument)                                  \
  nsCOMPtr<nsILoadGroup> loadGroup = aDocument->GetDocumentLoadGroup();        \
  printf("load group: %p", loadGroup);

#define NS_LOG_ACCDOC_DOCPARENT(aDocument)                                     \
  nsIDocument *parentDoc = aDocument->GetParentDocument();                     \
  printf("parent id: %p", parentDoc);                                          \
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
    if (aDocument) {                                                           \
      NS_LOG_ACCDOC_URI(aDocument)                                             \
      printf("\n    ");                                                        \
      NS_LOG_ACCDOC_SHELLSTATE(aDocument)                                      \
      printf("; ");                                                            \
      NS_LOG_ACCDOC_TYPE(aDocument)                                            \
      printf("\n    ");                                                        \
      NS_LOG_ACCDOC_DOCSTATES(aDocument)                                       \
      printf("\n    ");                                                        \
      NS_LOG_ACCDOC_DOCPRESSHELL(aDocument)                                    \
      printf("\n    ");                                                        \
      NS_LOG_ACCDOC_DOCLOADGROUP(aDocument)                                    \
      printf(", ");                                                            \
      NS_LOG_ACCDOC_DOCPARENT(aDocument)                                       \
      printf("\n");                                                            \
    }                                                                          \
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
    AccStateChangeEvent* event = downcast_accEvent(aEvent);                    \
    if (event->GetState() == states::BUSY) {                                   \
      strEventType.AssignLiteral("busy ");                                     \
      if (event->IsStateEnabled())                                             \
        strEventType.AppendLiteral("true");                                    \
      else                                                                     \
        strEventType.AppendLiteral("false");                                   \
    }                                                                          \
  }

#define NS_LOG_ACCDOC_ACCADDRESS(aName, aAcc)                                  \
  {                                                                            \
    nsINode* node = aAcc->GetNode();                                           \
    nsIDocument* doc = aAcc->GetDocumentNode();                                \
    nsDocAccessible *docacc = GetAccService()->GetDocAccessibleFromCache(doc); \
    printf("  " aName " accessible: %p, node: %p\n", aAcc, node);              \
    printf("  docacc for " aName " accessible: %p, node: %p\n", docacc, doc);  \
    printf("  ");                                                              \
    NS_LOG_ACCDOC_URI(doc)                                                     \
    printf("\n");                                                              \
  }

#define NS_LOG_ACCDOC_MSG(aMsg)                                                \
  printf("\n" aMsg "\n");                                                      \

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
    NS_LOG_ACCDOC_MSG("A11Y DOCLOAD: " aMsg);                                  \
                                                                               \
    nsCOMPtr<nsIDOMWindow> DOMWindow;                                          \
    aWebProgress->GetDOMWindow(getter_AddRefs(DOMWindow));                     \
    if (DOMWindow) {                                                           \
      nsCOMPtr<nsIDOMDocument> DOMDocument;                                    \
      DOMWindow->GetDocument(getter_AddRefs(DOMDocument));                     \
      if (DOMDocument) {                                                       \
        nsCOMPtr<nsIDocument> document(do_QueryInterface(DOMDocument));        \
        nsDocAccessible *docAcc =                                              \
          GetAccService()->GetDocAccessibleFromCache(document);                \
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
    NS_LOG_ACCDOC_MSG("A11Y DOCLOAD: " aMsg);                                  \
    nsDocAccessible *docAcc =                                                  \
      GetAccService()->GetDocAccessibleFromCache(aDocument);                   \
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
  NS_LOG_ACCDOC_MSG("A11Y DOCCREATE: " aMsg);                                  \
  NS_LOG_ACCDOC_DOCINFO(aDocument, aDocAcc)

#define NS_LOG_ACCDOCCREATE(aMsg, aDocument)                                   \
  {                                                                            \
    nsDocAccessible *docAcc =                                                  \
      GetAccService()->GetDocAccessibleFromCache(aDocument);                   \
    NS_LOG_ACCDOCCREATE_FOR(aMsg, aDocument, docAcc)                           \
  }

#define NS_LOG_ACCDOCCREATE_ACCADDRESS(aName, aAcc)                            \
  NS_LOG_ACCDOC_ACCADDRESS(aName, aAcc)

#define NS_LOG_ACCDOCCREATE_TEXT(aMsg)                                         \
    NS_LOG_ACCDOC_TEXT(aMsg)

#endif 


#ifdef DEBUG_ACCDOCMGR_DOCDESTROY
#define NS_LOG_ACCDOCDESTROY_FOR(aMsg, aDocument, aDocAcc)                     \
  NS_LOG_ACCDOC_MSG("A11Y DOCDESTROY: " aMsg);                                 \
  NS_LOG_ACCDOC_DOCINFO(aDocument, aDocAcc)

#define NS_LOG_ACCDOCDESTROY(aMsg, aDocument)                                  \
  {                                                                            \
    nsDocAccessible* docAcc =                                                  \
      GetAccService()->GetDocAccessibleFromCache(aDocument);                   \
    NS_LOG_ACCDOCDESTROY_FOR(aMsg, aDocument, docAcc)                          \
  }

#define NS_LOG_ACCDOCDESTROY_ACCADDRESS(aName, aAcc)                           \
  NS_LOG_ACCDOC_ACCADDRESS(aName, aAcc)

#define NS_LOG_ACCDOCDESTROY_MSG(aMsg)                                         \
  NS_LOG_ACCDOC_MSG(aMsg)

#define NS_LOG_ACCDOCDESTROY_TEXT(aMsg)                                        \
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
#define NS_LOG_ACCDOCCREATE_ACCADDRESS(aName, aAcc)
#define NS_LOG_ACCDOCCREATE_TEXT(aMsg)
#endif

#ifndef DEBUG_ACCDOCMGR_DOCDESTROY
#define NS_LOG_ACCDOCDESTROY_FOR(aMsg, aDocument, aDocAcc)
#define NS_LOG_ACCDOCDESTROY(aMsg, aDocument)
#define NS_LOG_ACCDOCDESTROY_MSG(aMsg)
#define NS_LOG_ACCDOCDESTROY_ACCADDRESS(aName, aAcc)
#define NS_LOG_ACCDOCDESTROY_TEXT(aMsg)
#endif

#endif 
