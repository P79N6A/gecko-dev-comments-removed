













































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

nsHttpChannelAuthProvider::nsHttpChannelAuthProvider()
    : mAuthChannel(nsnull)
    , mProxyAuthContinuationState(nsnull)
    , mAuthContinuationState(nsnull)
    , mProxyAuth(PR_FALSE)
    , mTriedProxyAuth(PR_FALSE)
    , mTriedHostAuth(PR_FALSE)
    , mSuppressDefensiveAuth(PR_FALSE)
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
nsHttpChannelAuthProvider::ProcessAuthentication(PRUint32 httpStatus,
                                                 PRBool   SSLConnectFailed)
{
    LOG(("nsHttpChannelAuthProvider::ProcessAuthentication "
         "[this=%p channel=%p code=%u SSLConnectFailed=%d]\n",
         this, mAuthChannel, httpStatus, SSLConnectFailed));

    NS_ASSERTION(mAuthChannel, "Channel not initialized");

    nsCOMPtr<nsIProxyInfo> proxyInfo;
    nsresult rv = mAuthChannel->GetProxyInfo(getter_AddRefs(proxyInfo));
    if (NS_FAILED(rv)) return rv;
    if (proxyInfo) {
        mProxyInfo = do_QueryInterface(proxyInfo);
        if (!mProxyInfo) return NS_ERROR_NO_INTERFACE;
    }

    PRUint32 loadFlags;
    rv = mAuthChannel->GetLoadFlags(&loadFlags);
    if (NS_FAILED(rv)) return rv;

    if (loadFlags & nsIRequest::LOAD_ANONYMOUS) {
        return NS_ERROR_NOT_AVAILABLE;
    }

    nsCAutoString challenges;
    mProxyAuth = (httpStatus == 407);

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

    nsCAutoString creds;
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

    PRUint32 loadFlags;
    rv = mAuthChannel->GetLoadFlags(&loadFlags);
    if (NS_FAILED(rv)) return rv;

    if (loadFlags & nsIRequest::LOAD_ANONYMOUS) {
        return NS_OK;
    }

    
    nsHttpAuthCache *authCache = gHttpHandler->AuthCache();

    
    const char *proxyHost = ProxyHost();
    if (proxyHost && UsingHttpProxy())
        SetAuthorizationHeader(authCache, nsHttp::Proxy_Authorization,
                               "http", proxyHost, ProxyPort(),
                               nsnull, 
                               mProxyIdent);

    
    nsCAutoString path, scheme;
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

    
    
    
    
    
    if (!ConfirmAuth(NS_LITERAL_STRING("SuperfluousAuth"), PR_TRUE)) {
        
        
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
        mAsyncPromptAuthCancelable = nsnull;
    }
    return NS_OK;
}

