




































#include "nsAuth.h"
#include "nsAuthSambaNTLM.h"
#include "prenv.h"
#include "plbase64.h"
#include "prerror.h"

#include <stdlib.h>

nsAuthSambaNTLM::nsAuthSambaNTLM()
    : mInitialMessage(nsnull), mChildPID(nsnull), mFromChildFD(nsnull),
      mToChildFD(nsnull)
{
}

nsAuthSambaNTLM::~nsAuthSambaNTLM()
{
    
    
    Shutdown();
    free(mInitialMessage);
}

void
nsAuthSambaNTLM::Shutdown()
{
    if (mFromChildFD) {
        PR_Close(mFromChildFD);
        mFromChildFD = nsnull;
    }
    if (mToChildFD) {
        PR_Close(mToChildFD);
        mToChildFD = nsnull;
    }
    if (mChildPID) {
        PRInt32 exitCode;
        PR_WaitProcess(mChildPID, &exitCode);
        mChildPID = nsnull;
    }
}

NS_IMPL_ISUPPORTS1(nsAuthSambaNTLM, nsIAuthModule)

static PRBool
SpawnIOChild(char** aArgs, PRProcess** aPID,
             PRFileDesc** aFromChildFD, PRFileDesc** aToChildFD)
{
    PRFileDesc* toChildPipeRead;
    PRFileDesc* toChildPipeWrite;
    if (PR_CreatePipe(&toChildPipeRead, &toChildPipeWrite) != PR_SUCCESS)
        return PR_FALSE;
    PR_SetFDInheritable(toChildPipeRead, PR_TRUE);
    PR_SetFDInheritable(toChildPipeWrite, PR_FALSE);

    PRFileDesc* fromChildPipeRead;
    PRFileDesc* fromChildPipeWrite;
    if (PR_CreatePipe(&fromChildPipeRead, &fromChildPipeWrite) != PR_SUCCESS) {
        PR_Close(toChildPipeRead);
        PR_Close(toChildPipeWrite);
        return PR_FALSE;
    }
    PR_SetFDInheritable(fromChildPipeRead, PR_FALSE);
    PR_SetFDInheritable(fromChildPipeWrite, PR_TRUE);

    PRProcessAttr* attr = PR_NewProcessAttr();
    if (!attr) {
        PR_Close(fromChildPipeRead);
        PR_Close(fromChildPipeWrite);
        PR_Close(toChildPipeRead);
        PR_Close(toChildPipeWrite);
        return PR_FALSE;
    }

    PR_ProcessAttrSetStdioRedirect(attr, PR_StandardInput, toChildPipeRead);
    PR_ProcessAttrSetStdioRedirect(attr, PR_StandardOutput, fromChildPipeWrite);   

    PRProcess* process = PR_CreateProcess(aArgs[0], aArgs, nsnull, attr);
    PR_DestroyProcessAttr(attr);
    PR_Close(fromChildPipeWrite);
    PR_Close(toChildPipeRead);
    if (!process) {
        LOG(("ntlm_auth exec failure [%d]", PR_GetError()));
        PR_Close(fromChildPipeRead);
        PR_Close(toChildPipeWrite);
        return PR_FALSE;        
    }

    *aPID = process;
    *aFromChildFD = fromChildPipeRead;
    *aToChildFD = toChildPipeWrite;
    return PR_TRUE;
}

static PRBool WriteString(PRFileDesc* aFD, const nsACString& aString)
{
    PRInt32 length = aString.Length();
    const char* s = aString.BeginReading();
    LOG(("Writing to ntlm_auth: %s", s));

    while (length > 0) {
        int result = PR_Write(aFD, s, length);
        if (result <= 0)
            return PR_FALSE;
        s += result;
        length -= result;
    }
    return PR_TRUE;
}

static PRBool ReadLine(PRFileDesc* aFD, nsACString& aString)
{
    
    
    
    aString.Truncate();
    for (;;) {
        char buf[1024];
        int result = PR_Read(aFD, buf, sizeof(buf));
        if (result <= 0)
            return PR_FALSE;
        aString.Append(buf, result);
        if (buf[result - 1] == '\n') {
            LOG(("Read from ntlm_auth: %s", nsPromiseFlatCString(aString).get()));
            return PR_TRUE;
        }
    }
}





