




































#include "nsComponentManagerUtils.h"
#include "nsNativeCharsetUtils.h"
#include "nsIServiceManager.h"
#include "nsIPrefService.h"

#include "nsAuthSASL.h"

static const char kNegotiateAuthSSPI[] = "network.auth.use-sspi";

nsAuthSASL::nsAuthSASL()
{
    mSASLReady = false;
}

void nsAuthSASL::Reset() 
{
    mSASLReady = false;
}

NS_IMPL_ISUPPORTS1(nsAuthSASL, nsIAuthModule)

NS_IMETHODIMP
nsAuthSASL::Init(const char *serviceName,
                 PRUint32    serviceFlags,
                 const PRUnichar *domain,
                 const PRUnichar *username,
                 const PRUnichar *password)
{
    nsresult rv;
    
    NS_ASSERTION(username, "SASL requires a username");
    NS_ASSERTION(!domain && !password, "unexpected credentials");

    mUsername = username;
    
    
    serviceFlags |= REQ_MUTUAL_AUTH;
   
    
    const char *contractID = NS_AUTH_MODULE_CONTRACTID_PREFIX "kerb-gss";
    
    nsCOMPtr<nsIPrefBranch> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
    if (prefs) {
        PRBool val;
        rv = prefs->GetBoolPref(kNegotiateAuthSSPI, &val);
        if (NS_SUCCEEDED(rv) && val)
            contractID = NS_AUTH_MODULE_CONTRACTID_PREFIX "kerb-sspi";
    }
    
    mInnerModule = do_CreateInstance(contractID, &rv);
    
    NS_ENSURE_SUCCESS(rv, rv);

    mInnerModule->Init(serviceName, serviceFlags, nsnull, nsnull, nsnull);

    return NS_OK;
}

NS_IMETHODIMP
nsAuthSASL::GetNextToken(const void *inToken,
                         PRUint32    inTokenLen,
                         void      **outToken,
                         PRUint32   *outTokenLen)
{
    nsresult rv;
    void *unwrappedToken;
    char *message;
    PRUint32 unwrappedTokenLen, messageLen;
    nsCAutoString userbuf;
    
    if (!mInnerModule) 
        return NS_ERROR_NOT_INITIALIZED;

    if (mSASLReady) {
        
        
        
        if (inTokenLen == 0) {
            *outToken = NULL;
            *outTokenLen = 0;
            return NS_OK;
        }
        
        

        
        rv = mInnerModule->Unwrap(inToken, inTokenLen, &unwrappedToken, 
                                  &unwrappedTokenLen);
        if (NS_FAILED(rv)) {
            Reset();
            return rv;
        }
        
        
        
        
        nsMemory::Free(unwrappedToken);
        
        NS_CopyUnicodeToNative(mUsername, userbuf);
        messageLen = userbuf.Length() + 4 + 1;
        message = (char *)nsMemory::Alloc(messageLen);
        if (!message) {
          Reset();
          return NS_ERROR_OUT_OF_MEMORY;
        }
        message[0] = 0x01; 
        message[1] = 0x00;
        message[2] = 0x00;
        message[3] = 0x00; 
        strcpy(message+4, userbuf.get());
        
        
        rv = mInnerModule->Wrap((void *) message, messageLen-1, PR_FALSE, 
                                outToken, outTokenLen);
        nsMemory::Free(message);
        Reset(); 
        return NS_SUCCEEDED(rv) ? NS_SUCCESS_AUTH_FINISHED : rv;
    }
    rv = mInnerModule->GetNextToken(inToken, inTokenLen, outToken, 
                                    outTokenLen);
    if (rv == NS_SUCCESS_AUTH_FINISHED) {
        mSASLReady = true;
        rv = NS_OK;
    }
    return rv;
}

NS_IMETHODIMP
nsAuthSASL::Unwrap(const void *inToken,
                   PRUint32    inTokenLen,
                   void      **outToken,
                   PRUint32   *outTokenLen)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsAuthSASL::Wrap(const void *inToken,
                 PRUint32    inTokenLen,
                 PRBool      confidential,
                 void      **outToken,
                 PRUint32   *outTokenLen)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
