






































#ifndef _nsXBMDecoder_h
#define _nsXBMDecoder_h

#include "nsCOMPtr.h"
#include "imgIDecoder.h"
#include "imgIContainer.h"
#include "imgIDecoderObserver.h"
#include "gfxIImageFrame.h"

#define NS_XBMDECODER_CID \
{ /* {dbfd145d-3298-4f3c-902f-2c5e1a1494ce} */ \
  0xdbfd145d, \
  0x3298, \
  0x4f3c, \
  { 0x90, 0x2f, 0x2c, 0x5e, 0x1a, 0x14, 0x94, 0xce } \
}

class nsXBMDecoder : public imgIDecoder
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_IMGIDECODER

    nsXBMDecoder();
    virtual ~nsXBMDecoder();

    nsresult ProcessData(const char* aData, PRUint32 aCount);
private:
    static NS_METHOD ReadSegCb(nsIInputStream* aIn, void* aClosure,
                               const char* aFromRawSegment, PRUint32 aToOffset,
                               PRUint32 aCount, PRUint32 *aWriteCount);

    nsCOMPtr<imgIDecoderObserver> mObserver;

    nsCOMPtr<imgIContainer> mImage;
    nsCOMPtr<gfxIImageFrame> mFrame;

    PRUint32 mCurRow;
    PRUint32 mCurCol;

    char* mBuf; 
    char* mPos;
    PRUint32 mBufSize; 

    PRUint32 mWidth;
    PRUint32 mHeight;
    PRUint32 mXHotspot;
    PRUint32 mYHotspot;

    PRUint8* mAlphaRow; 

    PRPackedBool mIsCursor;
    PRPackedBool mIsX10; 

    enum {
        RECV_HEADER,
        RECV_SEEK,
        RECV_DATA,
        RECV_DONE
    } mState;
};


#endif
