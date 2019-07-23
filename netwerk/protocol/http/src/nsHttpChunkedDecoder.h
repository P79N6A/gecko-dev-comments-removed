





































#ifndef nsHttpChunkedDecoder_h__
#define nsHttpChunkedDecoder_h__

#include "nsError.h"
#include "nsString.h"
#include "nsHttpHeaderArray.h"

class nsHttpChunkedDecoder
{
public:
    nsHttpChunkedDecoder() : mTrailers(nsnull)
                           , mChunkRemaining(0)
                           , mReachedEOF(PR_FALSE)
                           , mWaitEOF(PR_FALSE) {}
   ~nsHttpChunkedDecoder() { delete mTrailers; }

    PRBool ReachedEOF() { return mReachedEOF; }

    
    nsresult HandleChunkedContent(char *buf,
                                  PRUint32 count,
                                  PRUint32 *contentRead,
                                  PRUint32 *contentRemaining);

    nsHttpHeaderArray *Trailers() { return mTrailers; }

    nsHttpHeaderArray *TakeTrailers() { nsHttpHeaderArray *h = mTrailers;
                                        mTrailers = nsnull;
                                        return h; }

private:
    nsresult ParseChunkRemaining(char *buf,
                                 PRUint32 count,
                                 PRUint32 *countRead);

private:
    nsHttpHeaderArray *mTrailers;
    PRUint32           mChunkRemaining;
    nsCString          mLineBuf; 
    PRPackedBool       mReachedEOF;
    PRPackedBool       mWaitEOF;
};

#endif
