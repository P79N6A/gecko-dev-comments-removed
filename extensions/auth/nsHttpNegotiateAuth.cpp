

















































#include <string.h>
#include <stdlib.h>

#include "nsAuth.h"
#include "nsHttpNegotiateAuth.h"

#include "nsIHttpChannel.h"
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
#include "prlog.h"
#include "prmem.h"



static const char kNegotiate[] = "Negotiate";
static const char kNegotiateAuthTrustedURIs[] = "network.negotiate-auth.trusted-uris";
static const char kNegotiateAuthDelegationURIs[] = "network.negotiate-auth.delegation-uris";
static const char kNegotiateAuthAllowProxies[] = "network.negotiate-auth.allow-proxies";
static const char kNegotiateAuthSSPI[] = "network.auth.use-sspi";

#define kNegotiateLen  (sizeof(kNegotiate)-1)



NS_IMETHODIMP
nsHttpNegotiateAuth::GetAuthFlags(PRUint32 *flags)
{
    
    
    
    
    
    
    
    
    
    *flags = CONNECTION_BASED | IDENTITY_IGNORED; 
    return NS_OK;
}








NS_IMETHODIMP
nsHttpNegotiateAuth::ChallengeReceived(nsIHttpChannel *httpChannel,
                                       const char *challenge,
                                       PRBool isProxyAuth,
                                       nsISupports **sessionState,
                                       nsISupports **continuationState,
                                       PRBool *identityInvalid)
{
    nsIAuthModule *module = (nsIAuthModule *) *continuationState;

    *identityInvalid = PR_FALSE;
    if (module)
        return NS_OK;

    nsresult rv;

    nsCOMPtr<nsIURI> uri;
    rv = httpChannel->GetURI(getter_AddRefs(uri));
    if (NS_FAILED(rv))
        return rv;

    PRUint32 req_flags = nsIAuthModule::REQ_DEFAULT;
    nsCAutoString service;

    if (isProxyAuth) {
        if (!TestBoolPref(kNegotiateAuthAllowProxies)) {
            LOG(("nsHttpNegotiateAuth::ChallengeReceived proxy auth blocked\n"));
            return NS_ERROR_ABORT;
        }

        nsCOMPtr<nsIProxiedChannel> proxied =
                do_QueryInterface(httpChannel);
        NS_ENSURE_STATE(proxied);

        nsCOMPtr<nsIProxyInfo> proxyInfo;
        proxied->GetProxyInfo(getter_AddRefs(proxyInfo));
        NS_ENSURE_STATE(proxyInfo);

        proxyInfo->GetHost(service);
    }
    else {
        PRBool allowed = TestPref(uri, kNegotiateAuthTrustedURIs);
        if (!allowed) {
            LOG(("nsHttpNegotiateAuth::ChallengeReceived URI blocked\n"));
            return NS_ERROR_ABORT;
        }

        PRBool delegation = TestPref(uri, kNegotiateAuthDelegationURIs);
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

    rv = module->Init(service.get(), req_flags, nsnull, nsnull, nsnull);

    if (NS_FAILED(rv)) {
        NS_RELEASE(module);
        return rv;
    }

    *continuationState = module;
    return NS_OK;
}

NS_IMPL_ISUPPORTS1(nsHttpNegotiateAuth, nsIHttpAuthenticator)
   






NS_IMETHODIMP
nsHttpNegotiateAuth::GenerateCredentials(nsIHttpChannel *httpChannel,
                                         const char *challenge,
                                         PRBool isProxyAuth,
                                         const PRUnichar *domain,
                                         const PRUnichar *username,
                                         const PRUnichar *password,
                                         nsISupports **sessionState,
                                         nsISupports **continuationState,
                                         char **creds)
{
    
    nsIAuthModule *module = (nsIAuthModule *) *continuationState;
    NS_ENSURE_TRUE(module, NS_ERROR_NOT_INITIALIZED);

    LOG(("nsHttpNegotiateAuth::GenerateCredentials() [challenge=%s]\n", challenge));

    NS_ASSERTION(creds, "null param");

#ifdef DEBUG
    PRBool isGssapiAuth =
        !PL_strncasecmp(challenge, kNegotiate, kNegotiateLen);
    NS_ASSERTION(isGssapiAuth, "Unexpected challenge");
#endif

    
    
    
    
    
    
    
    
    
    unsigned int len = strlen(challenge);

    void *inToken, *outToken;
    PRUint32 inTokenLen, outTokenLen;

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

        
        
        
        if (PL_Base64Decode(challenge, len, (char *) inToken) == NULL) {
            free(inToken);
            return(NS_ERROR_UNEXPECTED);
        }
    }
    else {
        
        
        
        inToken = nsnull;
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

    
    
    
    char *encoded_token = PL_Base64Encode((char *)outToken, outTokenLen, nsnull);

    nsMemory::Free(outToken);

    if (!encoded_token)
        return NS_ERROR_OUT_OF_MEMORY;

    LOG(("  Sending a token of length %d\n", outTokenLen));

    
    *creds = (char *) nsMemory::Alloc(kNegotiateLen + 1 + strlen(encoded_token) + 1);
    if (NS_UNLIKELY(!*creds))
        rv = NS_ERROR_OUT_OF_MEMORY;
    else
        sprintf(*creds, "%s %s", kNegotiate, encoded_token);

    PR_Free(encoded_token);
    return rv;
}

PRBool
nsHttpNegotiateAuth::TestBoolPref(const char *pref)
{
    nsCOMPtr<nsIPrefBranch> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
    if (!prefs)
        return PR_FALSE;

    PRBool val;
    nsresult rv = prefs->GetBoolPref(pref, &val);
    if (NS_FAILED(rv))
        return PR_FALSE;

    return val;
}

PRBool
nsHttpNegotiateAuth::TestPref(nsIURI *uri, const char *pref)
{
    nsCOMPtr<nsIPrefBranch> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
    if (!prefs)
        return PR_FALSE;

    nsCAutoString scheme, host;
    PRInt32 port;

    if (NS_FAILED(uri->GetScheme(scheme)))
        return PR_FALSE;
    if (NS_FAILED(uri->GetAsciiHost(host)))
        return PR_FALSE;
    if (NS_FAILED(uri->GetPort(&port)))
        return PR_FALSE;

    char *hostList;
    if (NS_FAILED(prefs->GetCharPref(pref, &hostList)) || !hostList)
        return PR_FALSE;

    
    
    
    
    
    
    
    
    
    
    

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
            return PR_TRUE;
        if (*end == '\0')
            break;
        start = end + 1;
    }
    
    nsMemory::Free(hostList);
    return PR_FALSE;
}

