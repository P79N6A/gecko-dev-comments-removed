






#include "nsDataChannel.h"

#include "mozilla/Base64.h"
#include "nsIOService.h"
#include "nsDataHandler.h"
#include "nsIPipe.h"
#include "nsIInputStream.h"
#include "nsIOutputStream.h"
#include "nsEscape.h"

using namespace mozilla;

nsresult
nsDataChannel::OpenContentStream(bool async, nsIInputStream **result,
                                 nsIChannel** channel)
{
    NS_ENSURE_TRUE(URI(), NS_ERROR_NOT_INITIALIZED);

    nsresult rv;

    nsAutoCString spec;
    rv = URI()->GetAsciiSpec(spec);
    if (NS_FAILED(rv)) return rv;

    nsCString contentType, contentCharset, dataBuffer, hashRef;
    bool lBase64;
    rv = nsDataHandler::ParseURI(spec, contentType, contentCharset,
                                 lBase64, dataBuffer, hashRef);

    NS_UnescapeURL(dataBuffer);

    if (lBase64) {
        
        
        
        dataBuffer.StripWhitespace();
    }
    
    nsCOMPtr<nsIInputStream> bufInStream;
    nsCOMPtr<nsIOutputStream> bufOutStream;
    
    
    rv = NS_NewPipe(getter_AddRefs(bufInStream),
                    getter_AddRefs(bufOutStream),
                    nsIOService::gDefaultSegmentSize,
                    UINT32_MAX,
                    async, true);
    if (NS_FAILED(rv))
        return rv;

    uint32_t contentLen;
    if (lBase64) {
        const uint32_t dataLen = dataBuffer.Length();
        int32_t resultLen = 0;
        if (dataLen >= 1 && dataBuffer[dataLen-1] == '=') {
            if (dataLen >= 2 && dataBuffer[dataLen-2] == '=')
                resultLen = dataLen-2;
            else
                resultLen = dataLen-1;
        } else {
            resultLen = dataLen;
        }
        resultLen = ((resultLen * 3) / 4);

        nsAutoCString decodedData;
        rv = Base64Decode(dataBuffer, decodedData);
        NS_ENSURE_SUCCESS(rv, rv);
        rv = bufOutStream->Write(decodedData.get(), resultLen, &contentLen);
    } else {
        rv = bufOutStream->Write(dataBuffer.get(), dataBuffer.Length(), &contentLen);
    }
    if (NS_FAILED(rv))
        return rv;

    SetContentType(contentType);
    SetContentCharset(contentCharset);
    mContentLength = contentLen;

    bufInStream.forget(result);

    return NS_OK;
}
