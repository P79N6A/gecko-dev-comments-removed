














#include <string.h>
#include <stdlib.h>

#include "nsAuth.h"
#include "nsHttpNegotiateAuth.h"

#include "nsIHttpAuthenticableChannel.h"
#include "nsIProxiedChannel.h"
#include "nsIAuthModule.h"
#include "nsIServiceManager.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsIProxyInfo.h"
#include "nsIURI.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsNetCID.h"
#include "plbase64.h"
#include "plstr.h"
#include "prprf.h"
#include "mozilla/Logging.h"
#include "prmem.h"
#include "prnetdb.h"
#include "mozilla/Likely.h"



static const char kNegotiate[] = "Negotiate";
static const char kNegotiateAuthTrustedURIs[] = "network.negotiate-auth.trusted-uris";
static const char kNegotiateAuthDelegationURIs[] = "network.negotiate-auth.delegation-uris";
static const char kNegotiateAuthAllowProxies[] = "network.negotiate-auth.allow-proxies";
static const char kNegotiateAuthAllowNonFqdn[] = "network.negotiate-auth.allow-non-fqdn";
static const char kNegotiateAuthSSPI[] = "network.auth.use-sspi";

#define kNegotiateLen  (sizeof(kNegotiate)-1)



NS_IMETHODIMP
nsHttpNegotiateAuth::GetAuthFlags(uint32_t *flags)
{
    
    
    
    
    
    
    
    
    
    *flags = CONNECTION_BASED | IDENTITY_IGNORED; 
    return NS_OK;
}








NS_IMETHODIMP
nsHttpNegotiateAuth::ChallengeReceived(nsIHttpAuthenticableChannel *authChannel,
                                       const char *challenge,
                                       bool isProxyAuth,
                                       nsISupports **sessionState,
                                       nsISupports **continuationState,
                                       bool *identityInvalid)
{
    nsIAuthModule *module = (nsIAuthModule *) *continuationState;

    *identityInvalid = false;
    if (module)
        return NS_OK;

    nsresult rv;

    nsCOMPtr<nsIURI> uri;
    rv = authChannel->GetURI(getter_AddRefs(uri));
    if (NS_FAILED(rv))
        return rv;

    uint32_t req_flags = nsIAuthModule::REQ_DEFAULT;
    nsAutoCString service;

    if (isProxyAuth) {
        if (!TestBoolPref(kNegotiateAuthAllowProxies)) {
            LOG(("nsHttpNegotiateAuth::ChallengeReceived proxy auth blocked\n"));
            return NS_ERROR_ABORT;
        }

        req_flags |= nsIAuthModule::REQ_PROXY_AUTH;
        nsCOMPtr<nsIProxyInfo> proxyInfo;
        authChannel->GetProxyInfo(getter_AddRefs(proxyInfo));
        NS_ENSURE_STATE(proxyInfo);

        proxyInfo->GetHost(service);
    }
    else {
        bool allowed = TestNonFqdn(uri) ||
                       TestPref(uri, kNegotiateAuthTrustedURIs);
        if (!allowed) {
            LOG(("nsHttpNegotiateAuth::ChallengeReceived URI blocked\n"));
            return NS_ERROR_ABORT;
        }

        bool delegation = TestPref(uri, kNegotiateAuthDelegationURIs);
        if (delegation) {
            LOG(("  using REQ_DELEGATE\n"));
            req_flags |= nsIAuthModule::REQ_DELEGATE;
        }

        rv = uri->GetAsciiHost(service);
        if (NS_FAILED(rv))
            return rv;
    }

    LOG(("  service = %s\n", service.get()));

    
    
    
    
    
    
    
    
    service.Insert("HTTP@", 0);

    const char *contractID;
    if (TestBoolPref(kNegotiateAuthSSPI)) {
	   LOG(("  using negotiate-sspi\n"));
	   contractID = NS_AUTH_MODULE_CONTRACTID_PREFIX "negotiate-sspi";
    }
    else {
	   LOG(("  using negotiate-gss\n"));
	   contractID = NS_AUTH_MODULE_CONTRACTID_PREFIX "negotiate-gss";
    }

    rv = CallCreateInstance(contractID, &module);

    if (NS_FAILED(rv)) {
        LOG(("  Failed to load Negotiate Module \n"));
        return rv;
    }

    rv = module->Init(service.get(), req_flags, nullptr, nullptr, nullptr);

    if (NS_FAILED(rv)) {
        NS_RELEASE(module);
        return rv;
    }

    *continuationState = module;
    return NS_OK;
}

NS_IMPL_ISUPPORTS(nsHttpNegotiateAuth, nsIHttpAuthenticator)
   






