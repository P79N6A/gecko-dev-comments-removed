









#include "prlog.h"

#include "nsISupports.h"
#include "nsXPCOM.h"
#include "nsContentPolicyUtils.h"
#include "nsContentPolicy.h"
#include "nsIURI.h"
#include "nsIDOMNode.h"
#include "nsIDOMWindow.h"
#include "nsIContent.h"
#include "nsCOMArray.h"

NS_IMPL_ISUPPORTS(nsContentPolicy, nsIContentPolicy)

#ifdef PR_LOGGING
static PRLogModuleInfo* gConPolLog;
#endif

nsresult
NS_NewContentPolicy(nsIContentPolicy **aResult)
{
  *aResult = new nsContentPolicy;
  if (!*aResult)
      return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(*aResult);
  return NS_OK;
}

nsContentPolicy::nsContentPolicy()
    : mPolicies(NS_CONTENTPOLICY_CATEGORY)
    , mSimplePolicies(NS_SIMPLECONTENTPOLICY_CATEGORY)
{
#ifdef PR_LOGGING
    if (! gConPolLog) {
        gConPolLog = PR_NewLogModule("nsContentPolicy");
    }
#endif
}

nsContentPolicy::~nsContentPolicy()
{
}

#ifdef DEBUG
#define WARN_IF_URI_UNINITIALIZED(uri,name)                         \
  PR_BEGIN_MACRO                                                    \
    if ((uri)) {                                                    \
        nsAutoCString spec;                                         \
        (uri)->GetAsciiSpec(spec);                                  \
        if (spec.IsEmpty()) {                                       \
            NS_WARNING(name " is uninitialized, fix caller");       \
        }                                                           \
    }                                                               \
  PR_END_MACRO

#else  

#define WARN_IF_URI_UNINITIALIZED(uri,name)

#endif 

inline nsresult
nsContentPolicy::CheckPolicy(CPMethod          policyMethod,
                             SCPMethod         simplePolicyMethod,
                             uint32_t          contentType,
                             nsIURI           *contentLocation,
                             nsIURI           *requestingLocation,
                             nsISupports      *requestingContext,
                             const nsACString &mimeType,
                             nsISupports      *extra,
                             nsIPrincipal     *requestPrincipal,
                             int16_t           *decision)
{
    
    NS_PRECONDITION(decision, "Null out pointer");
    WARN_IF_URI_UNINITIALIZED(contentLocation, "Request URI");
    WARN_IF_URI_UNINITIALIZED(requestingLocation, "Requesting URI");

#ifdef DEBUG
    {
        nsCOMPtr<nsIDOMNode> node(do_QueryInterface(requestingContext));
        nsCOMPtr<nsIDOMWindow> window(do_QueryInterface(requestingContext));
        NS_ASSERTION(!requestingContext || node || window,
                     "Context should be a DOM node or a DOM window!");
    }
#endif

    




    if (!requestingLocation) {
        nsCOMPtr<nsIDocument> doc;
        nsCOMPtr<nsIContent> node = do_QueryInterface(requestingContext);
        if (node) {
            doc = node->OwnerDoc();
        }
        if (!doc) {
            doc = do_QueryInterface(requestingContext);
        }
        if (doc) {
            requestingLocation = doc->GetDocumentURI();
        }
    }

    



    nsresult rv;
    nsCOMArray<nsIContentPolicy> entries;
    mPolicies.GetEntries(entries);
    int32_t count = entries.Count();
    for (int32_t i = 0; i < count; i++) {
        
        rv = (entries[i]->*policyMethod)(contentType, contentLocation,
                                         requestingLocation, requestingContext,
                                         mimeType, extra, requestPrincipal,
                                         decision);

        if (NS_SUCCEEDED(rv) && NS_CP_REJECTED(*decision)) {
            
            return NS_OK;
        }
    }

    nsCOMPtr<nsIDOMElement> topFrameElement;
    bool isTopLevel = true;
    nsCOMPtr<nsPIDOMWindow> window;
    if (nsCOMPtr<nsINode> node = do_QueryInterface(requestingContext)) {
        window = node->OwnerDoc()->GetWindow();
    } else {
        window = do_QueryInterface(requestingContext);
    }

    if (window) {
        nsCOMPtr<nsIDocShell> docShell = window->GetDocShell();
        nsCOMPtr<nsILoadContext> loadContext = do_QueryInterface(docShell);
        loadContext->GetTopFrameElement(getter_AddRefs(topFrameElement));

        MOZ_ASSERT(window->IsOuterWindow());

        if (topFrameElement) {
            nsCOMPtr<nsIDOMWindow> topWindow;
            window->GetScriptableTop(getter_AddRefs(topWindow));
            isTopLevel = topWindow == static_cast<nsIDOMWindow*>(window);
        } else {
            
            
            
            
            topFrameElement = do_QueryInterface(requestingContext);
            isTopLevel = true;
        }
    }

    nsCOMArray<nsISimpleContentPolicy> simpleEntries;
    mSimplePolicies.GetEntries(simpleEntries);
    count = simpleEntries.Count();
    for (int32_t i = 0; i < count; i++) {
        
        rv = (simpleEntries[i]->*simplePolicyMethod)(contentType, contentLocation,
                                                     requestingLocation,
                                                     topFrameElement, isTopLevel,
                                                     mimeType, extra, requestPrincipal,
                                                     decision);

        if (NS_SUCCEEDED(rv) && NS_CP_REJECTED(*decision)) {
            
            return NS_OK;
        }
    }

    
    *decision = nsIContentPolicy::ACCEPT;
    return NS_OK;
}

