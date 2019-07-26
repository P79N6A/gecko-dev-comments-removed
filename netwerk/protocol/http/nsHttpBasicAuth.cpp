





#include "HttpLog.h"

#include "nsHttpBasicAuth.h"
#include "plbase64.h"
#include "nsString.h"

namespace mozilla {
namespace net {





nsHttpBasicAuth::nsHttpBasicAuth()
{
}

nsHttpBasicAuth::~nsHttpBasicAuth()
{
}





NS_IMPL_ISUPPORTS1(nsHttpBasicAuth, nsIHttpAuthenticator)





NS_IMETHODIMP
nsHttpBasicAuth::ChallengeReceived(nsIHttpAuthenticableChannel *authChannel,
                                   const char *challenge,
                                   bool isProxyAuth,
                                   nsISupports **sessionState,
                                   nsISupports **continuationState,
                                   bool *identityInvalid)
{
    
    
    *identityInvalid = true;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpBasicAuth::GenerateCredentials(nsIHttpAuthenticableChannel *authChannel,
                                     const char *challenge,
                                     bool isProxyAuth,
                                     const char16_t *domain,
                                     const char16_t *user,
                                     const char16_t *password,
                                     nsISupports **sessionState,
                                     nsISupports **continuationState,
                                     uint32_t *aFlags,
                                     char **creds)

{
    LOG(("nsHttpBasicAuth::GenerateCredentials [challenge=%s]\n", challenge));

    NS_ENSURE_ARG_POINTER(creds);

    *aFlags = 0;

    
    bool isBasicAuth = !PL_strncasecmp(challenge, "basic", 5);
    NS_ENSURE_TRUE(isBasicAuth, NS_ERROR_UNEXPECTED);

    
    nsAutoCString userpass;
    LossyCopyUTF16toASCII(user, userpass);
    userpass.Append(':'); 
    if (password)
        LossyAppendUTF16toASCII(password, userpass);

    
    
    *creds = (char *) calloc(6 + ((userpass.Length() + 2)/3)*4 + 1, 1);
    if (!*creds)
        return NS_ERROR_OUT_OF_MEMORY;

    memcpy(*creds, "Basic ", 6);
    PL_Base64Encode(userpass.get(), userpass.Length(), *creds + 6);
    return NS_OK;
}

NS_IMETHODIMP
nsHttpBasicAuth::GetAuthFlags(uint32_t *flags)
{
    *flags = REQUEST_BASED | REUSABLE_CREDENTIALS | REUSABLE_CHALLENGE;
    return NS_OK;
}

} 
} 
