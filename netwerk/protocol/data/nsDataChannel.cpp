






































#include "nsIOService.h"
#include "nsDataChannel.h"
#include "nsDataHandler.h"
#include "nsNetUtil.h"
#include "nsIPipe.h"
#include "nsIInputStream.h"
#include "nsIOutputStream.h"
#include "nsReadableUtils.h"
#include "nsEscape.h"
#include "plbase64.h"
#include "plstr.h"
#include "prmem.h"

nsresult
nsDataChannel::OpenContentStream(PRBool async, nsIInputStream **result,
                                 nsIChannel** channel)
{
    NS_ENSURE_TRUE(URI(), NS_ERROR_NOT_INITIALIZED);

    nsresult rv;

    nsCAutoString spec;
    rv = URI()->GetAsciiSpec(spec);
    if (NS_FAILED(rv)) return rv;

    nsCString contentType, contentCharset, dataBuffer;
    PRBool lBase64;
    rv = nsDataHandler::ParseURI(spec, contentType, contentCharset,
                                 lBase64, dataBuffer);

    NS_UnescapeURL(dataBuffer);

    if (lBase64) {
        
        
        
        dataBuffer.StripWhitespace();
    }
    
    nsCOMPtr<nsIInputStream> bufInStream;
    nsCOMPtr<nsIOutputStream> bufOutStream;
    
    
    rv = NS_NewPipe(getter_AddRefs(bufInStream),
                    getter_AddRefs(bufOutStream),
                    nsIOService::gDefaultSegmentSize,
                    PR_UINT32_MAX,
                    async, PR_TRUE);
    if (NS_FAILED(rv))
        return rv;

    PRUint32 contentLen;
    if (lBase64) {
        const PRUint32 dataLen = dataBuffer.Length();
        PRInt32 resultLen = 0;
        if (dataLen >= 1 && dataBuffer[dataLen-1] == '=') {
            if (dataLen >= 2 && dataBuffer[dataLen-2] == '=')
                resultLen = dataLen-2;
            else
                resultLen = dataLen-1;
        } else {
            resultLen = dataLen;
        }
        resultLen = ((resultLen * 3) / 4);

        
        
        
        char * decodedData = PL_Base64Decode(dataBuffer.get(), dataLen, nsnull);
        if (!decodedData) {
            return NS_ERROR_OUT_OF_MEMORY;
        }

        rv = bufOutStream->Write(decodedData, resultLen, &contentLen);

        PR_Free(decodedData);
    } else {
        rv = bufOutStream->Write(dataBuffer.get(), dataBuffer.Length(), &contentLen);
    }
    if (NS_FAILED(rv))
        return rv;

    SetContentType(contentType);
    SetContentCharset(contentCharset);
    SetContentLength64(contentLen);

    NS_ADDREF(*result = bufInStream);

    return NS_OK;
}