#ifdef PR_LOGGING



#define LOG_CHECK(logType)                                                    \
  PR_BEGIN_MACRO                                                              \
    /* skip all this nonsense if the call failed */                           \
    if (NS_SUCCEEDED(rv)) {                                                   \
      const char *resultName;                                                 \
      if (decision) {                                                         \
        resultName = NS_CP_ResponseName(*decision);                           \
      } else {                                                                \
        resultName = "(null ptr)";                                            \
      }                                                                       \
      nsAutoCString spec("None");                                             \
      if (contentLocation) {                                                  \
          contentLocation->GetSpec(spec);                                     \
      }                                                                       \
      nsAutoCString refSpec("None");                                          \
      if (requestingLocation) {                                               \
          requestingLocation->GetSpec(refSpec);                               \
      }                                                                       \
      PR_LOG(gConPolLog, PR_LOG_DEBUG,                                        \
             ("Content Policy: " logType ": <%s> <Ref:%s> result=%s",         \
              spec.get(), refSpec.get(), resultName)                          \
             );                                                               \
    }                                                                         \
  PR_END_MACRO

#else 

#define LOG_CHECK(logType)

#endif 

NS_IMETHODIMP
nsContentPolicy::ShouldLoad(uint32_t          contentType,
                            nsIURI           *contentLocation,
                            nsIURI           *requestingLocation,
                            nsISupports      *requestingContext,
                            const nsACString &mimeType,
                            nsISupports      *extra,
                            nsIPrincipal     *requestPrincipal,
                            int16_t          *decision)
{
    
    NS_PRECONDITION(contentLocation, "Must provide request location");
    nsresult rv = CheckPolicy(&nsIContentPolicy::ShouldLoad,
                              &nsISimpleContentPolicy::ShouldLoad,
                              contentType,
                              contentLocation, requestingLocation,
                              requestingContext, mimeType, extra,
                              requestPrincipal, decision);
    LOG_CHECK("ShouldLoad");

    return rv;
}

NS_IMETHODIMP
nsContentPolicy::ShouldProcess(uint32_t          contentType,
                               nsIURI           *contentLocation,
                               nsIURI           *requestingLocation,
                               nsISupports      *requestingContext,
                               const nsACString &mimeType,
                               nsISupports      *extra,
                               nsIPrincipal     *requestPrincipal,
                               int16_t          *decision)
{
    nsresult rv = CheckPolicy(&nsIContentPolicy::ShouldProcess,
                              &nsISimpleContentPolicy::ShouldProcess,
                              contentType,
                              contentLocation, requestingLocation,
                              requestingContext, mimeType, extra,
                              requestPrincipal, decision);
    LOG_CHECK("ShouldProcess");

    return rv;
}
