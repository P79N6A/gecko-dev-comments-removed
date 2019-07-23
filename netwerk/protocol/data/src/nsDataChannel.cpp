






































#include "nsDataChannel.h"
#include "nsNetUtil.h"
#include "nsIPipe.h"
#include "nsIInputStream.h"
#include "nsIOutputStream.h"
#include "nsReadableUtils.h"
#include "nsNetSegmentUtils.h"
#include "nsEscape.h"
#include "plbase64.h"
#include "plstr.h"
#include "prmem.h"

nsresult
nsDataChannel::OpenContentStream(PRBool async, nsIInputStream **result)
{
    NS_ENSURE_TRUE(URI(), NS_ERROR_NOT_INITIALIZED);

    nsresult rv;
    PRBool lBase64 = PR_FALSE;

    nsCAutoString spec;
    rv = URI()->GetAsciiSpec(spec);
    if (NS_FAILED(rv)) return rv;

    
    char *buffer = (char *) strstr(spec.BeginWriting(), "data:");
    if (!buffer) {
        
        return NS_ERROR_MALFORMED_URI;
    }
    buffer += 5;

    
    char *comma = strchr(buffer, ',');
    if (!comma)
        return NS_ERROR_MALFORMED_URI;

    *comma = '\0';

    
    char *base64 = strstr(buffer, ";base64");
    if (base64) {
        lBase64 = PR_TRUE;
        *base64 = '\0';
    }

    nsCString contentType, contentCharset;

    if (comma == buffer) {
        
        contentType.AssignLiteral("text/plain");
        contentCharset.AssignLiteral("US-ASCII");
    } else {
        
        char *semiColon = (char *) strchr(buffer, ';');
        if (semiColon)
            *semiColon = '\0';
        
        if (semiColon == buffer || base64 == buffer) {
            
            contentType.AssignLiteral("text/plain");
        } else {
            contentType = buffer;
            ToLowerCase(contentType);
        }

        if (semiColon) {
            char *charset = PL_strcasestr(semiColon + 1, "charset=");
            if (charset)
                contentCharset = charset + sizeof("charset=") - 1;

            *semiColon = ';';
        }
    }
    contentType.StripWhitespace();
    contentCharset.StripWhitespace();

    nsCAutoString dataBuffer(comma + 1);
    NS_UnescapeURL(dataBuffer);

    if (lBase64 || ((strncmp(contentType.get(),"text/",5) != 0) &&
                     contentType.Find("xml") == kNotFound)) {
        
        dataBuffer.StripWhitespace();
    }
    
    nsCOMPtr<nsIInputStream> bufInStream;
    nsCOMPtr<nsIOutputStream> bufOutStream;
    
    
    rv = NS_NewPipe(getter_AddRefs(bufInStream),
                    getter_AddRefs(bufOutStream),
                    NET_DEFAULT_SEGMENT_SIZE, PR_UINT32_MAX,
                    async, PR_TRUE);
    if (NS_FAILED(rv))
        return rv;

    PRUint32 contentLen;
    if (lBase64) {
        *base64 = ';';
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

    *comma = ',';

    SetContentType(contentType);
    SetContentCharset(contentCharset);
    SetContentLength64(contentLen);

    NS_ADDREF(*result = bufInStream);

    return NS_OK;
}
