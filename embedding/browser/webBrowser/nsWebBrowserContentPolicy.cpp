






































#include "nsWebBrowserContentPolicy.h"
#include "nsIDocShell.h"
#include "nsCOMPtr.h"
#include "nsContentPolicyUtils.h"

nsWebBrowserContentPolicy::nsWebBrowserContentPolicy()
{
    MOZ_COUNT_CTOR(nsWebBrowserContentPolicy);
}

nsWebBrowserContentPolicy::~nsWebBrowserContentPolicy()
{
    MOZ_COUNT_DTOR(nsWebBrowserContentPolicy);
}

NS_IMPL_ISUPPORTS1(nsWebBrowserContentPolicy, nsIContentPolicy)

static nsresult
PerformPolicyCheck(PRUint32     contentType,
                   nsISupports *requestingContext,
                   PRInt16     *decision)
{
    NS_PRECONDITION(decision, "Null out param");

    *decision = nsIContentPolicy::ACCEPT;

    nsIDocShell *shell = NS_CP_GetDocShellFromContext(requestingContext);
    
    if (!shell)
        return NS_OK;

    nsresult rv;
    PRBool allowed = PR_TRUE;

    switch (contentType) {
      case nsIContentPolicy::TYPE_OBJECT:
        rv = shell->GetAllowPlugins(&allowed);
        break;
      case nsIContentPolicy::TYPE_SCRIPT:
        rv = shell->GetAllowJavascript(&allowed);
        break;
      case nsIContentPolicy::TYPE_SUBDOCUMENT:
        rv = shell->GetAllowSubframes(&allowed);
        break;
#if 0
      
      case nsIContentPolicy::TYPE_REFRESH:
        rv = shell->GetAllowMetaRedirects(&allowed); 
        break;
#endif
      case nsIContentPolicy::TYPE_IMAGE:
        rv = shell->GetAllowImages(&allowed);
        break;
      default:
        return NS_OK;
    }

    if (NS_SUCCEEDED(rv) && !allowed) {
        *decision = nsIContentPolicy::REJECT_TYPE;
    }
    return rv;
}

NS_IMETHODIMP
nsWebBrowserContentPolicy::ShouldLoad(PRUint32          contentType,
                                      nsIURI           *contentLocation,
                                      nsIURI           *requestingLocation,
                                      nsISupports      *requestingContext,
                                      const nsACString &mimeGuess,
                                      nsISupports      *extra,
                                      PRInt16          *shouldLoad)
{
    return PerformPolicyCheck(contentType, requestingContext, shouldLoad);
}

NS_IMETHODIMP
nsWebBrowserContentPolicy::ShouldProcess(PRUint32          contentType,
                                         nsIURI           *contentLocation,
                                         nsIURI           *requestingLocation,
                                         nsISupports      *requestingContext,
                                         const nsACString &mimeGuess,
                                         nsISupports      *extra,
                                         PRInt16          *shouldProcess)
{
    *shouldProcess = nsIContentPolicy::ACCEPT;
    return NS_OK;
    
    
}
