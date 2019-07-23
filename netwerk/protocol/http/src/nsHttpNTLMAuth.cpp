





































#include <stdlib.h>
#include "nsHttp.h"
#include "nsHttpNTLMAuth.h"
#include "nsIComponentManager.h"
#include "nsIAuthModule.h"
#include "nsCOMPtr.h"
#include "plbase64.h"



#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsIServiceManager.h"
#include "nsIHttpChannel.h"
#include "nsIURI.h"

static const char kAllowProxies[] = "network.automatic-ntlm-auth.allow-proxies";
static const char kTrustedURIs[]  = "network.automatic-ntlm-auth.trusted-uris";





static PRBool
MatchesBaseURI(const nsCSubstring &matchScheme,
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

static PRBool
TestPref(nsIURI *uri, const char *pref)
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

static PRBool
CanUseSysNTLM(nsIHttpChannel *channel, PRBool isProxyAuth)
{
    

    nsCOMPtr<nsIPrefBranch> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
    if (!prefs)
        return PR_FALSE;

    PRBool val;
    if (isProxyAuth) {
        if (NS_FAILED(prefs->GetBoolPref(kAllowProxies, &val)))
            val = PR_FALSE;
        LOG(("sys-ntlm allowed for proxy: %d\n", val));
        return val;
    }
    else {
        nsCOMPtr<nsIURI> uri;
        channel->GetURI(getter_AddRefs(uri));
        if (uri && TestPref(uri, kTrustedURIs)) {
            LOG(("sys-ntlm allowed for host\n"));
            return PR_TRUE;
        }
    }

    return PR_FALSE;
}



class nsNTLMSessionState : public nsISupports 
{
public:
    NS_DECL_ISUPPORTS
};
NS_IMPL_ISUPPORTS0(nsNTLMSessionState)



NS_IMPL_ISUPPORTS1(nsHttpNTLMAuth, nsIHttpAuthenticator)

NS_IMETHODIMP
nsHttpNTLMAuth::ChallengeReceived(nsIHttpChannel *channel,
                                  const char     *challenge,
                                  PRBool          isProxyAuth,
                                  nsISupports   **sessionState,
                                  nsISupports   **continuationState,
                                  PRBool         *identityInvalid)
{
    LOG(("nsHttpNTLMAuth::ChallengeReceived [ss=%p cs=%p]\n",
         *sessionState, *continuationState));

    

    *identityInvalid = PR_FALSE;
    
    if (PL_strcasecmp(challenge, "NTLM") == 0) {
        nsCOMPtr<nsISupports> module;
        
        
        
        
        PRBool trySysNTLM = (*sessionState == nsnull);

        
        
        
        
        
        
        
        
        
        
        if (trySysNTLM && !*continuationState && CanUseSysNTLM(channel, isProxyAuth)) {
            module = do_CreateInstance(NS_AUTH_MODULE_CONTRACTID_PREFIX "sys-ntlm");
#ifdef PR_LOGGING
            if (!module)
                LOG(("failed to load sys-ntlm module\n"));
#endif
        }

        
        if (!module) {
            if (!*sessionState) {
                
                
                *sessionState = new nsNTLMSessionState();
                if (!*sessionState)
                    return NS_ERROR_OUT_OF_MEMORY;
                NS_ADDREF(*sessionState);
            }

            module = do_CreateInstance(NS_AUTH_MODULE_CONTRACTID_PREFIX "ntlm");

            
            *identityInvalid = PR_TRUE;
        }

        
        if (!module)
            return NS_ERROR_UNEXPECTED;

        
        
        module.swap(*continuationState);
    }
    return NS_OK;
}

NS_IMETHODIMP
nsHttpNTLMAuth::GenerateCredentials(nsIHttpChannel  *httpChannel,
                                    const char      *challenge,
                                    PRBool           isProxyAuth,
                                    const PRUnichar *domain,
                                    const PRUnichar *user,
                                    const PRUnichar *pass,
                                    nsISupports    **sessionState,
                                    nsISupports    **continuationState,
                                    char           **creds)

{
    LOG(("nsHttpNTLMAuth::GenerateCredentials\n"));

    *creds = nsnull;

    nsresult rv;
    nsCOMPtr<nsIAuthModule> module = do_QueryInterface(*continuationState, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    void *inBuf, *outBuf;
    PRUint32 inBufLen, outBufLen;

    
    if (PL_strcasecmp(challenge, "NTLM") == 0) {
        
        nsCOMPtr<nsIURI> uri;
        rv = httpChannel->GetURI(getter_AddRefs(uri));
        if (NS_FAILED(rv))
            return rv;
        nsCAutoString serviceName, host;
        rv = uri->GetAsciiHost(host);
        if (NS_FAILED(rv))
            return rv;
        serviceName.AppendLiteral("HTTP@");
        serviceName.Append(host);
        
        rv = module->Init(serviceName.get(), nsIAuthModule::REQ_DEFAULT, domain, user, pass);
        if (NS_FAILED(rv))
            return rv;

        inBufLen = 0;
        inBuf = nsnull;
    }
    else {
        
        
        int len = strlen(challenge);
        if (len < 6)
            return NS_ERROR_UNEXPECTED; 
        challenge += 5;
        len -= 5;

        
        inBufLen = (len * 3)/4;      
        inBuf = nsMemory::Alloc(inBufLen);
        if (!inBuf)
            return NS_ERROR_OUT_OF_MEMORY;

        
        while (challenge[len - 1] == '=')
          len--;

        if (PL_Base64Decode(challenge, len, (char *) inBuf) == nsnull) {
            nsMemory::Free(inBuf);
            return NS_ERROR_UNEXPECTED; 
        }
    }

    rv = module->GetNextToken(inBuf, inBufLen, &outBuf, &outBufLen);
    if (NS_SUCCEEDED(rv)) {
        
        int credsLen = 5 + ((outBufLen + 2)/3)*4;
        *creds = (char *) nsMemory::Alloc(credsLen + 1);
        if (!*creds)
            rv = NS_ERROR_OUT_OF_MEMORY;
        else {
            memcpy(*creds, "NTLM ", 5);
            PL_Base64Encode((char *) outBuf, outBufLen, *creds + 5);
            (*creds)[credsLen] = '\0'; 
        }
        
        nsMemory::Free(outBuf);
    }

    if (inBuf)
        nsMemory::Free(inBuf);

    return rv;
}

NS_IMETHODIMP
nsHttpNTLMAuth::GetAuthFlags(PRUint32 *flags)
{
    *flags = CONNECTION_BASED | IDENTITY_INCLUDES_DOMAIN | IDENTITY_ENCRYPTED;
    return NS_OK;
}
