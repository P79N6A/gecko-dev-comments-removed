





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
nsWebBrowserContentPolicy::ShouldLoad(uint32_t aContentType,
                                      nsIURI* aContentLocation,
                                      nsIURI* aRequestingLocation,
                                      nsISupports* aRequestingContext,
                                      const nsACString& aMimeGuess,
                                      nsISupports* aExtra,
                                      nsIPrincipal* aRequestPrincipal,
                                      int16_t* aShouldLoad)
{
  NS_PRECONDITION(aShouldLoad, "Null out param");

  MOZ_ASSERT(aContentType == nsContentUtils::InternalContentPolicyTypeToExternal(aContentType),
             "We should only see external content policy types here.");

  *aShouldLoad = nsIContentPolicy::ACCEPT;

  nsIDocShell* shell = NS_CP_GetDocShellFromContext(aRequestingContext);
  
  if (!shell) {
    return NS_OK;
  }

  nsresult rv;
  bool allowed = true;

  switch (aContentType) {
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
    case nsIContentPolicy::TYPE_IMAGESET:
      rv = shell->GetAllowImages(&allowed);
      break;
    default:
      return NS_OK;
  }

  if (NS_SUCCEEDED(rv) && !allowed) {
    *aShouldLoad = nsIContentPolicy::REJECT_TYPE;
  }
  return rv;
}

NS_IMETHODIMP
nsWebBrowserContentPolicy::ShouldProcess(uint32_t aContentType,
                                         nsIURI* aContentLocation,
                                         nsIURI* aRequestingLocation,
                                         nsISupports* aRequestingContext,
                                         const nsACString& aMimeGuess,
                                         nsISupports* aExtra,
                                         nsIPrincipal* aRequestPrincipal,
                                         int16_t* aShouldProcess)
{
  NS_PRECONDITION(aShouldProcess, "Null out param");

  MOZ_ASSERT(aContentType == nsContentUtils::InternalContentPolicyTypeToExternal(aContentType),
             "We should only see external content policy types here.");

  *aShouldProcess = nsIContentPolicy::ACCEPT;

  
  
  
  if (aContentType != nsIContentPolicy::TYPE_OBJECT) {
    return NS_OK;
  }

  nsIDocShell* shell = NS_CP_GetDocShellFromContext(aRequestingContext);
  if (shell && (!shell->PluginsAllowedInCurrentDoc())) {
    *aShouldProcess = nsIContentPolicy::REJECT_TYPE;
  }

  return NS_OK;
}
