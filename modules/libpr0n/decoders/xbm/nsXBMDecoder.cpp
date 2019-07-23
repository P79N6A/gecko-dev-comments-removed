








































#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "nsXBMDecoder.h"

#include "nsIInputStream.h"
#include "nsIComponentManager.h"
#include "nsIImage.h"
#include "nsIInterfaceRequestorUtils.h"

#include "imgILoad.h"

#include "nsIProperties.h"
#include "nsISupportsPrimitives.h"

NS_IMPL_ISUPPORTS1(nsXBMDecoder, imgIDecoder)

nsXBMDecoder::nsXBMDecoder() : mBuf(nsnull), mPos(nsnull), mAlphaRow(nsnull)
{
}

nsXBMDecoder::~nsXBMDecoder()
{
    if (mBuf)
        free(mBuf);

    if (mAlphaRow)
        free(mAlphaRow);
}

NS_IMETHODIMP nsXBMDecoder::Init(imgILoad *aLoad)
{
    nsresult rv;
    mObserver = do_QueryInterface(aLoad);

    mImage = do_CreateInstance("@mozilla.org/image/container;1", &rv);
    if (NS_FAILED(rv))
        return rv;

    mFrame = do_CreateInstance("@mozilla.org/gfx/image/frame;2", &rv);
    if (NS_FAILED(rv))
        return rv;

    aLoad->SetImage(mImage);

    mCurRow = mBufSize = mWidth = mHeight = 0;
    mState = RECV_HEADER;
    mHasNoAlpha = PR_TRUE;

    return NS_OK;
}

NS_IMETHODIMP nsXBMDecoder::Close()
{
    if (mHasNoAlpha) {
        nsCOMPtr<nsIImage> img(do_GetInterface(mFrame));
        img->SetHasNoAlpha();
    }

    mObserver->OnStopContainer(nsnull, mImage);
    mObserver->OnStopDecode(nsnull, NS_OK, nsnull);
    mObserver = nsnull;
    mImage = nsnull;
    mFrame = nsnull;

    if (mAlphaRow) {
        free(mAlphaRow);
        mAlphaRow = nsnull;
    }

    return NS_OK;
}

NS_IMETHODIMP nsXBMDecoder::Flush()
{
    mFrame->SetMutable(PR_FALSE);
    return NS_OK;
}

NS_METHOD nsXBMDecoder::ReadSegCb(nsIInputStream* aIn, void* aClosure,
                             const char* aFromRawSegment, PRUint32 aToOffset,
                             PRUint32 aCount, PRUint32 *aWriteCount) {
    nsXBMDecoder *decoder = reinterpret_cast<nsXBMDecoder*>(aClosure);
    *aWriteCount = aCount;
    return decoder->ProcessData(aFromRawSegment, aCount);
}

NS_IMETHODIMP nsXBMDecoder::WriteFrom(nsIInputStream *aInStr, PRUint32 aCount, PRUint32 *aRetval)
{
    return aInStr->ReadSegments(ReadSegCb, this, aCount, aRetval);
}

