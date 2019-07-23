





































#include "plstr.h"
#include "nsNetUtil.h"
#include "nsJVMAuthTools.h"
#include "nsIHttpAuthManager.h"

static NS_DEFINE_CID(kHttpAuthManagerCID, NS_HTTPAUTHMANAGER_CID);




NS_IMPL_ISUPPORTS1(nsAuthenticationInfoImp, nsIAuthenticationInfo)

nsAuthenticationInfoImp::nsAuthenticationInfoImp(char* username,
                                                 char* password)
{
    mUserName = username;
    mPassWord = password;
}

nsAuthenticationInfoImp::~nsAuthenticationInfoImp()
{
    if (mUserName)
        nsMemory::Free(mUserName);
        
    if (mPassWord)
        nsMemory::Free(mPassWord);
}


NS_IMETHODIMP nsAuthenticationInfoImp::GetUsername(const char * *aUsername)
{
    *aUsername = mUserName;
    return NS_OK;
}


NS_IMETHODIMP nsAuthenticationInfoImp::GetPassword(const char * *aPassword)
{
    *aPassword = mPassWord;
    return NS_OK;
}





NS_IMPL_AGGREGATED(nsJVMAuthTools)

nsJVMAuthTools::nsJVMAuthTools(nsISupports* outer)
{
    NS_INIT_AGGREGATED(outer);
}

nsJVMAuthTools::~nsJVMAuthTools(void)
{
}

NS_INTERFACE_MAP_BEGIN_AGGREGATED(nsJVMAuthTools)
    NS_INTERFACE_MAP_ENTRY(nsIJVMAuthTools)
NS_INTERFACE_MAP_END

NS_METHOD
nsJVMAuthTools::GetAuthenticationInfo(const char* protocol,
                                      const char* host,
                                      PRInt32 port, 
                                      const char* scheme,
                                      const char* realm,
                                      nsIAuthenticationInfo **_retval)
{
    if (!protocol || !host || !scheme || !realm)
        return NS_ERROR_INVALID_ARG;

    if (!PL_strcasecmp(protocol, "HTTP") && !PL_strcasecmp(protocol, "HTTPS"))
        return NS_ERROR_INVALID_ARG;

    nsCOMPtr<nsIHttpAuthManager> authManager = do_GetService(kHttpAuthManagerCID);
    if (!authManager)
        return NS_ERROR_FAILURE;
    
    nsDependentCString protocolString(protocol);
    nsDependentCString hostString(host);
    nsDependentCString schemeString(scheme);
    nsDependentCString realmString(realm);
    nsAutoString       domainString, username, password;
    
    nsresult rv = authManager->GetAuthIdentity(protocolString,
                                               hostString,
                                               port,
                                               schemeString,
                                               realmString,
                                               EmptyCString(), 
                                               domainString,
                                               username,
                                               password);
    if (NS_FAILED(rv))
        return NS_ERROR_FAILURE;

    nsAuthenticationInfoImp* authInfo = new nsAuthenticationInfoImp(
                                            ToNewUTF8String(username),
                                            ToNewUTF8String(password));
    NS_ENSURE_TRUE(authInfo, NS_ERROR_OUT_OF_MEMORY);
    NS_ADDREF(authInfo);
    *_retval = authInfo;
    
    return NS_OK;
}

NS_METHOD
nsJVMAuthTools::SetAuthenticationInfo(const char* protocol,
                                      const char* host,
                                      PRInt32 port, 
                                      const char* scheme,
                                      const char* realm,
                                      const char *username,
                                      const char *password)
{
    if (!protocol || !host || !scheme || !realm)
        return NS_ERROR_INVALID_ARG;
    
    if (!PL_strcasecmp(protocol, "HTTP") && !PL_strcasecmp(protocol, "HTTPS"))
        return NS_ERROR_INVALID_ARG;

    nsCOMPtr<nsIHttpAuthManager> authManager = do_GetService(kHttpAuthManagerCID);
    if (!authManager)
        return NS_ERROR_FAILURE;
    
    nsDependentCString protocolString(protocol);
    nsDependentCString hostString(host);
    nsDependentCString schemeString(scheme);
    nsDependentCString realmString(realm);
    
    nsresult rv = authManager->SetAuthIdentity(protocolString,
                                               hostString,
                                               port,
                                               schemeString,
                                               realmString,
                                               EmptyCString(),
                                               EmptyString(), 
                                               NS_ConvertUTF8toUTF16(username),
                                               NS_ConvertUTF8toUTF16(password));
    return rv;
}