PRBool
nsHttpNegotiateAuth::MatchesBaseURI(const nsCSubstring &matchScheme,
                                    const nsCSubstring &matchHost,
                                    PRInt32             matchPort,
                                    const char         *baseStart,
                                    const char         *baseEnd)
{
    

    
    const char *hostStart, *schemeEnd = strstr(baseStart, "://");
    if (schemeEnd) {
        
        if (!matchScheme.Equals(Substring(baseStart, schemeEnd)))
            return PR_FALSE;
        hostStart = schemeEnd + 3;
    }
    else
        hostStart = baseStart;

    
    const char *hostEnd = strchr(hostStart, ':');
    if (hostEnd && hostEnd < baseEnd) {
        
        int port = atoi(hostEnd + 1);
        if (matchPort != (PRInt32) port)
            return PR_FALSE;
    }
    else
        hostEnd = baseEnd;


    
    if (hostStart == hostEnd)
        return PR_TRUE;

    PRUint32 hostLen = hostEnd - hostStart;

    
    if (matchHost.Length() < hostLen)
        return PR_FALSE;

    const char *end = matchHost.EndReading();
    if (PL_strncasecmp(end - hostLen, hostStart, hostLen) == 0) {
        
        
        
        if (matchHost.Length() == hostLen ||
            *(end - hostLen) == '.' ||
            *(end - hostLen - 1) == '.')
            return PR_TRUE;
    }

    return PR_FALSE;
}
