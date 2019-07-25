






































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
#include "nsIHttpAuthenticableChannel.h"
#include "nsIURI.h"

static const char kAllowProxies[] = "network.automatic-ntlm-auth.allow-proxies";
static const char kTrustedURIs[]  = "network.automatic-ntlm-auth.trusted-uris";
static const char kForceGeneric[] = "network.auth.force-generic-ntlm";





static bool
MatchesBaseURI(const nsCSubstring &matchScheme,
               const nsCSubstring &matchHost,
               PRInt32             matchPort,
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
        if (matchPort != (PRInt32) port)
            return false;
    }
    else
        hostEnd = baseEnd;


    
    if (hostStart == hostEnd)
        return true;

    PRUint32 hostLen = hostEnd - hostStart;

    
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

static bool
TestPref(nsIURI *uri, const char *pref)
{
    nsCOMPtr<nsIPrefBranch> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
    if (!prefs)
        return false;

    nsCAutoString scheme, host;
    PRInt32 port;

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
    
    nsMemory::Free(hostList);
    return false;
}


static bool
ForceGenericNTLM()
{
    nsCOMPtr<nsIPrefBranch> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
    if (!prefs)
        return false;
    bool flag = false;

    if (NS_FAILED(prefs->GetBoolPref(kForceGeneric, &flag)))
        flag = false;

    LOG(("Force use of generic ntlm auth module: %d\n", flag));
    return flag;
}


static bool
CanUseDefaultCredentials(nsIHttpAuthenticableChannel *channel,
                         bool isProxyAuth)
{
    nsCOMPtr<nsIPrefBranch> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
    if (!prefs)
        return false;

    if (isProxyAuth) {
        bool val;
        if (NS_FAILED(prefs->GetBoolPref(kAllowProxies, &val)))
            val = false;
        LOG(("Default credentials allowed for proxy: %d\n", val));
        return val;
    }

    nsCOMPtr<nsIURI> uri;
    channel->GetURI(getter_AddRefs(uri));
    bool isTrustedHost = (uri && TestPref(uri, kTrustedURIs));
    LOG(("Default credentials allowed for host: %d\n", isTrustedHost));
    return isTrustedHost;
}



class nsNTLMSessionState : public nsISupports
{
public:
    NS_DECL_ISUPPORTS
};
NS_IMPL_ISUPPORTS0(nsNTLMSessionState)



NS_IMPL_ISUPPORTS1(nsHttpNTLMAuth, nsIHttpAuthenticator)

NS_IMETHODIMP
nsHttpNTLMAuth::ChallengeReceived(nsIHttpAuthenticableChannel *channel,
                                  const char     *challenge,
                                  bool            isProxyAuth,
                                  nsISupports   **sessionState,
                                  nsISupports   **continuationState,
                                  bool           *identityInvalid)
{
    LOG(("nsHttpNTLMAuth::ChallengeReceived [ss=%p cs=%p]\n",
         *sessionState, *continuationState));

    

    *identityInvalid = false;

    
    
    
    if (PL_strcasecmp(challenge, "NTLM") == 0) {
        nsCOMPtr<nsISupports> module;

        
        
        
        
        bool forceGeneric = ForceGenericNTLM();
        if (!forceGeneric && !*sessionState) {
            
            
            
            if (!*continuationState && CanUseDefaultCredentials(channel, isProxyAuth)) {
                
                
                
                module = do_CreateInstance(NS_AUTH_MODULE_CONTRACTID_PREFIX "sys-ntlm");
            }
#ifdef XP_WIN
            else {
                
                
                
                
                
                module = do_CreateInstance(NS_AUTH_MODULE_CONTRACTID_PREFIX "sys-ntlm");
                *identityInvalid = true;
            }
#endif 
#ifdef PR_LOGGING
            if (!module)
                LOG(("Native sys-ntlm auth module not found.\n"));
#endif
        }

#ifdef XP_WIN
        
        if (!forceGeneric && !module)
            return NS_ERROR_UNEXPECTED;
#endif

        
        if (!module) {
            if (!*sessionState) {
                
                
                *sessionState = new nsNTLMSessionState();
                if (!*sessionState)
                    return NS_ERROR_OUT_OF_MEMORY;
                NS_ADDREF(*sessionState);
            }

            
            
            LOG(("Trying to fall back on internal ntlm auth.\n"));
            module = do_CreateInstance(NS_AUTH_MODULE_CONTRACTID_PREFIX "ntlm");

            
            *identityInvalid = true;
        }

        
        if (!module) {
            LOG(("No ntlm auth modules available.\n"));
            return NS_ERROR_UNEXPECTED;
        }

        
        
        module.swap(*continuationState);
    }
    return NS_OK;
}

NS_IMETHODIMP
nsHttpNTLMAuth::GenerateCredentials(nsIHttpAuthenticableChannel *authChannel,
                                    const char      *challenge,
                                    bool             isProxyAuth,
                                    const PRUnichar *domain,
                                    const PRUnichar *user,
                                    const PRUnichar *pass,
                                    nsISupports    **sessionState,
                                    nsISupports    **continuationState,
                                    PRUint32       *aFlags,
                                    char           **creds)

{
    LOG(("nsHttpNTLMAuth::GenerateCredentials\n"));

    *creds = nsnull;
    *aFlags = 0;

    
    
    
    
    if (!user || !pass)
        *aFlags = USING_INTERNAL_IDENTITY;

    nsresult rv;
    nsCOMPtr<nsIAuthModule> module = do_QueryInterface(*continuationState, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    void *inBuf, *outBuf;
    PRUint32 inBufLen, outBufLen;

    
    if (PL_strcasecmp(challenge, "NTLM") == 0) {
        
        nsCOMPtr<nsIURI> uri;
        rv = authChannel->GetURI(getter_AddRefs(uri));
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

        
        while (challenge[len - 1] == '=')
          len--;

        
        inBufLen = (len * 3)/4;      
        inBuf = nsMemory::Alloc(inBufLen);
        if (!inBuf)
            return NS_ERROR_OUT_OF_MEMORY;

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
