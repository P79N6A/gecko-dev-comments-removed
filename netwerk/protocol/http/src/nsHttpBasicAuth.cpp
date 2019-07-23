








































#include <stdlib.h>
#include "nsHttp.h"
#include "nsHttpBasicAuth.h"
#include "plbase64.h"
#include "plstr.h"
#include "prmem.h"
#include "nsString.h"





nsHttpBasicAuth::nsHttpBasicAuth()
{
}

nsHttpBasicAuth::~nsHttpBasicAuth()
{
}





NS_IMPL_ISUPPORTS1(nsHttpBasicAuth, nsIHttpAuthenticator)





NS_IMETHODIMP
nsHttpBasicAuth::ChallengeReceived(nsIHttpChannel *httpChannel,
                                   const char *challenge,
                                   PRBool isProxyAuth,
                                   nsISupports **sessionState,
                                   nsISupports **continuationState,
                                   PRBool *identityInvalid)
{
    
    
    *identityInvalid = PR_TRUE;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpBasicAuth::GenerateCredentials(nsIHttpChannel *httpChannel,
                                     const char *challenge,
                                     PRBool isProxyAuth,
                                     const PRUnichar *domain,
                                     const PRUnichar *user,
                                     const PRUnichar *password,
                                     nsISupports **sessionState,
                                     nsISupports **continuationState,
                                     char **creds)

{
    LOG(("nsHttpBasicAuth::GenerateCredentials [challenge=%s]\n", challenge));

    NS_ENSURE_ARG_POINTER(creds);

    
    PRBool isBasicAuth = !PL_strncasecmp(challenge, "basic", 5);
    NS_ENSURE_TRUE(isBasicAuth, NS_ERROR_UNEXPECTED);

    
    nsCAutoString userpass;
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
nsHttpBasicAuth::GetAuthFlags(nsresult *flags)
{
    *flags = REQUEST_BASED | REUSABLE_CREDENTIALS | REUSABLE_CHALLENGE;
    return NS_OK;
}