NS_IMETHODIMP
nsHttpChannelAuthProvider::Disconnect(nsresult status)
{
    mAuthChannel = nsnull;

    if (mAsyncPromptAuthCancelable) {
        mAsyncPromptAuthCancelable->Cancel(status);
        mAsyncPromptAuthCancelable = nsnull;
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
         PRUint32 authFlags,
         PRUnichar *userBuf,
         PRUnichar *passBuf)
{
    const PRUnichar *user = userBuf;
    const PRUnichar *domain = nsnull;

    if (authFlags & nsIHttpAuthenticator::IDENTITY_INCLUDES_DOMAIN)
        ParseUserDomain(userBuf, &user, &domain);

    ident.Set(domain, user, passBuf);
}


static void
GetAuthPrompt(nsIInterfaceRequestor *ifreq, PRBool proxyAuth,
              nsIAuthPrompt2 **result)
{
    if (!ifreq)
        return;

    PRUint32 promptReason;
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
                                               PRBool                proxyAuth,
                                               const char           *scheme,
                                               const char           *host,
                                               PRInt32               port,
                                               const char           *directory,
                                               const char           *realm,
                                               const char           *challenge,
                                               const nsHttpAuthIdentity &ident,
                                               nsCOMPtr<nsISupports>    &sessionState,
                                               char                    **result)
{
    nsresult rv;
    PRUint32 authFlags;

    rv = auth->GetAuthFlags(&authFlags);
    if (NS_FAILED(rv)) return rv;

    nsISupports *ss = sessionState;

    
    
    
    nsISupports **continuationState;

    if (proxyAuth) {
        continuationState = &mProxyAuthContinuationState;
    } else {
        continuationState = &mAuthContinuationState;
    }

    PRUint32 generateFlags;
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

    
    
    PRBool saveCreds =
        0 != (authFlags & nsIHttpAuthenticator::REUSABLE_CREDENTIALS);
    PRBool saveChallenge =
        0 != (authFlags & nsIHttpAuthenticator::REUSABLE_CHALLENGE);

    PRBool saveIdentity =
        0 == (generateFlags & nsIHttpAuthenticator::USING_INTERNAL_IDENTITY);

    
    nsHttpAuthCache *authCache = gHttpHandler->AuthCache();

    
    
    
    
    
    
    rv = authCache->SetAuthEntry(scheme, host, port, directory, realm,
                                 saveCreds ? *result : nsnull,
                                 saveChallenge ? challenge : nsnull,
                                 saveIdentity ? &ident : nsnull,
                                 sessionState);
    return rv;
}

nsresult
nsHttpChannelAuthProvider::PrepareForAuthentication(PRBool proxyAuth)
{
    LOG(("nsHttpChannelAuthProvider::PrepareForAuthentication "
         "[this=%p channel=%p]\n", this, mAuthChannel));

    if (!proxyAuth) {
        
        
        NS_IF_RELEASE(mProxyAuthContinuationState);
        LOG(("  proxy continuation state has been reset"));
    }

    if (!UsingHttpProxy() || mProxyAuthType.IsEmpty())
        return NS_OK;

    
    

    nsCAutoString contractId;
    contractId.Assign(NS_HTTP_AUTHENTICATOR_CONTRACTID_PREFIX);
    contractId.Append(mProxyAuthType);

    nsresult rv;
    nsCOMPtr<nsIHttpAuthenticator> precedingAuth =
        do_GetService(contractId.get(), &rv);
    if (NS_FAILED(rv))
        return rv;

    PRUint32 precedingAuthFlags;
    rv = precedingAuth->GetAuthFlags(&precedingAuthFlags);
    if (NS_FAILED(rv))
        return rv;

    if (!(precedingAuthFlags & nsIHttpAuthenticator::REQUEST_BASED)) {
        nsCAutoString challenges;
        rv = mAuthChannel->GetProxyChallenges(challenges);
        if (NS_FAILED(rv)) {
            
            
            rv = mAuthChannel->SetProxyCredentials(EmptyCString());
            if (NS_FAILED(rv)) return rv;
            LOG(("  cleared proxy authorization header"));
        }
    }

    return NS_OK;
}

nsresult
nsHttpChannelAuthProvider::GetCredentials(const char     *challenges,
                                          PRBool          proxyAuth,
                                          nsAFlatCString &creds)
{
    nsCOMPtr<nsIHttpAuthenticator> auth;
    nsCAutoString challenge;

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
    PRBool gotCreds = PR_FALSE;

    
    for (const char *eol = challenges - 1; eol; ) {
        const char *p = eol + 1;

        
        if ((eol = strchr(p, '\n')) != nsnull)
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
                gotCreds = PR_TRUE;
                *currentAuthType = authType;

                break;
            }
            else if (rv == NS_ERROR_IN_PROGRESS) {
                
                
                
                
                mCurrentChallenge = challenge;
                mRemainingChallenges = eol ? eol+1 : nsnull;
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
nsHttpChannelAuthProvider::GetAuthorizationMembers(PRBool               proxyAuth,
                                                   nsCSubstring&        scheme,
                                                   const char*&         host,
                                                   PRInt32&             port,
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
                                                      PRBool      proxyAuth,
                                                      nsIHttpAuthenticator *auth,
                                                      nsAFlatCString     &creds)
{
    LOG(("nsHttpChannelAuthProvider::GetCredentialsForChallenge "
         "[this=%p channel=%p proxyAuth=%d challenges=%s]\n",
        this, mAuthChannel, proxyAuth, challenge));

    
    nsHttpAuthCache *authCache = gHttpHandler->AuthCache();

    PRUint32 authFlags;
    nsresult rv = auth->GetAuthFlags(&authFlags);
    if (NS_FAILED(rv)) return rv;

    nsCAutoString realm;
    ParseRealm(challenge, realm);

    
    
    
    
    






    
    
    
    const char *host;
    PRInt32 port;
    nsHttpAuthIdentity *ident;
    nsCAutoString path, scheme;
    PRBool identFromURI = PR_FALSE;
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

    
    
    
    
    
    
    nsHttpAuthEntry *entry = nsnull;
    authCache->GetAuthEntryForDomain(scheme.get(), host, port,
                                     realm.get(), &entry);

    
    
    nsCOMPtr<nsISupports> sessionStateGrip;
    if (entry)
        sessionStateGrip = entry->mMetaData;

    
    PRBool identityInvalid;
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
                LOG(("  clearing bad auth cache entry\n"));
                
                
                authCache->ClearAuthEntry(scheme.get(), host,
                                          port, realm.get());
                entry = nsnull;
                ident->Clear();
            }
            else if (!identFromURI ||
                     nsCRT::strcmp(ident->User(),
                                   entry->Identity().User()) == 0) {
                LOG(("  taking identity from auth cache\n"));
                
                
                
                
                
                
                ident->Set(entry->Identity());
                identFromURI = PR_FALSE;
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
            PRUint32 level = nsIAuthPrompt2::LEVEL_NONE;
            if (mUsingSSL)
                level = nsIAuthPrompt2::LEVEL_SECURE;
            else if (authFlags & nsIHttpAuthenticator::IDENTITY_ENCRYPTED)
                level = nsIAuthPrompt2::LEVEL_PW_ENCRYPTED;

            
            
            rv = PromptForIdentity(level, proxyAuth, realm.get(),
                                   authType, authFlags, *ident);
            if (NS_FAILED(rv)) return rv;
            identFromURI = PR_FALSE;
        }
    }

    if (identFromURI) {
        
        
        if (!ConfirmAuth(NS_LITERAL_STRING("AutomaticAuth"), PR_FALSE)) {
            
            
            mAuthChannel->Cancel(NS_ERROR_ABORT);
            
            
            
            
            return NS_ERROR_ABORT;
        }
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

    
    if ((p = strchr(challenge, ' ')) != nsnull)
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

    nsCAutoString contractid;
    contractid.Assign(NS_HTTP_AUTHENTICATOR_CONTRACTID_PREFIX);
    contractid.Append(authType);

    return CallGetService(contractid.get(), auth);
}

void
nsHttpChannelAuthProvider::GetIdentityFromURI(PRUint32            authFlags,
                                              nsHttpAuthIdentity &ident)
{
    LOG(("nsHttpChannelAuthProvider::GetIdentityFromURI [this=%p channel=%p]\n",
        this, mAuthChannel));

    nsAutoString userBuf;
    nsAutoString passBuf;

    
    nsCAutoString buf;
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
        PRBool has_quote = PR_FALSE;
        p += 6;
        if (*p == '"') {
            has_quote = PR_TRUE;
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
    nsHTTPAuthInformation(PRUint32 aFlags, const nsString& aRealm,
                          const nsCString& aAuthType)
        : nsAuthInformationHolder(aFlags, aRealm, aAuthType) {}

    void SetToHttpAuthIdentity(PRUint32 authFlags,
                               nsHttpAuthIdentity& identity);
};

void
nsHTTPAuthInformation::SetToHttpAuthIdentity(PRUint32 authFlags,
                                             nsHttpAuthIdentity& identity)
{
    identity.Set(Domain().get(), User().get(), Password().get());
}

nsresult
nsHttpChannelAuthProvider::PromptForIdentity(PRUint32            level,
                                             PRBool              proxyAuth,
                                             const char         *realm,
                                             const char         *authType,
                                             PRUint32            authFlags,
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

    
    PRUint32 promptFlags = 0;
    if (proxyAuth)
    {
        promptFlags |= nsIAuthInformation::AUTH_PROXY;
        if (mTriedProxyAuth)
            promptFlags |= nsIAuthInformation::PREVIOUS_FAILED;
        mTriedProxyAuth = PR_TRUE;
    }
    else {
        promptFlags |= nsIAuthInformation::AUTH_HOST;
        if (mTriedHostAuth)
            promptFlags |= nsIAuthInformation::PREVIOUS_FAILED;
        mTriedHostAuth = PR_TRUE;
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
        authPrompt->AsyncPromptAuth(channel, this, nsnull, level, holder,
                                    getter_AddRefs(mAsyncPromptAuthCancelable));

    if (NS_SUCCEEDED(rv)) {
        
        
        rv = NS_ERROR_IN_PROGRESS;
    }
    else {
        
        PRBool retval = PR_FALSE;
        rv = authPrompt->PromptAuth(channel, level, holder, &retval);
        if (NS_FAILED(rv))
            return rv;

        if (!retval)
            rv = NS_ERROR_ABORT;
        else
            holder->SetToHttpAuthIdentity(authFlags, ident);
    }

    
    if (!proxyAuth)
        mSuppressDefensiveAuth = PR_TRUE;

    return rv;
}

NS_IMETHODIMP nsHttpChannelAuthProvider::OnAuthAvailable(nsISupports *aContext,
                                                         nsIAuthInformation *aAuthInfo)
{
    LOG(("nsHttpChannelAuthProvider::OnAuthAvailable [this=%p channel=%p]",
        this, mAuthChannel));

    mAsyncPromptAuthCancelable = nsnull;
    if (!mAuthChannel)
        return NS_OK;

    nsresult rv;

    const char *host;
    PRInt32 port;
    nsHttpAuthIdentity *ident;
    nsCAutoString path, scheme;
    nsISupports **continuationState;
    rv = GetAuthorizationMembers(mProxyAuth, scheme, host, port,
                                 path, ident, continuationState);
    if (NS_FAILED(rv))
        OnAuthCancelled(aContext, PR_FALSE);

    nsCAutoString realm;
    ParseRealm(mCurrentChallenge.get(), realm);

    nsHttpAuthCache *authCache = gHttpHandler->AuthCache();
    nsHttpAuthEntry *entry = nsnull;
    authCache->GetAuthEntryForDomain(scheme.get(), host, port,
                                     realm.get(), &entry);

    nsCOMPtr<nsISupports> sessionStateGrip;
    if (entry)
        sessionStateGrip = entry->mMetaData;

    nsAuthInformationHolder* holder =
            static_cast<nsAuthInformationHolder*>(aAuthInfo);
    ident->Set(holder->Domain().get(),
               holder->User().get(),
               holder->Password().get());

    nsCAutoString unused;
    nsCOMPtr<nsIHttpAuthenticator> auth;
    rv = GetAuthenticator(mCurrentChallenge.get(), unused,
                          getter_AddRefs(auth));
    if (NS_FAILED(rv)) {
        NS_ASSERTION(PR_FALSE, "GetAuthenticator failed");
        OnAuthCancelled(aContext, PR_TRUE);
        return NS_OK;
    }

    nsXPIDLCString creds;
    rv = GenCredsAndSetEntry(auth, mProxyAuth,
                             scheme.get(), host, port, path.get(),
                             realm.get(), mCurrentChallenge.get(), *ident,
                             sessionStateGrip, getter_Copies(creds));

    mCurrentChallenge.Truncate();
    if (NS_FAILED(rv)) {
        OnAuthCancelled(aContext, PR_TRUE);
        return NS_OK;
    }

    return ContinueOnAuthAvailable(creds);
}

NS_IMETHODIMP nsHttpChannelAuthProvider::OnAuthCancelled(nsISupports *aContext,
                                                         PRBool      userCancel)
{
    LOG(("nsHttpChannelAuthProvider::OnAuthCancelled [this=%p channel=%p]",
        this, mAuthChannel));

    mAsyncPromptAuthCancelable = nsnull;
    if (!mAuthChannel)
        return NS_OK;

    if (userCancel) {
        if (!mRemainingChallenges.IsEmpty()) {
            
            nsresult rv;

            nsCAutoString creds;
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

PRBool
nsHttpChannelAuthProvider::ConfirmAuth(const nsString &bundleKey,
                                       PRBool          doYesNoPrompt)
{
    
    
    
    

    PRUint32 loadFlags;
    nsresult rv = mAuthChannel->GetLoadFlags(&loadFlags);
    if (NS_FAILED(rv)) return rv;

    if (mSuppressDefensiveAuth ||
        !(loadFlags & nsIChannel::LOAD_INITIAL_DOCUMENT_URI))
        return PR_TRUE;

    nsCAutoString userPass;
    rv = mURI->GetUserPass(userPass);
    if (NS_FAILED(rv) ||
        (userPass.Length() < gHttpHandler->PhishyUserPassLength()))
        return PR_TRUE;

    
    
    

    nsCOMPtr<nsIStringBundleService> bundleService =
            do_GetService(NS_STRINGBUNDLE_CONTRACTID);
    if (!bundleService)
        return PR_TRUE;

    nsCOMPtr<nsIStringBundle> bundle;
    bundleService->CreateBundle(NECKO_MSGS_URL, getter_AddRefs(bundle));
    if (!bundle)
        return PR_TRUE;

    nsCAutoString host;
    rv = mURI->GetHost(host);
    if (NS_FAILED(rv))
        return PR_TRUE;

    nsCAutoString user;
    rv = mURI->GetUsername(user);
    if (NS_FAILED(rv))
        return PR_TRUE;

    NS_ConvertUTF8toUTF16 ucsHost(host), ucsUser(user);
    const PRUnichar *strs[2] = { ucsHost.get(), ucsUser.get() };

    nsXPIDLString msg;
    bundle->FormatStringFromName(bundleKey.get(), strs, 2, getter_Copies(msg));
    if (!msg)
        return PR_TRUE;

    nsCOMPtr<nsIInterfaceRequestor> callbacks;
    rv = mAuthChannel->GetNotificationCallbacks(getter_AddRefs(callbacks));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsILoadGroup> loadGroup;
    rv = mAuthChannel->GetLoadGroup(getter_AddRefs(loadGroup));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIPrompt> prompt;
    NS_QueryNotificationCallbacks(callbacks, loadGroup, NS_GET_IID(nsIPrompt),
                                  getter_AddRefs(prompt));
    if (!prompt)
        return PR_TRUE;

    
    mSuppressDefensiveAuth = PR_TRUE;

    PRBool confirmed;
    if (doYesNoPrompt) {
        PRInt32 choice;
        
        
        PRBool checkState = PR_FALSE;
        rv = prompt->ConfirmEx(nsnull, msg,
                               nsIPrompt::BUTTON_POS_1_DEFAULT +
                               nsIPrompt::STD_YES_NO_BUTTONS,
                               nsnull, nsnull, nsnull, nsnull,
                               &checkState, &choice);
        if (NS_FAILED(rv))
            return PR_TRUE;

        confirmed = choice == 0;
    }
    else {
        rv = prompt->Confirm(nsnull, msg, &confirmed);
        if (NS_FAILED(rv))
            return PR_TRUE;
    }

    return confirmed;
}

void
nsHttpChannelAuthProvider::SetAuthorizationHeader(nsHttpAuthCache    *authCache,
                                                  nsHttpAtom          header,
                                                  const char         *scheme,
                                                  const char         *host,
                                                  PRInt32             port,
                                                  const char         *path,
                                                  nsHttpAuthIdentity &ident)
{
    nsHttpAuthEntry *entry = nsnull;
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
        PRBool identFromURI;
        if (ident.IsEmpty()) {
            ident.Set(entry->Identity());
            identFromURI = PR_FALSE;
        }
        else
            identFromURI = PR_TRUE;

        nsXPIDLCString temp;
        const char *creds     = entry->Creds();
        const char *challenge = entry->Challenge();
        
        
        
        
        if ((!creds[0] || identFromURI) && challenge[0]) {
            nsCOMPtr<nsIHttpAuthenticator> auth;
            nsCAutoString unused;
            rv = GetAuthenticator(challenge, unused, getter_AddRefs(auth));
            if (NS_SUCCEEDED(rv)) {
                PRBool proxyAuth = (header == nsHttp::Proxy_Authorization);
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
                mSuppressDefensiveAuth = PR_TRUE;
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

NS_IMPL_ISUPPORTS3(nsHttpChannelAuthProvider, nsICancelable,
                   nsIHttpChannelAuthProvider, nsIAuthPromptCallback)
