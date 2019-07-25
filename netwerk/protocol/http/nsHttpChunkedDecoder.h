




#ifndef nsHttpChunkedDecoder_h__
#define nsHttpChunkedDecoder_h__

#include "nsHttp.h"
#include "nsError.h"
#include "nsString.h"
#include "nsHttpHeaderArray.h"

class nsHttpChunkedDecoder
{
public:
    nsHttpChunkedDecoder() : mTrailers(nullptr)
                           , mChunkRemaining(0)
                           , mReachedEOF(false)
                           , mWaitEOF(false) {}
   ~nsHttpChunkedDecoder() { delete mTrailers; }

    bool ReachedEOF() { return mReachedEOF; }

    
    nsresult HandleChunkedContent(char *buf,
                                  uint32_t count,
                                  uint32_t *contentRead,
                                  uint32_t *contentRemaining);

    nsHttpHeaderArray *Trailers() { return mTrailers; }

    nsHttpHeaderArray *TakeTrailers() { nsHttpHeaderArray *h = mTrailers;
                                        mTrailers = nullptr;
                                        return h; }

    uint32_t GetChunkRemaining() { return mChunkRemaining; }

private:
    nsresult ParseChunkRemaining(char *buf,
                                 uint32_t count,
                                 uint32_t *countRead);

private:
    nsHttpHeaderArray *mTrailers;
    uint32_t           mChunkRemaining;
    nsCString          mLineBuf; 
    bool               mReachedEOF;
    bool               mWaitEOF;
};

#endif