nsresult nsXBMDecoder::ProcessData(const char* aData, PRUint32 aCount) {
    char *endPtr;
    
    
    const PRPtrdiff posOffset = mPos ? (mPos - mBuf) : 0;

    
    char* oldbuf = mBuf;
    PRUint32 newbufsize = mBufSize + aCount + 1;
    if (newbufsize < mBufSize)
        mBuf = nsnull;  
    else
        mBuf = (char*)realloc(mBuf, newbufsize);

    if (!mBuf) {
        mState = RECV_DONE;
        if (oldbuf)
            free(oldbuf);
        return NS_ERROR_OUT_OF_MEMORY;
    }
    memcpy(mBuf + mBufSize, aData, aCount);
    mBufSize += aCount;
    mBuf[mBufSize] = 0;
    mPos = mBuf + posOffset;

    
    if (mState == RECV_HEADER) {
        mPos = strstr(mBuf, "#define");
        if (!mPos)
            
            return NS_OK;

        
        if (sscanf(mPos, "#define %*s %u #define %*s %u #define %*s %u #define %*s %u unsigned", &mWidth, &mHeight, &mXHotspot, &mYHotspot) == 4)
            mIsCursor = PR_TRUE;
        else if (sscanf(mPos, "#define %*s %u #define %*s %u unsigned", &mWidth, &mHeight) == 2)
            mIsCursor = PR_FALSE;
        else
             
            return NS_OK;

        
        if (strstr(mPos, " char "))
            mIsX10 = PR_FALSE;
        
        else if (strstr(mPos, " short "))
            mIsX10 = PR_TRUE;
        else
            
            return NS_OK;

        mImage->Init(mWidth, mHeight, mObserver);
        mObserver->OnStartContainer(nsnull, mImage);

        nsresult rv = mFrame->Init(0, 0, mWidth, mHeight, gfxIFormats::RGB_A1, 24);
        if (NS_FAILED(rv))
            return rv;

        if (mIsCursor) {
            nsCOMPtr<nsIProperties> props(do_QueryInterface(mImage));
            if (props) {
                nsCOMPtr<nsISupportsPRUint32> intwrapx = do_CreateInstance("@mozilla.org/supports-PRUint32;1");
                nsCOMPtr<nsISupportsPRUint32> intwrapy = do_CreateInstance("@mozilla.org/supports-PRUint32;1");

                if (intwrapx && intwrapy) {
                    intwrapx->SetData(mXHotspot);
                    intwrapy->SetData(mYHotspot);

                    props->Set("hotspotX", intwrapx);
                    props->Set("hotspotY", intwrapy);
                }
            }
        }

        mImage->AppendFrame(mFrame);
        mObserver->OnStartFrame(nsnull, mFrame);

        PRUint32 bpr;
        mFrame->GetImageBytesPerRow(&bpr);
        PRUint32 abpr;
        mFrame->GetAlphaBytesPerRow(&abpr);

        mAlphaRow = (PRUint8*)malloc(abpr);
        if (!mAlphaRow) {
          mState = RECV_DONE;
          return NS_ERROR_OUT_OF_MEMORY;
        }

        mState = RECV_SEEK;

        mCurRow = 0;
        mCurCol = 0;

    }
    if (mState == RECV_SEEK) {
        if ((endPtr = strchr(mPos, '{')) != NULL) {
            mPos = endPtr+1;
            mState = RECV_DATA;
        } else {
            mPos = mBuf + mBufSize;
            return NS_OK;
        }
    }
    if (mState == RECV_DATA) {
        PRUint32 bpr;
        mFrame->GetImageBytesPerRow(&bpr);
        PRUint32 abpr;
        mFrame->GetAlphaBytesPerRow(&abpr);
        PRBool hiByte = PR_TRUE;
        PRBool chunkHasNoAlpha = PR_TRUE;

        do {
            PRUint32 pixel = strtoul(mPos, &endPtr, 0);
            if (endPtr == mPos)
                return NS_OK;   
            if (!*endPtr)
                return NS_OK;   
            if (pixel == 0 && *endPtr == 'x')
                return NS_OK;   
            while (*endPtr && isspace(*endPtr))
                endPtr++;       

            if (!*endPtr) {
                
                return NS_OK;
            } else if (*endPtr != ',') {
                *endPtr = '\0';
                mState = RECV_DONE;  
            }
            if (!mIsX10 || !hiByte)
                mPos = endPtr; 
            if (mIsX10) {
                
                if (hiByte)
                    pixel >>= 8;
                hiByte = !hiByte;
            }

            PRUint32 *ar = ((PRUint32*)mAlphaRow) + mCurCol;
            const int alphas = PR_MIN(8, mWidth - mCurCol);
            for (int i = 0; i < alphas; i++) {
                const PRUint8 val = ((pixel & (1 << i)) >> i) ? 255 : 0;
                *ar++ = (val << 24) | 0;
                if (val == 0)
                    chunkHasNoAlpha = PR_FALSE;
            }

            mCurCol = PR_MIN(mCurCol + 8, mWidth);
            if (mCurCol == mWidth || mState == RECV_DONE) {
                mFrame->SetImageData(mAlphaRow, abpr, mCurRow * abpr);

                nsIntRect r(0, mCurRow, mWidth, 1);
                mObserver->OnDataAvailable(nsnull, mFrame, &r);

                if ((mCurRow + 1) == mHeight) {
                    mState = RECV_DONE;
                    return mObserver->OnStopFrame(nsnull, mFrame);
                }
                mCurRow++;
                mCurCol = 0;
            }

            
            NS_ASSERTION(mState != RECV_DATA || *mPos == ',' ||
                         (mIsX10 && hiByte),
                         "Must be a comma");
            if (*mPos == ',')
                mPos++;
        } while ((mState == RECV_DATA) && *mPos);

        if (!chunkHasNoAlpha)
            mHasNoAlpha = PR_FALSE;
    }

    return NS_OK;
}


