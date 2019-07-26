




#include "nsHttpChunkedDecoder.h"
#include "nsHttp.h"
#include <algorithm>





nsresult
nsHttpChunkedDecoder::HandleChunkedContent(char *buf,
                                           uint32_t count,
                                           uint32_t *contentRead,
                                           uint32_t *contentRemaining)
{
    LOG(("nsHttpChunkedDecoder::HandleChunkedContent [count=%u]\n", count));

    *contentRead = 0;
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    while (count) {
        if (mChunkRemaining) {
            uint32_t amt = std::min(mChunkRemaining, count);

            count -= amt;
            mChunkRemaining -= amt;

            *contentRead += amt;
            buf += amt;
        }
        else if (mReachedEOF)
            break; 
        else {
            uint32_t bytesConsumed = 0;

            nsresult rv = ParseChunkRemaining(buf, count, &bytesConsumed);
            if (NS_FAILED(rv)) return rv;

            count -= bytesConsumed;

            if (count) {
                
                memmove(buf, buf + bytesConsumed, count);
            }
        }
    }
    
    *contentRemaining = count;
    return NS_OK;
}





nsresult
nsHttpChunkedDecoder::ParseChunkRemaining(char *buf,
                                          uint32_t count,
                                          uint32_t *bytesConsumed)
{
    NS_PRECONDITION(mChunkRemaining == 0, "chunk remaining should be zero");
    NS_PRECONDITION(count, "unexpected");

    *bytesConsumed = 0;
    
    char *p = static_cast<char *>(memchr(buf, '\n', count));
    if (p) {
        *p = 0;
        if ((p > buf) && (*(p-1) == '\r')) 
            *(p-1) = 0;
        *bytesConsumed = p - buf + 1;

        
        if (!mLineBuf.IsEmpty()) {
            mLineBuf.Append(buf);
            buf = (char *) mLineBuf.get();
        }

        if (mWaitEOF) {
            if (*buf) {
                LOG(("got trailer: %s\n", buf));
                
                if (!mTrailers) {
                    mTrailers = new nsHttpHeaderArray();
                }
                mTrailers->ParseHeaderLine(buf);
            }
            else {
                mWaitEOF = false;
                mReachedEOF = true;
                LOG(("reached end of chunked-body\n"));
            }
        }
        else if (*buf) {
            
            if ((p = PL_strchr(buf, ';')) != nullptr)
                *p = 0;

            if (!sscanf(buf, "%x", &mChunkRemaining)) {
                LOG(("sscanf failed parsing hex on string [%s]\n", buf));
                return NS_ERROR_UNEXPECTED;
            }

            
            if (mChunkRemaining == 0)
                mWaitEOF = true;
        }

        
        mLineBuf.Truncate();
    }
    else {
        
        *bytesConsumed = count;
        
        if (buf[count-1] == '\r')
            count--;
        mLineBuf.Append(buf, count);
    }

    return NS_OK;
}
