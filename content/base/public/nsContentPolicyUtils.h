












#ifndef __nsContentPolicyUtils_h__
#define __nsContentPolicyUtils_h__

#include "nsIContentPolicy.h"
#include "nsIServiceManager.h"
#include "nsIContent.h"
#include "nsIScriptSecurityManager.h"
#include "nsIPrincipal.h"


#include "nsIDocument.h"
#include "nsPIDOMWindow.h"

class nsACString;

#define NS_CONTENTPOLICY_CONTRACTID   "@mozilla.org/layout/content-policy;1"
#define NS_CONTENTPOLICY_CATEGORY "content-policy"
#define NS_CONTENTPOLICY_CID                              \
  {0x0e3afd3d, 0xeb60, 0x4c2b,                            \
     { 0x96, 0x3b, 0x56, 0xd7, 0xc4, 0x39, 0xf1, 0x24 }}






#define NS_CP_ACCEPTED(val) ((val) == nsIContentPolicy::ACCEPT)






#define NS_CP_REJECTED(val) ((val) != nsIContentPolicy::ACCEPT)





#define CASE_RETURN(name)          \
  case nsIContentPolicy:: name :   \
    return #name

#ifdef PR_LOGGING









inline const char *
NS_CP_ResponseName(PRInt16 response)
{
  switch (response) {
    CASE_RETURN( REJECT_REQUEST );
    CASE_RETURN( REJECT_TYPE    );
    CASE_RETURN( REJECT_SERVER  );
    CASE_RETURN( REJECT_OTHER   );
    CASE_RETURN( ACCEPT         );
  default:
    return "<Unknown Response>";
  }
}










inline const char *
NS_CP_ContentTypeName(PRUint32 contentType)
{
  switch (contentType) {
    CASE_RETURN( TYPE_OTHER             );
    CASE_RETURN( TYPE_SCRIPT            );
    CASE_RETURN( TYPE_IMAGE             );
    CASE_RETURN( TYPE_STYLESHEET        );
    CASE_RETURN( TYPE_OBJECT            );
    CASE_RETURN( TYPE_DOCUMENT          );
    CASE_RETURN( TYPE_SUBDOCUMENT       );
    CASE_RETURN( TYPE_REFRESH           );
    CASE_RETURN( TYPE_XBL               );
    CASE_RETURN( TYPE_PING              );
    CASE_RETURN( TYPE_XMLHTTPREQUEST    );
    CASE_RETURN( TYPE_OBJECT_SUBREQUEST );
    CASE_RETURN( TYPE_DTD               );
    CASE_RETURN( TYPE_FONT              );
    CASE_RETURN( TYPE_MEDIA             );
    CASE_RETURN( TYPE_WEBSOCKET         );
   default:
    return "<Unknown Type>";
  }
}

#endif 

#undef CASE_RETURN


#define CHECK_CONTENT_POLICY(action)                                          \
  PR_BEGIN_MACRO                                                              \
    nsCOMPtr<nsIContentPolicy> policy =                                       \
         do_GetService(NS_CONTENTPOLICY_CONTRACTID);                          \
    if (!policy)                                                              \
        return NS_ERROR_FAILURE;                                              \
                                                                              \
    return policy-> action (contentType, contentLocation, requestOrigin,      \
                            context, mimeType, extra, originPrincipal,        \
                            decision);                                        \
  PR_END_MACRO


#define CHECK_CONTENT_POLICY_WITH_SERVICE(action, _policy)                    \
  PR_BEGIN_MACRO                                                              \
    return _policy-> action (contentType, contentLocation, requestOrigin,     \
                             context, mimeType, extra, originPrincipal,       \
                             decision);                                       \
  PR_END_MACRO







#define CHECK_PRINCIPAL                                                       \
  nsCOMPtr<nsIURI> requestOrigin;                                             \
  PR_BEGIN_MACRO                                                              \
  if (originPrincipal) {                                                      \
      nsCOMPtr<nsIScriptSecurityManager> secMan = aSecMan;                    \
      if (!secMan) {                                                          \
          secMan = do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID);        \
      }                                                                       \
      if (secMan) {                                                           \
          bool isSystem;                                                    \
          nsresult rv = secMan->IsSystemPrincipal(originPrincipal,            \
                                                  &isSystem);                 \
          NS_ENSURE_SUCCESS(rv, rv);                                          \
          if (isSystem) {                                                     \
              *decision = nsIContentPolicy::ACCEPT;                           \
              return NS_OK;                                                   \
          }                                                                   \
      }                                                                       \
      nsresult rv = originPrincipal->GetURI(getter_AddRefs(requestOrigin));   \
      NS_ENSURE_SUCCESS(rv, rv);                                              \
  }                                                                           \
  PR_END_MACRO










inline nsresult
NS_CheckContentLoadPolicy(PRUint32          contentType,
                          nsIURI           *contentLocation,
                          nsIPrincipal     *originPrincipal,
                          nsISupports      *context,
                          const nsACString &mimeType,
                          nsISupports      *extra,
                          PRInt16          *decision,
                          nsIContentPolicy *policyService = nsnull,
                          nsIScriptSecurityManager* aSecMan = nsnull)
{
    CHECK_PRINCIPAL;
    if (policyService) {
        CHECK_CONTENT_POLICY_WITH_SERVICE(ShouldLoad, policyService);
    }
    CHECK_CONTENT_POLICY(ShouldLoad);
}










inline nsresult
NS_CheckContentProcessPolicy(PRUint32          contentType,
                             nsIURI           *contentLocation,
                             nsIPrincipal     *originPrincipal,
                             nsISupports      *context,
                             const nsACString &mimeType,
                             nsISupports      *extra,
                             PRInt16          *decision,
                             nsIContentPolicy *policyService = nsnull,
                             nsIScriptSecurityManager* aSecMan = nsnull)
{
    CHECK_PRINCIPAL;
    if (policyService) {
        CHECK_CONTENT_POLICY_WITH_SERVICE(ShouldProcess, policyService);
    }
    CHECK_CONTENT_POLICY(ShouldProcess);
}

#undef CHECK_CONTENT_POLICY
#undef CHECK_CONTENT_POLICY_WITH_SERVICE
























inline nsIDocShell*
NS_CP_GetDocShellFromContext(nsISupports *aContext)
{
    if (!aContext) {
        return nsnull;
    }

    nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(aContext);

    if (!window) {
        
        
        nsCOMPtr<nsIDocument> doc = do_QueryInterface(aContext);
        if (!doc) {
            
            
            nsCOMPtr<nsIContent> content = do_QueryInterface(aContext);
            if (content) {
                doc = content->OwnerDoc();
            }
        }

        if (doc) {
            if (doc->GetDisplayDocument()) {
                doc = doc->GetDisplayDocument();
            }
            
            window = doc->GetWindow();
        }
    }

    if (!window) {
        return nsnull;
    }

    return window->GetDocShell();
}

#endif 
