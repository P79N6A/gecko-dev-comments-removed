





#ifndef nsHttpChannelAuthProvider_h__
#define nsHttpChannelAuthProvider_h__

#include "nsIHttpChannelAuthProvider.h"
#include "nsIAuthPromptCallback.h"
#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsHttpAuthCache.h"
#include "nsProxyInfo.h"
#include "nsCRT.h"

class nsIHttpAuthenticableChannel;
class nsIHttpAuthenticator;
class nsIURI;

namespace mozilla { namespace net {

class nsHttpHandler;

class nsHttpChannelAuthProvider : public nsIHttpChannelAuthProvider
                                , public nsIAuthPromptCallback
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSICANCELABLE
    NS_DECL_NSIHTTPCHANNELAUTHPROVIDER
    NS_DECL_NSIAUTHPROMPTCALLBACK

    nsHttpChannelAuthProvider();
    static void InitializePrefs();
private:
    virtual ~nsHttpChannelAuthProvider();

    const char *ProxyHost() const
    { return mProxyInfo ? mProxyInfo->Host().get() : nullptr; }

    int32_t     ProxyPort() const
    { return mProxyInfo ? mProxyInfo->Port() : -1; }

    const char *Host() const      { return mHost.get(); }
    int32_t     Port() const      { return mPort; }
    bool        UsingSSL() const  { return mUsingSSL; }

    bool        UsingHttpProxy() const
    { return mProxyInfo && (mProxyInfo->IsHTTP() || mProxyInfo->IsHTTPS()); }

    nsresult PrepareForAuthentication(bool proxyAuth);
    nsresult GenCredsAndSetEntry(nsIHttpAuthenticator *, bool proxyAuth,
                                 const char *scheme, const char *host,
                                 int32_t port, const char *dir,
                                 const char *realm, const char *challenge,
                                 const nsHttpAuthIdentity &ident,
                                 nsCOMPtr<nsISupports> &session, char **result);
    nsresult GetAuthenticator(const char *challenge, nsCString &scheme,
                              nsIHttpAuthenticator **auth);
    void     ParseRealm(const char *challenge, nsACString &realm);
    void     GetIdentityFromURI(uint32_t authFlags, nsHttpAuthIdentity&);

    





    nsresult GetCredentials(const char *challenges, bool proxyAuth,
                            nsAFlatCString &creds);
    nsresult GetCredentialsForChallenge(const char *challenge,
                                        const char *scheme,  bool proxyAuth,
                                        nsIHttpAuthenticator *auth,
                                        nsAFlatCString &creds);
    nsresult PromptForIdentity(uint32_t level, bool proxyAuth,
                               const char *realm, const char *authType,
                               uint32_t authFlags, nsHttpAuthIdentity &);

    bool     ConfirmAuth(const nsString &bundleKey, bool doYesNoPrompt);
    void     SetAuthorizationHeader(nsHttpAuthCache *, nsHttpAtom header,
                                    const char *scheme, const char *host,
                                    int32_t port, const char *path,
                                    nsHttpAuthIdentity &ident);
    nsresult GetCurrentPath(nsACString &);
    




    nsresult GetAuthorizationMembers(bool proxyAuth, nsCSubstring& scheme,
                                     const char*& host, int32_t& port,
                                     nsCSubstring& path,
                                     nsHttpAuthIdentity*& ident,
                                     nsISupports**& continuationState);
    




    nsresult ContinueOnAuthAvailable(const nsCSubstring& creds);

    nsresult DoRedirectChannelToHttps();

    





    nsresult ProcessSTSHeader();

    
    
    
    
    bool BlockPrompt();

private:
    nsIHttpAuthenticableChannel      *mAuthChannel;  

    nsCOMPtr<nsIURI>                  mURI;
    nsCOMPtr<nsProxyInfo>             mProxyInfo;
    nsCString                         mHost;
    int32_t                           mPort;
    bool                              mUsingSSL;
    bool                              mIsPrivate;

    nsISupports                      *mProxyAuthContinuationState;
    nsCString                         mProxyAuthType;
    nsISupports                      *mAuthContinuationState;
    nsCString                         mAuthType;
    nsHttpAuthIdentity                mIdent;
    nsHttpAuthIdentity                mProxyIdent;

    
    
    
    nsCOMPtr<nsICancelable>           mAsyncPromptAuthCancelable;
    
    
    
    nsCString                         mCurrentChallenge;
    
    
    
    nsCString                         mRemainingChallenges;

    
    
    uint32_t                          mProxyAuth                : 1;
    uint32_t                          mTriedProxyAuth           : 1;
    uint32_t                          mTriedHostAuth            : 1;
    uint32_t                          mSuppressDefensiveAuth    : 1;

    nsRefPtr<nsHttpHandler>           mHttpHandler;  

    
    
    
    static uint32_t                   sAuthAllowPref;
};

}} 

#endif 
