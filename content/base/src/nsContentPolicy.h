





#ifndef __nsContentPolicy_h__
#define __nsContentPolicy_h__

#include "nsIContentPolicy.h"
#include "nsCategoryCache.h"





class nsContentPolicy : public nsIContentPolicy
{
 public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSICONTENTPOLICY

    nsContentPolicy();
    virtual ~nsContentPolicy();
 private:
    
    nsCategoryCache<nsIContentPolicy> mPolicies;

    
    typedef
    NS_STDCALL_FUNCPROTO(nsresult, CPMethod, nsIContentPolicy,
                         ShouldProcess,
                         (uint32_t, nsIURI*, nsIURI*, nsISupports*,
                           const nsACString &, nsISupports*, nsIPrincipal*,
                           int16_t*));

    
    
    nsresult CheckPolicy(CPMethod policyMethod, uint32_t contentType,
                         nsIURI *aURI, nsIURI *origURI,
                         nsISupports *requestingContext,
                         const nsACString &mimeGuess, nsISupports *extra,
                         nsIPrincipal *requestPrincipal,
                         int16_t *decision);
};

nsresult
NS_NewContentPolicy(nsIContentPolicy **aResult);

#endif 
