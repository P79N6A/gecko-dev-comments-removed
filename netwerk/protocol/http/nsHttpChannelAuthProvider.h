





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
#include "nsIDNSListener.h"
#include "mozilla/Attributes.h"

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
    { return mProxyInfo ? mProxyInfo->Host().get() : nullptr; }

    int32_t     ProxyPort() const
    { return mProxyInfo ? mProxyInfo->Port() : -1; }

    const char *Host() const      { return mHost.get(); }
    int32_t     Port() const      { return mPort; }
    bool        UsingSSL() const  { return mUsingSSL; }

    bool        UsingHttpProxy() const
    { return !!(mProxyInfo && !nsCRT::strcmp(mProxyInfo->Type(), "http")); }

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
    bool     AuthModuleRequiresCanonicalName(nsISupports *state);
    nsresult ResolveHost();

    





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

    void SetDNSQuery(nsICancelable *aQuery) { mDNSQuery = aQuery; }
    void SetCanonicalizedHost(nsACString &aHost) { mCanonicalizedHost = aHost; }

private:
    nsIHttpAuthenticableChannel      *mAuthChannel;  

    nsCOMPtr<nsIURI>                  mURI;
    nsCOMPtr<nsProxyInfo>             mProxyInfo;
    nsCString                         mHost;
    nsCString                         mCanonicalizedHost;
    int32_t                           mPort;
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

    
    
    uint32_t                          mProxyAuth                : 1;
    uint32_t                          mTriedProxyAuth           : 1;
    uint32_t                          mTriedHostAuth            : 1;
    uint32_t                          mSuppressDefensiveAuth    : 1;
    uint32_t                          mResolvedHost             : 1;

    
    class DNSCallback MOZ_FINAL : public nsIDNSListener
    {
        NS_DECL_ISUPPORTS
        NS_DECL_NSIDNSLISTENER

        DNSCallback(nsHttpChannelAuthProvider *authProvider)
            : mAuthProvider(authProvider)
        { }

    private:
        nsRefPtr<nsHttpChannelAuthProvider> mAuthProvider;
    };
    nsCOMPtr<nsICancelable>          mDNSQuery;
};

#endif 
