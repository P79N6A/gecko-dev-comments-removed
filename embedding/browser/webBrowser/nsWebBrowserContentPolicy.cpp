






#include "nsWebBrowserContentPolicy.h"
#include "nsIDocShell.h"
#include "nsCOMPtr.h"
#include "nsContentPolicyUtils.h"
#include "nsIContentViewer.h"

nsWebBrowserContentPolicy::nsWebBrowserContentPolicy()
{
    MOZ_COUNT_CTOR(nsWebBrowserContentPolicy);
}

nsWebBrowserContentPolicy::~nsWebBrowserContentPolicy()
{
    MOZ_COUNT_DTOR(nsWebBrowserContentPolicy);
}

NS_IMPL_ISUPPORTS(nsWebBrowserContentPolicy, nsIContentPolicy)

NS_IMETHODIMP
nsWebBrowserContentPolicy::ShouldLoad(uint32_t          contentType,
                                      nsIURI           *contentLocation,
                                      nsIURI           *requestingLocation,
                                      nsISupports      *requestingContext,
                                      const nsACString &mimeGuess,
                                      nsISupports      *extra,
                                      nsIPrincipal     *requestPrincipal,
                                      int16_t          *shouldLoad)
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
nsWebBrowserContentPolicy::ShouldProcess(uint32_t          contentType,
                                         nsIURI           *contentLocation,
                                         nsIURI           *requestingLocation,
                                         nsISupports      *requestingContext,
                                         const nsACString &mimeGuess,
                                         nsISupports      *extra,
                                         nsIPrincipal     *requestPrincipal,
                                         int16_t          *shouldProcess)
{
    NS_PRECONDITION(shouldProcess, "Null out param");

    *shouldProcess = nsIContentPolicy::ACCEPT;

    
    
    
    if (contentType != nsIContentPolicy::TYPE_OBJECT) {
        return NS_OK;
    }

    nsIDocShell *shell = NS_CP_GetDocShellFromContext(requestingContext);
    if (shell && (!shell->PluginsAllowedInCurrentDoc())) {
        *shouldProcess = nsIContentPolicy::REJECT_TYPE;
    }

    return NS_OK;
}
