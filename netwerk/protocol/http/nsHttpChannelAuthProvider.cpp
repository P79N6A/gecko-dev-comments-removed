





#include "nsHttpChannelAuthProvider.h"
#include "nsNetUtil.h"
#include "nsHttpHandler.h"
#include "nsIHttpAuthenticator.h"
#include "nsIAuthPrompt2.h"
#include "nsIAuthPromptProvider.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsEscape.h"
#include "nsAuthInformationHolder.h"
#include "nsIStringBundle.h"
#include "nsIPrompt.h"
#include "nsIAuthModule.h"
#include "nsIDNSService.h"
#include "nsNetCID.h"
#include "nsIDNSRecord.h"

nsHttpChannelAuthProvider::nsHttpChannelAuthProvider()
    : mAuthChannel(nullptr)
    , mProxyAuthContinuationState(nullptr)
    , mAuthContinuationState(nullptr)
    , mProxyAuth(false)
    , mTriedProxyAuth(false)
    , mTriedHostAuth(false)
    , mSuppressDefensiveAuth(false)
    , mResolvedHost(0)
{
    
    nsHttpHandler *handler = gHttpHandler;
    NS_ADDREF(handler);
}

nsHttpChannelAuthProvider::~nsHttpChannelAuthProvider()
{
    NS_ASSERTION(!mAuthChannel, "Disconnect wasn't called");

    
    nsHttpHandler *handler = gHttpHandler;
    NS_RELEASE(handler);
}