NS_IMETHODIMP
nsHttpNegotiateAuth::GenerateCredentials(nsIHttpAuthenticableChannel *authChannel,
                                         const char *challenge,
                                         bool isProxyAuth,
                                         const char16_t *domain,
                                         const char16_t *username,
                                         const char16_t *password,
                                         nsISupports **sessionState,
                                         nsISupports **continuationState,
                                         uint32_t *flags,
                                         char **creds)
{
    
    nsIAuthModule *module = (nsIAuthModule *) *continuationState;
    NS_ENSURE_TRUE(module, NS_ERROR_NOT_INITIALIZED);

    *flags = USING_INTERNAL_IDENTITY;

    LOG(("nsHttpNegotiateAuth::GenerateCredentials() [challenge=%s]\n", challenge));

    NS_ASSERTION(creds, "null param");

#ifdef DEBUG
    bool isGssapiAuth =
        !PL_strncasecmp(challenge, kNegotiate, kNegotiateLen);
    NS_ASSERTION(isGssapiAuth, "Unexpected challenge");
#endif

    
    
    
    
    
    
    
    
    
    unsigned int len = strlen(challenge);

    void *inToken, *outToken;
    uint32_t inTokenLen, outTokenLen;

    if (len > kNegotiateLen) {
        challenge += kNegotiateLen;
        while (*challenge == ' ')
            challenge++;
        len = strlen(challenge);

        
        while (challenge[len - 1] == '=')
            len--;

        inTokenLen = (len * 3)/4;
        inToken = malloc(inTokenLen);
        if (!inToken)
            return (NS_ERROR_OUT_OF_MEMORY);

        
        
        
        if (PL_Base64Decode(challenge, len, (char *) inToken) == nullptr) {
            free(inToken);
            return(NS_ERROR_UNEXPECTED);
        }
    }
    else {
        
        
        
        inToken = nullptr;
        inTokenLen = 0;
    }

    nsresult rv = module->GetNextToken(inToken, inTokenLen, &outToken, &outTokenLen);

    free(inToken);

    if (NS_FAILED(rv))
        return rv;

    if (outTokenLen == 0) {
        LOG(("  No output token to send, exiting"));
        return NS_ERROR_FAILURE;
    }

    
    
    
    char *encoded_token = PL_Base64Encode((char *)outToken, outTokenLen, nullptr);

    free(outToken);

    if (!encoded_token)
        return NS_ERROR_OUT_OF_MEMORY;

    LOG(("  Sending a token of length %d\n", outTokenLen));

    
    *creds = (char *) moz_xmalloc(kNegotiateLen + 1 + strlen(encoded_token) + 1);
    if (MOZ_UNLIKELY(!*creds))
        rv = NS_ERROR_OUT_OF_MEMORY;
    else
        sprintf(*creds, "%s %s", kNegotiate, encoded_token);

    PR_Free(encoded_token);
    return rv;
}

bool
nsHttpNegotiateAuth::TestBoolPref(const char *pref)
{
    nsCOMPtr<nsIPrefBranch> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
    if (!prefs)
        return false;

    bool val;
    nsresult rv = prefs->GetBoolPref(pref, &val);
    if (NS_FAILED(rv))
        return false;

    return val;
}

bool
nsHttpNegotiateAuth::TestNonFqdn(nsIURI *uri)
{
    nsAutoCString host;
    PRNetAddr addr;

    if (!TestBoolPref(kNegotiateAuthAllowNonFqdn))
        return false;

    if (NS_FAILED(uri->GetAsciiHost(host)))
        return false;

    
    return !host.IsEmpty() && host.FindChar('.') == kNotFound &&
           PR_StringToNetAddr(host.BeginReading(), &addr) != PR_SUCCESS;
}

bool
nsHttpNegotiateAuth::TestPref(nsIURI *uri, const char *pref)
{
    nsCOMPtr<nsIPrefBranch> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
    if (!prefs)
        return false;

    nsAutoCString scheme, host;
    int32_t port;

    if (NS_FAILED(uri->GetScheme(scheme)))
        return false;
    if (NS_FAILED(uri->GetAsciiHost(host)))
        return false;
    if (NS_FAILED(uri->GetPort(&port)))
        return false;

    char *hostList;
    if (NS_FAILED(prefs->GetCharPref(pref, &hostList)) || !hostList)
        return false;

    
    
    
    
    
    
    
    
    
    
    

    char *start = hostList, *end;
    for (;;) {
        
        while (*start == ' ' || *start == '\t')
            ++start;
        end = strchr(start, ',');
        if (!end)
            end = start + strlen(start);
        if (start == end)
            break;
        if (MatchesBaseURI(scheme, host, port, start, end))
            return true;
        if (*end == '\0')
            break;
        start = end + 1;
    }
    
    free(hostList);
    return false;
}

bool
nsHttpNegotiateAuth::MatchesBaseURI(const nsCSubstring &matchScheme,
                                    const nsCSubstring &matchHost,
                                    int32_t             matchPort,
                                    const char         *baseStart,
                                    const char         *baseEnd)
{
    

    
    const char *hostStart, *schemeEnd = strstr(baseStart, "://");
    if (schemeEnd) {
        
        if (!matchScheme.Equals(Substring(baseStart, schemeEnd)))
            return false;
        hostStart = schemeEnd + 3;
    }
    else
        hostStart = baseStart;

    
    const char *hostEnd = strchr(hostStart, ':');
    if (hostEnd && hostEnd < baseEnd) {
        
        int port = atoi(hostEnd + 1);
        if (matchPort != (int32_t) port)
            return false;
    }
    else
        hostEnd = baseEnd;


    
    if (hostStart == hostEnd)
        return true;

    uint32_t hostLen = hostEnd - hostStart;

    
    if (matchHost.Length() < hostLen)
        return false;

    const char *end = matchHost.EndReading();
    if (PL_strncasecmp(end - hostLen, hostStart, hostLen) == 0) {
        
        
        
        if (matchHost.Length() == hostLen ||
            *(end - hostLen) == '.' ||
            *(end - hostLen - 1) == '.')
            return true;
    }

    return false;
}
