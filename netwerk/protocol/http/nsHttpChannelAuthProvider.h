








































#ifndef nsHttpChannelAuthProvider_h__
#define nsHttpChannelAuthProvider_h__

#include "nsHttp.h"
#include "nsIHttpChannelAuthProvider.h"
#include "nsIAuthPromptCallback.h"
#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsIHttpAuthenticableChannel.h"
#include "nsIURI.h"
#include "nsHttpAuthCache.h"
#include "nsProxyInfo.h"

class nsIHttpAuthenticator;

class nsHttpChannelAuthProvider : public nsIHttpChannelAuthProvider
                                , public nsIAuthPromptCallback
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSICANCELABLE
    NS_DECL_NSIHTTPCHANNELAUTHPROVIDER
    NS_DECL_NSIAUTHPROMPTCALLBACK

    nsHttpChannelAuthProvider();
    virtual ~nsHttpChannelAuthProvider();

private:
    const char *ProxyHost() const
    { return mProxyInfo ? mProxyInfo->Host().get() : nsnull; }

    PRInt32     ProxyPort() const
    { return mProxyInfo ? mProxyInfo->Port() : -1; }

    const char *Host() const      { return mHost.get(); }
    PRInt32     Port() const      { return mPort; }
    bool        UsingSSL() const  { return mUsingSSL; }

    bool        UsingHttpProxy() const
    { return !!(mProxyInfo && !nsCRT::strcmp(mProxyInfo->Type(), "http")); }

    nsresult PrepareForAuthentication(bool proxyAuth);
    nsresult GenCredsAndSetEntry(nsIHttpAuthenticator *, bool proxyAuth,
                                 const char *scheme, const char *host,
                                 PRInt32 port, const char *dir,
                                 const char *realm, const char *challenge,
                                 const nsHttpAuthIdentity &ident,
                                 nsCOMPtr<nsISupports> &session, char **result);
    nsresult GetAuthenticator(const char *challenge, nsCString &scheme,
                              nsIHttpAuthenticator **auth);
    void     ParseRealm(const char *challenge, nsACString &realm);
    void     GetIdentityFromURI(PRUint32 authFlags, nsHttpAuthIdentity&);
    





    nsresult GetCredentials(const char *challenges, bool proxyAuth,
                            nsAFlatCString &creds);
    nsresult GetCredentialsForChallenge(const char *challenge,
                                        const char *scheme,  bool proxyAuth,
                                        nsIHttpAuthenticator *auth,
                                        nsAFlatCString &creds);
    nsresult PromptForIdentity(PRUint32 level, bool proxyAuth,
                               const char *realm, const char *authType,
                               PRUint32 authFlags, nsHttpAuthIdentity &);

    bool     ConfirmAuth(const nsString &bundleKey, bool doYesNoPrompt);
    void     SetAuthorizationHeader(nsHttpAuthCache *, nsHttpAtom header,
                                    const char *scheme, const char *host,
                                    PRInt32 port, const char *path,
                                    nsHttpAuthIdentity &ident);
    nsresult GetCurrentPath(nsACString &);
    




    nsresult GetAuthorizationMembers(bool proxyAuth, nsCSubstring& scheme,
                                     const char*& host, PRInt32& port,
                                     nsCSubstring& path,
                                     nsHttpAuthIdentity*& ident,
                                     nsISupports**& continuationState);
    




    nsresult ContinueOnAuthAvailable(const nsCSubstring& creds);

    nsresult DoRedirectChannelToHttps();

    





    nsresult ProcessSTSHeader();

private:
    nsIHttpAuthenticableChannel      *mAuthChannel;  

    nsCOMPtr<nsIURI>                  mURI;
    nsCOMPtr<nsProxyInfo>             mProxyInfo;
    nsCString                         mHost;
    PRInt32                           mPort;
    bool                              mUsingSSL;

    nsISupports                      *mProxyAuthContinuationState;
    nsCString                         mProxyAuthType;
    nsISupports                      *mAuthContinuationState;
    nsCString                         mAuthType;
    nsHttpAuthIdentity                mIdent;
    nsHttpAuthIdentity                mProxyIdent;

    
    
    
    nsCOMPtr<nsICancelable>           mAsyncPromptAuthCancelable;
    
    
    
    nsCString                         mCurrentChallenge;
    
    
    
    nsCString                         mRemainingChallenges;

    
    
    PRUint32                          mProxyAuth                : 1;
    PRUint32                          mTriedProxyAuth           : 1;
    PRUint32                          mTriedHostAuth            : 1;
    PRUint32                          mSuppressDefensiveAuth    : 1;
};

#endif 