static PRUint8* ExtractMessage(const nsACString& aLine, PRUint32* aLen)
{
    
    
    PRInt32 length = aLine.Length();
    
    
    NS_ASSERTION(length >= 4, "Line too short...");
    const char* line = aLine.BeginReading();
    const char* s = line + 3;
    length -= 4; 
    NS_ASSERTION(s[length] == '\n', "aLine not newline-terminated");
    
    if (length & 3) {
        
        
        NS_WARNING("Base64 encoded block should be a multiple of 4 chars");
        return nsnull;
    } 

    
    
    PRInt32 numEquals;
    for (numEquals = 0; numEquals < length; ++numEquals) {
        if (s[length - 1 - numEquals] != '=')
            break;
    }
    *aLen = (length/4)*3 - numEquals;
    return NS_REINTERPRET_CAST(PRUint8*, PL_Base64Decode(s, length, nsnull));
}

nsresult
nsAuthSambaNTLM::SpawnNTLMAuthHelper()
{
    const char* username = PR_GetEnv("USER");
    if (!username)
        return NS_ERROR_FAILURE;

    char* args[] = {
        "ntlm_auth",
        "--helper-protocol", "ntlmssp-client-1",
        "--use-cached-creds",
        "--username", NS_CONST_CAST(char*, username),
        nsnull
    };

    PRBool isOK = SpawnIOChild(args, &mChildPID, &mFromChildFD, &mToChildFD);
    if (!isOK)  
        return NS_ERROR_FAILURE;

    if (!WriteString(mToChildFD, NS_LITERAL_CSTRING("YR\n")))
        return NS_ERROR_FAILURE;
    nsCString line;
    if (!ReadLine(mFromChildFD, line))
        return NS_ERROR_FAILURE;
    if (!StringBeginsWith(line, NS_LITERAL_CSTRING("YR "))) {
        
        return NS_ERROR_FAILURE;
    }

    
    
    mInitialMessage = ExtractMessage(line, &mInitialMessageLen);
    if (!mInitialMessage)
        return NS_ERROR_FAILURE;
    return NS_OK;
}

NS_IMETHODIMP
nsAuthSambaNTLM::Init(const char *serviceName,
                      PRUint32    serviceFlags,
                      const PRUnichar *domain,
                      const PRUnichar *username,
                      const PRUnichar *password)
{
    NS_ASSERTION(!username && !domain && !password, "unexpected credentials");
    return NS_OK;
}

NS_IMETHODIMP
nsAuthSambaNTLM::GetNextToken(const void *inToken,
                              PRUint32    inTokenLen,
                              void      **outToken,
                              PRUint32   *outTokenLen)
{
    if (!inToken) {
        
        *outToken = nsMemory::Clone(mInitialMessage, mInitialMessageLen);
        if (!*outToken)
            return NS_ERROR_OUT_OF_MEMORY;
        *outTokenLen = mInitialMessageLen;
        return NS_OK;
    }

    
    char* encoded = PL_Base64Encode(NS_STATIC_CAST(const char*, inToken), inTokenLen, nsnull);
    if (!encoded)
        return NS_ERROR_OUT_OF_MEMORY;

    nsCString request;
    request.AssignLiteral("TT ");
    request.Append(encoded);
    free(encoded);
    request.Append('\n');

    if (!WriteString(mToChildFD, request))
        return NS_ERROR_FAILURE;
    nsCString line;
    if (!ReadLine(mFromChildFD, line))
        return NS_ERROR_FAILURE;
    if (!StringBeginsWith(line, NS_LITERAL_CSTRING("KK "))) {
        
        return NS_ERROR_FAILURE;
    }
    PRUint8* buf = ExtractMessage(line, outTokenLen);
    if (!buf)
        return NS_ERROR_FAILURE;
    
    *outToken = nsMemory::Clone(buf, *outTokenLen);
    if (!*outToken) {
        free(buf);
        return NS_ERROR_OUT_OF_MEMORY;
    }
    
    
    
    Shutdown();
    return NS_SUCCESS_AUTH_FINISHED;
}

NS_IMETHODIMP
nsAuthSambaNTLM::Unwrap(const void *inToken,
                        PRUint32    inTokenLen,
                        void      **outToken,
                        PRUint32   *outTokenLen)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsAuthSambaNTLM::Wrap(const void *inToken,
                      PRUint32    inTokenLen,
                      PRBool      confidential,
                      void      **outToken,
                      PRUint32   *outTokenLen)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
