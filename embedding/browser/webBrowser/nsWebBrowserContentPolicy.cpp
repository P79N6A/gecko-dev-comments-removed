






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

NS_IMETHODIMP
nsWebBrowserContentPolicy::ShouldLoad(PRUint32          contentType,
                                      nsIURI           *contentLocation,
                                      nsIURI           *requestingLocation,
                                      nsISupports      *requestingContext,
                                      const nsACString &mimeGuess,
                                      nsISupports      *extra,
                                      nsIPrincipal     *requestPrincipal,
                                      PRInt16          *shouldLoad)
{
    NS_PRECONDITION(shouldLoad, "Null out param");

    *shouldLoad = nsIContentPolicy::ACCEPT;

    nsIDocShell *shell = NS_CP_GetDocShellFromContext(requestingContext);
    
    if (!shell) {
        return NS_OK;
    }
    
    nsresult rv;
    bool allowed = true;

    switch (contentType) {
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
        *shouldLoad = nsIContentPolicy::REJECT_TYPE;
    }
    return rv;
}

NS_IMETHODIMP
nsWebBrowserContentPolicy::ShouldProcess(PRUint32          contentType,
                                         nsIURI           *contentLocation,
                                         nsIURI           *requestingLocation,
                                         nsISupports      *requestingContext,
                                         const nsACString &mimeGuess,
                                         nsISupports      *extra,
                                         nsIPrincipal     *requestPrincipal,
                                         PRInt16          *shouldProcess)
{
    NS_PRECONDITION(shouldProcess, "Null out param");

    *shouldProcess = nsIContentPolicy::ACCEPT;

    
    
    
    if (contentType != nsIContentPolicy::TYPE_OBJECT) {
        return NS_OK;
    }

    nsIDocShell *shell = NS_CP_GetDocShellFromContext(requestingContext);
    bool allowed;
    if (shell && (NS_FAILED(shell->GetAllowPlugins(&allowed)) || !allowed)) {
        *shouldProcess = nsIContentPolicy::REJECT_TYPE;
    }

    return NS_OK;
}