NS_IMETHODIMP
nsHttpChannelAuthProvider::Init(nsIHttpAuthenticableChannel *channel)
{
    NS_ASSERTION(channel, "channel expected!");

    mAuthChannel = channel;

    nsresult rv = mAuthChannel->GetURI(getter_AddRefs(mURI));
    if (NS_FAILED(rv)) return rv;

    mAuthChannel->GetIsSSL(&mUsingSSL);
    if (NS_FAILED(rv)) return rv;

    rv = mURI->GetAsciiHost(mHost);
    if (NS_FAILED(rv)) return rv;

    
    if (mHost.IsEmpty())
        return NS_ERROR_MALFORMED_URI;

    rv = mURI->GetPort(&mPort);
    if (NS_FAILED(rv)) return rv;

    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannelAuthProvider::ProcessAuthentication(uint32_t httpStatus,
                                                 bool     SSLConnectFailed)
{
    LOG(("nsHttpChannelAuthProvider::ProcessAuthentication "
         "[this=%p channel=%p code=%u SSLConnectFailed=%d]\n",
         this, mAuthChannel, httpStatus, SSLConnectFailed));

    NS_ASSERTION(mAuthChannel, "Channel not initialized");
    
    mCanonicalizedHost.Truncate();
    mResolvedHost = 0;

    nsCOMPtr<nsIProxyInfo> proxyInfo;
    nsresult rv = mAuthChannel->GetProxyInfo(getter_AddRefs(proxyInfo));
    if (NS_FAILED(rv)) return rv;
    if (proxyInfo) {
        mProxyInfo = do_QueryInterface(proxyInfo);
        if (!mProxyInfo) return NS_ERROR_NO_INTERFACE;
    }

    uint32_t loadFlags;
    rv = mAuthChannel->GetLoadFlags(&loadFlags);
    if (NS_FAILED(rv)) return rv;

    nsAutoCString challenges;
    mProxyAuth = (httpStatus == 407);

    
    if ((loadFlags & nsIRequest::LOAD_ANONYMOUS) &&
        (!mProxyAuth || !UsingHttpProxy())) {
        LOG(("Skipping authentication for anonymous non-proxy request\n"));
        return NS_ERROR_NOT_AVAILABLE;
    }

    rv = PrepareForAuthentication(mProxyAuth);
    if (NS_FAILED(rv))
        return rv;

    if (mProxyAuth) {
        
        
        
        
        
        
        if (!UsingHttpProxy()) {
            LOG(("rejecting 407 when proxy server not configured!\n"));
            return NS_ERROR_UNEXPECTED;
        }
        if (UsingSSL() && !SSLConnectFailed) {
            
            
            
            LOG(("rejecting 407 from origin server!\n"));
            return NS_ERROR_UNEXPECTED;
        }
        rv = mAuthChannel->GetProxyChallenges(challenges);
    }
    else
        rv = mAuthChannel->GetWWWChallenges(challenges);
    if (NS_FAILED(rv)) return rv;

    nsAutoCString creds;
    rv = GetCredentials(challenges.get(), mProxyAuth, creds);
    if (rv == NS_ERROR_IN_PROGRESS)
        return rv;
    if (NS_FAILED(rv))
        LOG(("unable to authenticate\n"));
    else {
        
        if (mProxyAuth)
            rv = mAuthChannel->SetProxyCredentials(creds);
        else
            rv = mAuthChannel->SetWWWCredentials(creds);
    }
    return rv;
}

NS_IMETHODIMP
nsHttpChannelAuthProvider::AddAuthorizationHeaders()
{
    LOG(("nsHttpChannelAuthProvider::AddAuthorizationHeaders? "
         "[this=%p channel=%p]\n", this, mAuthChannel));

    NS_ASSERTION(mAuthChannel, "Channel not initialized");

    nsCOMPtr<nsIProxyInfo> proxyInfo;
    nsresult rv = mAuthChannel->GetProxyInfo(getter_AddRefs(proxyInfo));
    if (NS_FAILED(rv)) return rv;
    if (proxyInfo) {
        mProxyInfo = do_QueryInterface(proxyInfo);
        if (!mProxyInfo) return NS_ERROR_NO_INTERFACE;
    }

    uint32_t loadFlags;
    rv = mAuthChannel->GetLoadFlags(&loadFlags);
    if (NS_FAILED(rv)) return rv;

    
    nsHttpAuthCache *authCache = gHttpHandler->AuthCache();

    
    const char *proxyHost = ProxyHost();
    if (proxyHost && UsingHttpProxy())
        SetAuthorizationHeader(authCache, nsHttp::Proxy_Authorization,
                               "http", proxyHost, ProxyPort(),
                               nullptr, 
                               mProxyIdent);

    if (loadFlags & nsIRequest::LOAD_ANONYMOUS) {
        LOG(("Skipping Authorization header for anonymous load\n"));
        return NS_OK;
    }

    
    nsAutoCString path, scheme;
    if (NS_SUCCEEDED(GetCurrentPath(path)) &&
        NS_SUCCEEDED(mURI->GetScheme(scheme))) {
        SetAuthorizationHeader(authCache, nsHttp::Authorization,
                               scheme.get(),
                               Host(),
                               Port(),
                               path.get(),
                               mIdent);
    }

    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannelAuthProvider::CheckForSuperfluousAuth()
{
    LOG(("nsHttpChannelAuthProvider::CheckForSuperfluousAuth? "
         "[this=%p channel=%p]\n", this, mAuthChannel));

    NS_ASSERTION(mAuthChannel, "Channel not initialized");

    
    
    
    
    
    if (!ConfirmAuth(NS_LITERAL_STRING("SuperfluousAuth"), true)) {
        
        
        mAuthChannel->Cancel(NS_ERROR_ABORT);
        return NS_ERROR_ABORT;
    }
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannelAuthProvider::Cancel(nsresult status)
{
    NS_ASSERTION(mAuthChannel, "Channel not initialized");

    if (mAsyncPromptAuthCancelable) {
        mAsyncPromptAuthCancelable->Cancel(status);
        mAsyncPromptAuthCancelable = nullptr;
    }
    if (mDNSQuery) {
        mDNSQuery->Cancel(status);
        mDNSQuery = nullptr;
    }
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannelAuthProvider::Disconnect(nsresult status)
{
    mAuthChannel = nullptr;

    if (mAsyncPromptAuthCancelable) {
        mAsyncPromptAuthCancelable->Cancel(status);
        mAsyncPromptAuthCancelable = nullptr;
    }
    if (mDNSQuery) {
        mDNSQuery->Cancel(status);
        mDNSQuery = nullptr;
    }

    NS_IF_RELEASE(mProxyAuthContinuationState);
    NS_IF_RELEASE(mAuthContinuationState);

    return NS_OK;
}


static void
ParseUserDomain(PRUnichar *buf,
                const PRUnichar **user,
                const PRUnichar **domain)
{
    PRUnichar *p = buf;
    while (*p && *p != '\\') ++p;
    if (!*p)
        return;
    *p = '\0';
    *domain = buf;
    *user = p + 1;
}


static void
SetIdent(nsHttpAuthIdentity &ident,
         uint32_t authFlags,
         PRUnichar *userBuf,
         PRUnichar *passBuf)
{
    const PRUnichar *user = userBuf;
    const PRUnichar *domain = nullptr;

    if (authFlags & nsIHttpAuthenticator::IDENTITY_INCLUDES_DOMAIN)
        ParseUserDomain(userBuf, &user, &domain);

    ident.Set(domain, user, passBuf);
}


static void
GetAuthPrompt(nsIInterfaceRequestor *ifreq, bool proxyAuth,
              nsIAuthPrompt2 **result)
{
    if (!ifreq)
        return;

    uint32_t promptReason;
    if (proxyAuth)
        promptReason = nsIAuthPromptProvider::PROMPT_PROXY;
    else
        promptReason = nsIAuthPromptProvider::PROMPT_NORMAL;

    nsCOMPtr<nsIAuthPromptProvider> promptProvider = do_GetInterface(ifreq);
    if (promptProvider)
        promptProvider->GetAuthPrompt(promptReason,
                                      NS_GET_IID(nsIAuthPrompt2),
                                      reinterpret_cast<void**>(result));
    else
        NS_QueryAuthPrompt2(ifreq, result);
}


nsresult
nsHttpChannelAuthProvider::GenCredsAndSetEntry(nsIHttpAuthenticator *auth,
                                               bool                  proxyAuth,
                                               const char           *scheme,
                                               const char           *host,
                                               int32_t               port,
                                               const char           *directory,
                                               const char           *realm,
                                               const char           *challenge,
                                               const nsHttpAuthIdentity &ident,
                                               nsCOMPtr<nsISupports>    &sessionState,
                                               char                    **result)
{
    nsresult rv;
    uint32_t authFlags;

    rv = auth->GetAuthFlags(&authFlags);
    if (NS_FAILED(rv)) return rv;

    nsISupports *ss = sessionState;

    
    
    
    nsISupports **continuationState;

    if (proxyAuth) {
        continuationState = &mProxyAuthContinuationState;
    } else {
        continuationState = &mAuthContinuationState;
    }

    uint32_t generateFlags;
    rv = auth->GenerateCredentials(mAuthChannel,
                                   challenge,
                                   proxyAuth,
                                   ident.Domain(),
                                   ident.User(),
                                   ident.Password(),
                                   &ss,
                                   &*continuationState,
                                   &generateFlags,
                                   result);

    sessionState.swap(ss);
    if (NS_FAILED(rv)) return rv;

    
#ifdef DEBUG
    LOG(("generated creds: %s\n", *result));
#endif

    
    
    bool saveCreds =
        0 != (authFlags & nsIHttpAuthenticator::REUSABLE_CREDENTIALS);
    bool saveChallenge =
        0 != (authFlags & nsIHttpAuthenticator::REUSABLE_CHALLENGE);

    bool saveIdentity =
        0 == (generateFlags & nsIHttpAuthenticator::USING_INTERNAL_IDENTITY);

    
    nsHttpAuthCache *authCache = gHttpHandler->AuthCache();

    
    
    
    
    
    
    rv = authCache->SetAuthEntry(scheme, host, port, directory, realm,
                                 saveCreds ? *result : nullptr,
                                 saveChallenge ? challenge : nullptr,
                                 saveIdentity ? &ident : nullptr,
                                 sessionState);
    return rv;
}

nsresult
nsHttpChannelAuthProvider::PrepareForAuthentication(bool proxyAuth)
{
    LOG(("nsHttpChannelAuthProvider::PrepareForAuthentication "
         "[this=%p channel=%p]\n", this, mAuthChannel));

    if (!proxyAuth) {
        
        
        NS_IF_RELEASE(mProxyAuthContinuationState);
        LOG(("  proxy continuation state has been reset"));
    }

    if (!UsingHttpProxy() || mProxyAuthType.IsEmpty())
        return NS_OK;

    
    

    nsAutoCString contractId;
    contractId.Assign(NS_HTTP_AUTHENTICATOR_CONTRACTID_PREFIX);
    contractId.Append(mProxyAuthType);

    nsresult rv;
    nsCOMPtr<nsIHttpAuthenticator> precedingAuth =
        do_GetService(contractId.get(), &rv);
    if (NS_FAILED(rv))
        return rv;

    uint32_t precedingAuthFlags;
    rv = precedingAuth->GetAuthFlags(&precedingAuthFlags);
    if (NS_FAILED(rv))
        return rv;

    if (!(precedingAuthFlags & nsIHttpAuthenticator::REQUEST_BASED)) {
        nsAutoCString challenges;
        rv = mAuthChannel->GetProxyChallenges(challenges);
        if (NS_FAILED(rv)) {
            
            
            rv = mAuthChannel->SetProxyCredentials(EmptyCString());
            if (NS_FAILED(rv)) return rv;
            LOG(("  cleared proxy authorization header"));
        }
    }

    return NS_OK;
}

bool
nsHttpChannelAuthProvider::AuthModuleRequiresCanonicalName(nsISupports *state)
{
    if (mResolvedHost || !state)
        return false;

    nsCOMPtr<nsIAuthModule> module = do_QueryInterface(state);
    if (!module)
        return false;

    uint32_t flags;
    if (NS_FAILED(module->GetModuleProperties(&flags)))
        return false;

    if (!(flags & nsIAuthModule::CANONICAL_NAME_REQUIRED))
        return false;
    
    LOG(("nsHttpChannelAuthProvider::AuthModuleRequiresCanoncialName "
         "this=%p\n", this));
    return true;
}

nsresult
nsHttpChannelAuthProvider::ResolveHost()
{
    mResolvedHost = 1;

    nsresult rv;
    static NS_DEFINE_CID(kDNSServiceCID, NS_DNSSERVICE_CID);
        nsCOMPtr<nsIDNSService> dns = do_GetService(kDNSServiceCID, &rv);
    if (NS_FAILED(rv))
        return rv;

    nsCOMPtr<nsIURI> uri;
    rv = mAuthChannel->GetURI(getter_AddRefs(uri));
    if (NS_FAILED(rv))
        return rv;
    nsAutoCString host;
    rv = uri->GetAsciiHost(host);
    if (NS_FAILED(rv))
        return rv;

    LOG(("nsHttpChannelAuthProvider::ResolveHost() this=%p "
         "looking up canoncial of %s\n", this, host.get()));
    nsRefPtr<DNSCallback> dnsCallback = new DNSCallback(this);
    rv = dns->AsyncResolve(host,
                           nsIDNSService::RESOLVE_CANONICAL_NAME,
                           dnsCallback, NS_GetCurrentThread(),
                           getter_AddRefs(mDNSQuery));
    return rv;
}

nsresult
nsHttpChannelAuthProvider::GetCredentials(const char     *challenges,
                                          bool            proxyAuth,
                                          nsAFlatCString &creds)
{
    nsCOMPtr<nsIHttpAuthenticator> auth;
    nsAutoCString challenge;

    nsCString authType; 
                        

    
    
    nsISupports **currentContinuationState;
    nsCString *currentAuthType;

    if (proxyAuth) {
        currentContinuationState = &mProxyAuthContinuationState;
        currentAuthType = &mProxyAuthType;
    } else {
        currentContinuationState = &mAuthContinuationState;
        currentAuthType = &mAuthType;
    }

    nsresult rv = NS_ERROR_NOT_AVAILABLE;
    bool gotCreds = false;

    
    for (const char *eol = challenges - 1; eol; ) {
        const char *p = eol + 1;

        
        if ((eol = strchr(p, '\n')) != nullptr)
            challenge.Assign(p, eol - p);
        else
            challenge.Assign(p);

        rv = GetAuthenticator(challenge.get(), authType, getter_AddRefs(auth));
        if (NS_SUCCEEDED(rv)) {
            
            
            
            
            
            
            if (!currentAuthType->IsEmpty() && authType != *currentAuthType)
                continue;

            
            
            
            
            
            
            
            
            
            
            
            
            
            
            rv = GetCredentialsForChallenge(challenge.get(), authType.get(),
                                            proxyAuth, auth, creds);
            if (NS_SUCCEEDED(rv)) {
                gotCreds = true;
                *currentAuthType = authType;

                break;
            }
            else if (rv == NS_ERROR_IN_PROGRESS) {
                
                
                
                
                mCurrentChallenge = challenge;
                mRemainingChallenges = eol ? eol+1 : nullptr;
                return rv;
            }

            
            NS_IF_RELEASE(*currentContinuationState);
            currentAuthType->Truncate();
        }
    }

    if (!gotCreds && !currentAuthType->IsEmpty()) {
        
        
        currentAuthType->Truncate();
        NS_IF_RELEASE(*currentContinuationState);

        rv = GetCredentials(challenges, proxyAuth, creds);
    }

    return rv;
}

nsresult
nsHttpChannelAuthProvider::GetAuthorizationMembers(bool                 proxyAuth,
                                                   nsCSubstring&        scheme,
                                                   const char*&         host,
                                                   int32_t&             port,
                                                   nsCSubstring&        path,
                                                   nsHttpAuthIdentity*& ident,
                                                   nsISupports**&       continuationState)
{
    if (proxyAuth) {
        NS_ASSERTION (UsingHttpProxy(),
                      "proxyAuth is true, but no HTTP proxy is configured!");

        host = ProxyHost();
        port = ProxyPort();
        ident = &mProxyIdent;
        scheme.AssignLiteral("http");

        continuationState = &mProxyAuthContinuationState;
    }
    else {
        host = Host();
        port = Port();
        ident = &mIdent;

        nsresult rv;
        rv = GetCurrentPath(path);
        if (NS_FAILED(rv)) return rv;

        rv = mURI->GetScheme(scheme);
        if (NS_FAILED(rv)) return rv;

        continuationState = &mAuthContinuationState;
    }

    return NS_OK;
}

nsresult
nsHttpChannelAuthProvider::GetCredentialsForChallenge(const char *challenge,
                                                      const char *authType,
                                                      bool        proxyAuth,
                                                      nsIHttpAuthenticator *auth,
                                                      nsAFlatCString     &creds)
{
    LOG(("nsHttpChannelAuthProvider::GetCredentialsForChallenge "
         "[this=%p channel=%p proxyAuth=%d challenges=%s]\n",
        this, mAuthChannel, proxyAuth, challenge));

    
    nsHttpAuthCache *authCache = gHttpHandler->AuthCache();

    uint32_t authFlags;
    nsresult rv = auth->GetAuthFlags(&authFlags);
    if (NS_FAILED(rv)) return rv;

    nsAutoCString realm;
    ParseRealm(challenge, realm);

    
    
    
    
    






    
    
    
    const char *host;
    int32_t port;
    nsHttpAuthIdentity *ident;
    nsAutoCString path, scheme;
    bool identFromURI = false;
    nsISupports **continuationState;

    rv = GetAuthorizationMembers(proxyAuth, scheme, host, port,
                                 path, ident, continuationState);
    if (NS_FAILED(rv)) return rv;

    if (!proxyAuth) {
        
        
        if (mIdent.IsEmpty()) {
            GetIdentityFromURI(authFlags, mIdent);
            identFromURI = !mIdent.IsEmpty();
        }
    }

    
    
    
    
    
    
    nsHttpAuthEntry *entry = nullptr;
    authCache->GetAuthEntryForDomain(scheme.get(), host, port,
                                     realm.get(), &entry);

    
    
    nsCOMPtr<nsISupports> sessionStateGrip;
    if (entry)
        sessionStateGrip = entry->mMetaData;

    
    bool identityInvalid;
    nsISupports *sessionState = sessionStateGrip;
    rv = auth->ChallengeReceived(mAuthChannel,
                                 challenge,
                                 proxyAuth,
                                 &sessionState,
                                 &*continuationState,
                                 &identityInvalid);
    sessionStateGrip.swap(sessionState);
    if (NS_FAILED(rv)) return rv;

    LOG(("  identity invalid = %d\n", identityInvalid));

    if (identityInvalid) {
        if (entry) {
            if (ident->Equals(entry->Identity())) {
                if (!identFromURI) {
                    LOG(("  clearing bad auth cache entry\n"));
                    
                    
                    authCache->ClearAuthEntry(scheme.get(), host,
                                              port, realm.get());
                    entry = nullptr;
                    ident->Clear();
                }
            }
            else if (!identFromURI ||
                     nsCRT::strcmp(ident->User(),
                                   entry->Identity().User()) == 0) {
                LOG(("  taking identity from auth cache\n"));
                
                
                
                
                
                
                ident->Set(entry->Identity());
                identFromURI = false;
                if (entry->Creds()[0] != '\0') {
                    LOG(("    using cached credentials!\n"));
                    creds.Assign(entry->Creds());
                    return entry->AddPath(path.get());
                }
            }
        }
        else if (!identFromURI) {
            
            
            ident->Clear();
        }

        if (!entry && ident->IsEmpty()) {
            uint32_t level = nsIAuthPrompt2::LEVEL_NONE;
            if (mUsingSSL)
                level = nsIAuthPrompt2::LEVEL_SECURE;
            else if (authFlags & nsIHttpAuthenticator::IDENTITY_ENCRYPTED)
                level = nsIAuthPrompt2::LEVEL_PW_ENCRYPTED;

            
            
            rv = PromptForIdentity(level, proxyAuth, realm.get(),
                                   authType, authFlags, *ident);
            if (NS_FAILED(rv)) return rv;
            identFromURI = false;
        }
    }

    if (identFromURI) {
        
        
        if (!ConfirmAuth(NS_LITERAL_STRING("AutomaticAuth"), false)) {
            
            
            mAuthChannel->Cancel(NS_ERROR_ABORT);
            
            
            
            
            return NS_ERROR_ABORT;
        }
    }

    if (AuthModuleRequiresCanonicalName(*continuationState)) {
        nsresult rv = ResolveHost();
        if (NS_SUCCEEDED(rv))
            return NS_ERROR_IN_PROGRESS;
        return rv;
    }

    
    
    
    
    
    
    
    
    
    
    nsXPIDLCString result;
    rv = GenCredsAndSetEntry(auth, proxyAuth, scheme.get(), host, port,
                             path.get(), realm.get(), challenge, *ident,
                             sessionStateGrip, getter_Copies(result));
    if (NS_SUCCEEDED(rv))
        creds = result;
    return rv;
}

inline void
GetAuthType(const char *challenge, nsCString &authType)
{
    const char *p;

    
    if ((p = strchr(challenge, ' ')) != nullptr)
        authType.Assign(challenge, p - challenge);
    else
        authType.Assign(challenge);
}

nsresult
nsHttpChannelAuthProvider::GetAuthenticator(const char            *challenge,
                                            nsCString             &authType,
                                            nsIHttpAuthenticator **auth)
{
    LOG(("nsHttpChannelAuthProvider::GetAuthenticator [this=%p channel=%p]\n",
        this, mAuthChannel));

    GetAuthType(challenge, authType);

    
    ToLowerCase(authType);

    nsAutoCString contractid;
    contractid.Assign(NS_HTTP_AUTHENTICATOR_CONTRACTID_PREFIX);
    contractid.Append(authType);

    return CallGetService(contractid.get(), auth);
}

void
nsHttpChannelAuthProvider::GetIdentityFromURI(uint32_t            authFlags,
                                              nsHttpAuthIdentity &ident)
{
    LOG(("nsHttpChannelAuthProvider::GetIdentityFromURI [this=%p channel=%p]\n",
        this, mAuthChannel));

    nsAutoString userBuf;
    nsAutoString passBuf;

    
    nsAutoCString buf;
    mURI->GetUsername(buf);
    if (!buf.IsEmpty()) {
        NS_UnescapeURL(buf);
        CopyASCIItoUTF16(buf, userBuf);
        mURI->GetPassword(buf);
        if (!buf.IsEmpty()) {
            NS_UnescapeURL(buf);
            CopyASCIItoUTF16(buf, passBuf);
        }
    }

    if (!userBuf.IsEmpty()) {
        SetIdent(ident, authFlags, (PRUnichar *) userBuf.get(),
                 (PRUnichar *) passBuf.get());
    }
}

void
nsHttpChannelAuthProvider::ParseRealm(const char *challenge,
                                      nsACString &realm)
{
    
    
    
    
    
    
    
    
    
    const char *p = PL_strcasestr(challenge, "realm=");
    if (p) {
        bool has_quote = false;
        p += 6;
        if (*p == '"') {
            has_quote = true;
            p++;
        }

        const char *end = p;
        while (*end && has_quote) {
           
           
            if (*end == '"' && end[-1] != '\\')
                break;
            ++end;
        }

        if (!has_quote)
            end = strchr(p, ' ');
        if (end)
            realm.Assign(p, end - p);
        else
            realm.Assign(p);
    }
}


class nsHTTPAuthInformation : public nsAuthInformationHolder {
public:
    nsHTTPAuthInformation(uint32_t aFlags, const nsString& aRealm,
                          const nsCString& aAuthType)
        : nsAuthInformationHolder(aFlags, aRealm, aAuthType) {}

    void SetToHttpAuthIdentity(uint32_t authFlags,
                               nsHttpAuthIdentity& identity);
};

void
nsHTTPAuthInformation::SetToHttpAuthIdentity(uint32_t authFlags,
                                             nsHttpAuthIdentity& identity)
{
    identity.Set(Domain().get(), User().get(), Password().get());
}

nsresult
nsHttpChannelAuthProvider::PromptForIdentity(uint32_t            level,
                                             bool                proxyAuth,
                                             const char         *realm,
                                             const char         *authType,
                                             uint32_t            authFlags,
                                             nsHttpAuthIdentity &ident)
{
    LOG(("nsHttpChannelAuthProvider::PromptForIdentity [this=%p channel=%p]\n",
        this, mAuthChannel));

    nsresult rv;

    nsCOMPtr<nsIInterfaceRequestor> callbacks;
    rv = mAuthChannel->GetNotificationCallbacks(getter_AddRefs(callbacks));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsILoadGroup> loadGroup;
    rv = mAuthChannel->GetLoadGroup(getter_AddRefs(loadGroup));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIAuthPrompt2> authPrompt;
    GetAuthPrompt(callbacks, proxyAuth, getter_AddRefs(authPrompt));
    if (!authPrompt && loadGroup) {
        nsCOMPtr<nsIInterfaceRequestor> cbs;
        loadGroup->GetNotificationCallbacks(getter_AddRefs(cbs));
        GetAuthPrompt(cbs, proxyAuth, getter_AddRefs(authPrompt));
    }
    if (!authPrompt)
        return NS_ERROR_NO_INTERFACE;

    
    NS_ConvertASCIItoUTF16 realmU(realm);

    
    uint32_t promptFlags = 0;
    if (proxyAuth)
    {
        promptFlags |= nsIAuthInformation::AUTH_PROXY;
        if (mTriedProxyAuth)
            promptFlags |= nsIAuthInformation::PREVIOUS_FAILED;
        mTriedProxyAuth = true;
    }
    else {
        promptFlags |= nsIAuthInformation::AUTH_HOST;
        if (mTriedHostAuth)
            promptFlags |= nsIAuthInformation::PREVIOUS_FAILED;
        mTriedHostAuth = true;
    }

    if (authFlags & nsIHttpAuthenticator::IDENTITY_INCLUDES_DOMAIN)
        promptFlags |= nsIAuthInformation::NEED_DOMAIN;

    nsRefPtr<nsHTTPAuthInformation> holder =
        new nsHTTPAuthInformation(promptFlags, realmU,
                                  nsDependentCString(authType));
    if (!holder)
        return NS_ERROR_OUT_OF_MEMORY;

    nsCOMPtr<nsIChannel> channel(do_QueryInterface(mAuthChannel, &rv));
    if (NS_FAILED(rv)) return rv;

    rv =
        authPrompt->AsyncPromptAuth(channel, this, nullptr, level, holder,
                                    getter_AddRefs(mAsyncPromptAuthCancelable));

    if (NS_SUCCEEDED(rv)) {
        
        
        rv = NS_ERROR_IN_PROGRESS;
    }
    else {
        
        bool retval = false;
        rv = authPrompt->PromptAuth(channel, level, holder, &retval);
        if (NS_FAILED(rv))
            return rv;

        if (!retval)
            rv = NS_ERROR_ABORT;
        else
            holder->SetToHttpAuthIdentity(authFlags, ident);
    }

    
    if (!proxyAuth)
        mSuppressDefensiveAuth = true;

    return rv;
}

NS_IMETHODIMP nsHttpChannelAuthProvider::OnAuthAvailable(nsISupports *aContext,
                                                         nsIAuthInformation *aAuthInfo)
{
    LOG(("nsHttpChannelAuthProvider::OnAuthAvailable [this=%p channel=%p]",
        this, mAuthChannel));

    mAsyncPromptAuthCancelable = nullptr;
    if (!mAuthChannel)
        return NS_OK;

    nsresult rv;

    const char *host;
    int32_t port;
    nsHttpAuthIdentity *ident;
    nsAutoCString path, scheme;
    nsISupports **continuationState;
    rv = GetAuthorizationMembers(mProxyAuth, scheme, host, port,
                                 path, ident, continuationState);
    if (NS_FAILED(rv))
        OnAuthCancelled(aContext, false);

    nsAutoCString realm;
    ParseRealm(mCurrentChallenge.get(), realm);

    nsHttpAuthCache *authCache = gHttpHandler->AuthCache();
    nsHttpAuthEntry *entry = nullptr;
    authCache->GetAuthEntryForDomain(scheme.get(), host, port,
                                     realm.get(), &entry);

    nsCOMPtr<nsISupports> sessionStateGrip;
    if (entry)
        sessionStateGrip = entry->mMetaData;

    nsAuthInformationHolder* holder =
            static_cast<nsAuthInformationHolder*>(aAuthInfo);
    if (holder) {
        ident->Set(holder->Domain().get(),
                   holder->User().get(),
                   holder->Password().get());
    }

    if (AuthModuleRequiresCanonicalName(*continuationState)) {
        rv = ResolveHost();
        if (NS_FAILED(rv))
            OnAuthCancelled(aContext, true);
        return NS_OK;
    }

    nsAutoCString unused;
    nsCOMPtr<nsIHttpAuthenticator> auth;
    rv = GetAuthenticator(mCurrentChallenge.get(), unused,
                          getter_AddRefs(auth));
    if (NS_FAILED(rv)) {
        NS_ASSERTION(false, "GetAuthenticator failed");
        OnAuthCancelled(aContext, true);
        return NS_OK;
    }

    nsXPIDLCString creds;
    rv = GenCredsAndSetEntry(auth, mProxyAuth,
                             scheme.get(), host, port, path.get(),
                             realm.get(), mCurrentChallenge.get(), *ident,
                             sessionStateGrip, getter_Copies(creds));

    mCurrentChallenge.Truncate();
    if (NS_FAILED(rv)) {
        OnAuthCancelled(aContext, true);
        return NS_OK;
    }

    return ContinueOnAuthAvailable(creds);
}

NS_IMETHODIMP nsHttpChannelAuthProvider::OnAuthCancelled(nsISupports *aContext,
                                                         bool        userCancel)
{
    LOG(("nsHttpChannelAuthProvider::OnAuthCancelled [this=%p channel=%p]",
        this, mAuthChannel));

    mAsyncPromptAuthCancelable = nullptr;
    if (!mAuthChannel)
        return NS_OK;

    if (userCancel) {
        if (!mRemainingChallenges.IsEmpty()) {
            
            nsresult rv;

            nsAutoCString creds;
            rv = GetCredentials(mRemainingChallenges.get(), mProxyAuth, creds);
            if (NS_SUCCEEDED(rv)) {
                
                
                
                mRemainingChallenges.Truncate();
                return ContinueOnAuthAvailable(creds);
            }
            else if (rv == NS_ERROR_IN_PROGRESS) {
                
                
                
                return NS_OK;
            }

            
        }

        mRemainingChallenges.Truncate();
    }

    mAuthChannel->OnAuthCancelled(userCancel);

    return NS_OK;
}

nsresult
nsHttpChannelAuthProvider::ContinueOnAuthAvailable(const nsCSubstring& creds)
{
    nsresult rv;
    if (mProxyAuth)
        rv = mAuthChannel->SetProxyCredentials(creds);
    else
        rv = mAuthChannel->SetWWWCredentials(creds);
    if (NS_FAILED(rv)) return rv;

    
    
    
    
    mRemainingChallenges.Truncate();

    mAuthChannel->OnAuthAvailable();

    return NS_OK;
}

bool
nsHttpChannelAuthProvider::ConfirmAuth(const nsString &bundleKey,
                                       bool            doYesNoPrompt)
{
    
    
    
    

    uint32_t loadFlags;
    nsresult rv = mAuthChannel->GetLoadFlags(&loadFlags);
    if (NS_FAILED(rv))
        return true;

    if (mSuppressDefensiveAuth ||
        !(loadFlags & nsIChannel::LOAD_INITIAL_DOCUMENT_URI))
        return true;

    nsAutoCString userPass;
    rv = mURI->GetUserPass(userPass);
    if (NS_FAILED(rv) ||
        (userPass.Length() < gHttpHandler->PhishyUserPassLength()))
        return true;

    
    
    

    nsCOMPtr<nsIStringBundleService> bundleService =
            do_GetService(NS_STRINGBUNDLE_CONTRACTID);
    if (!bundleService)
        return true;

    nsCOMPtr<nsIStringBundle> bundle;
    bundleService->CreateBundle(NECKO_MSGS_URL, getter_AddRefs(bundle));
    if (!bundle)
        return true;

    nsAutoCString host;
    rv = mURI->GetHost(host);
    if (NS_FAILED(rv))
        return true;

    nsAutoCString user;
    rv = mURI->GetUsername(user);
    if (NS_FAILED(rv))
        return true;

    NS_ConvertUTF8toUTF16 ucsHost(host), ucsUser(user);
    const PRUnichar *strs[2] = { ucsHost.get(), ucsUser.get() };

    nsXPIDLString msg;
    bundle->FormatStringFromName(bundleKey.get(), strs, 2, getter_Copies(msg));
    if (!msg)
        return true;

    nsCOMPtr<nsIInterfaceRequestor> callbacks;
    rv = mAuthChannel->GetNotificationCallbacks(getter_AddRefs(callbacks));
    if (NS_FAILED(rv))
        return true;

    nsCOMPtr<nsILoadGroup> loadGroup;
    rv = mAuthChannel->GetLoadGroup(getter_AddRefs(loadGroup));
    if (NS_FAILED(rv))
        return true;

    nsCOMPtr<nsIPrompt> prompt;
    NS_QueryNotificationCallbacks(callbacks, loadGroup, NS_GET_IID(nsIPrompt),
                                  getter_AddRefs(prompt));
    if (!prompt)
        return true;

    
    mSuppressDefensiveAuth = true;

    bool confirmed;
    if (doYesNoPrompt) {
        int32_t choice;
        
        
        bool checkState = false;
        rv = prompt->ConfirmEx(nullptr, msg,
                               nsIPrompt::BUTTON_POS_1_DEFAULT +
                               nsIPrompt::STD_YES_NO_BUTTONS,
                               nullptr, nullptr, nullptr, nullptr,
                               &checkState, &choice);
        if (NS_FAILED(rv))
            return true;

        confirmed = choice == 0;
    }
    else {
        rv = prompt->Confirm(nullptr, msg, &confirmed);
        if (NS_FAILED(rv))
            return true;
    }

    return confirmed;
}

void
nsHttpChannelAuthProvider::SetAuthorizationHeader(nsHttpAuthCache    *authCache,
                                                  nsHttpAtom          header,
                                                  const char         *scheme,
                                                  const char         *host,
                                                  int32_t             port,
                                                  const char         *path,
                                                  nsHttpAuthIdentity &ident)
{
    nsHttpAuthEntry *entry = nullptr;
    nsresult rv;

    
    
    
    nsISupports **continuationState;

    if (header == nsHttp::Proxy_Authorization) {
        continuationState = &mProxyAuthContinuationState;
    } else {
        continuationState = &mAuthContinuationState;
    }

    rv = authCache->GetAuthEntryForPath(scheme, host, port, path, &entry);
    if (NS_SUCCEEDED(rv)) {
        
        
        
        
        
        
        
        if (header == nsHttp::Authorization && entry->Domain()[0] == '\0') {
            GetIdentityFromURI(0, ident);
            
            
            if (nsCRT::strcmp(ident.User(), entry->User()) == 0)
                ident.Clear();
        }
        bool identFromURI;
        if (ident.IsEmpty()) {
            ident.Set(entry->Identity());
            identFromURI = false;
        }
        else
            identFromURI = true;

        nsXPIDLCString temp;
        const char *creds     = entry->Creds();
        const char *challenge = entry->Challenge();
        
        
        
        
        if ((!creds[0] || identFromURI) && challenge[0]) {
            nsCOMPtr<nsIHttpAuthenticator> auth;
            nsAutoCString unused;
            rv = GetAuthenticator(challenge, unused, getter_AddRefs(auth));
            if (NS_SUCCEEDED(rv)) {
                bool proxyAuth = (header == nsHttp::Proxy_Authorization);
                rv = GenCredsAndSetEntry(auth, proxyAuth, scheme, host, port,
                                         path, entry->Realm(), challenge, ident,
                                         entry->mMetaData, getter_Copies(temp));
                if (NS_SUCCEEDED(rv))
                    creds = temp.get();

                
                
                NS_IF_RELEASE(*continuationState);
            }
        }
        if (creds[0]) {
            LOG(("   adding \"%s\" request header\n", header.get()));
            if (header == nsHttp::Proxy_Authorization)
                mAuthChannel->SetProxyCredentials(nsDependentCString(creds));
            else
                mAuthChannel->SetWWWCredentials(nsDependentCString(creds));

            
            
            
            
            if (header == nsHttp::Authorization)
                mSuppressDefensiveAuth = true;
        }
        else
            ident.Clear(); 
    }
}

nsresult
nsHttpChannelAuthProvider::GetCurrentPath(nsACString &path)
{
    nsresult rv;
    nsCOMPtr<nsIURL> url = do_QueryInterface(mURI);
    if (url)
        rv = url->GetDirectory(path);
    else
        rv = mURI->GetPath(path);
    return rv;
}

nsresult
nsHttpChannelAuthProvider::GetAsciiHostForAuth(nsACString &host)
{
    if (!mCanonicalizedHost.IsEmpty()) {
        LOG(("nsHttpChannelAuthProvider::GetAsciiHostForAuth"
             " this=%p host is %s\n", this, mCanonicalizedHost.get()));
        host = mCanonicalizedHost;
        return NS_OK;
    }
    
    
    nsresult rv;
    nsCOMPtr<nsIURI> uri;
    rv = mAuthChannel->GetURI(getter_AddRefs(uri));
    if (NS_FAILED(rv))
        return rv;
    return uri->GetAsciiHost(host);
}

NS_IMETHODIMP
nsHttpChannelAuthProvider::DNSCallback::OnLookupComplete(nsICancelable *request,
                                                         nsIDNSRecord  *record,
                                                         nsresult       rv)
{
    nsCString cname;
    mAuthProvider->SetDNSQuery(nullptr);

    LOG(("nsHttpChannelAuthProvider::OnLookupComplete this=%p "
         "rv=%X\n", mAuthProvider.get(), rv));

    if (NS_SUCCEEDED(rv))
        rv = record->GetCanonicalName(cname);

    if (NS_SUCCEEDED(rv)) {
        LOG(("nsHttpChannelAuthProvider::OnLookupComplete this=%p "
             "resolved to %s\n", mAuthProvider.get(), cname.get()));
        mAuthProvider->SetCanonicalizedHost(cname);
    }
    else {
        LOG(("nsHttpChannelAuthProvider::OnLookupComplete this=%p "
             "GetCanonicalName failed\n", mAuthProvider.get()));
    }

    
    mAuthProvider->OnAuthAvailable(nullptr, nullptr);
    return NS_OK;
}

NS_IMPL_ISUPPORTS3(nsHttpChannelAuthProvider, nsICancelable,
                   nsIHttpChannelAuthProvider, nsIAuthPromptCallback)
NS_IMPL_THREADSAFE_ISUPPORTS1(nsHttpChannelAuthProvider::DNSCallback,
                              nsIDNSListener)
