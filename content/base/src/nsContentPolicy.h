






































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
                         (PRUint32, nsIURI*, nsIURI*, nsISupports*,
                           const nsACString &, nsISupports*, PRInt16*));

    
    
    nsresult CheckPolicy(CPMethod policyMethod, PRUint32 contentType,
                         nsIURI *aURI, nsIURI *origURI,
                         nsISupports *requestingContext,
                         const nsACString &mimeGuess, nsISupports *extra,
                         PRInt16 *decision);
};

nsresult
NS_NewContentPolicy(nsIContentPolicy **aResult);

#endif 
