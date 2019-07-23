









































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

NS_IMPL_ISUPPORTS1(nsContentPolicy, nsIContentPolicy)

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
        nsCAutoString spec;                                         \
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
                             PRUint32          contentType,
                             nsIURI           *contentLocation,
                             nsIURI           *requestingLocation,
                             nsISupports      *requestingContext,
                             const nsACString &mimeType,
                             nsISupports      *extra,
                             PRInt16           *decision)
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
            doc = node->GetOwnerDoc();
        }
        if (!doc) {
            doc = do_QueryInterface(requestingContext);
        }
        if (doc) {
            requestingLocation = doc->GetDocumentURI();
        }
    }

    



    nsresult rv;
    const nsCOMArray<nsIContentPolicy>& entries = mPolicies.GetEntries();
    PRInt32 count = entries.Count();
    for (PRInt32 i = 0; i < count; i++) {
        
        rv = (entries[i]->*policyMethod)(contentType, contentLocation,
                                         requestingLocation, requestingContext,
                                         mimeType, extra, decision);

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
      nsCAutoString spec("None");                                             \
      if (contentLocation) {                                                  \
          contentLocation->GetSpec(spec);                                     \
      }                                                                       \
      nsCAutoString refSpec("None");                                          \
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
nsContentPolicy::ShouldLoad(PRUint32          contentType,
                            nsIURI           *contentLocation,
                            nsIURI           *requestingLocation,
                            nsISupports      *requestingContext,
                            const nsACString &mimeType,
                            nsISupports      *extra,
                            PRInt16          *decision)
{
    
    NS_PRECONDITION(contentLocation, "Must provide request location");
    nsresult rv = CheckPolicy(&nsIContentPolicy::ShouldLoad, contentType,
                              contentLocation, requestingLocation,
                              requestingContext, mimeType, extra, decision);
    LOG_CHECK("ShouldLoad");

    return rv;
}

NS_IMETHODIMP
nsContentPolicy::ShouldProcess(PRUint32          contentType,
                               nsIURI           *contentLocation,
                               nsIURI           *requestingLocation,
                               nsISupports      *requestingContext,
                               const nsACString &mimeType,
                               nsISupports      *extra,
                               PRInt16          *decision)
{
    nsresult rv = CheckPolicy(&nsIContentPolicy::ShouldProcess, contentType,
                              contentLocation, requestingLocation,
                              requestingContext, mimeType, extra, decision);
    LOG_CHECK("ShouldProcess");

    return rv;
}
