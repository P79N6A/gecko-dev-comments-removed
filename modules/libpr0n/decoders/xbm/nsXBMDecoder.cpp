








































#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "nsXBMDecoder.h"

#include "nsIInputStream.h"
#include "nsIComponentManager.h"
#include "nsIInterfaceRequestorUtils.h"

#include "imgILoad.h"

#include "nsIProperties.h"
#include "nsISupportsPrimitives.h"

#include "gfxColor.h"
#include "nsIInterfaceRequestorUtils.h"


static const PRUint32 kColors[2] = {
    GFX_PACKED_PIXEL(0, 0, 0, 0),     
    GFX_PACKED_PIXEL(255, 0, 0, 0)    
};

NS_IMPL_ISUPPORTS1(nsXBMDecoder, imgIDecoder)

nsXBMDecoder::nsXBMDecoder() : mBuf(nsnull), mPos(nsnull), mImageData(nsnull)
{
}

nsXBMDecoder::~nsXBMDecoder()
{
    if (mBuf)
        free(mBuf);
}

NS_IMETHODIMP nsXBMDecoder::Init(imgILoad *aLoad)
{
    nsresult rv;
    mObserver = do_QueryInterface(aLoad);

    mImage = do_CreateInstance("@mozilla.org/image/container;2", &rv);
    if (NS_FAILED(rv))
        return rv;

    aLoad->SetImage(mImage);

    mCurRow = mBufSize = mWidth = mHeight = 0;
    mState = RECV_HEADER;

    return NS_OK;
}

NS_IMETHODIMP nsXBMDecoder::Close()
{
    mImage->DecodingComplete();

    mObserver->OnStopContainer(nsnull, mImage);
    mObserver->OnStopDecode(nsnull, NS_OK, nsnull);
    mObserver = nsnull;
    mImage = nsnull;
    mImageData = nsnull;

    return NS_OK;
}

NS_IMETHODIMP nsXBMDecoder::Flush()
{
    return NS_OK;
}

NS_METHOD nsXBMDecoder::ReadSegCb(nsIInputStream* aIn, void* aClosure,
                             const char* aFromRawSegment, PRUint32 aToOffset,
                             PRUint32 aCount, PRUint32 *aWriteCount) {
    nsXBMDecoder *decoder = reinterpret_cast<nsXBMDecoder*>(aClosure);
    *aWriteCount = aCount;

    nsresult rv = decoder->ProcessData(aFromRawSegment, aCount);

    if (NS_FAILED(rv)) {
        *aWriteCount = 0;
    }

    return rv;
}

NS_IMETHODIMP nsXBMDecoder::WriteFrom(nsIInputStream *aInStr, PRUint32 aCount, PRUint32 *aRetval)
{
    nsresult rv = aInStr->ReadSegments(ReadSegCb, this, aCount, aRetval);
    
    if (aCount != *aRetval) { 
        *aRetval = aCount; 
        return NS_ERROR_FAILURE; 
    }
    
    return rv;    
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

        PRUint32 imageLen;
        nsresult rv = mImage->AppendFrame(0, 0, mWidth, mHeight, gfxASurface::ImageFormatARGB32,
                                          (PRUint8**)&mImageData, &imageLen);
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

        mObserver->OnStartFrame(nsnull, 0);

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
        PRUint32 *ar = mImageData + mCurRow * mWidth + mCurCol;

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
            }
            if (*endPtr != ',') {
                *endPtr = '\0';
                mState = RECV_DONE;  
            } else {
                
                endPtr++;
            }
            mPos = endPtr;
            PRUint32 numPixels = 8;
            if (mIsX10) { 
                pixel = (pixel >> 8) | ((pixel&0xFF) << 8);
                numPixels = 16;
            }
            numPixels = PR_MIN(numPixels, mWidth - mCurCol);
            for (PRUint32 i = numPixels; i > 0; --i) {
                *ar++ = kColors[pixel & 1];
                pixel >>= 1;
            }
            mCurCol += numPixels;
            if (mCurCol == mWidth || mState == RECV_DONE) {
                nsIntRect r(0, mCurRow, mWidth, 1);
                nsresult rv = mImage->FrameUpdated(0, r);
                if (NS_FAILED(rv)) {
                  return rv;
                }

                mObserver->OnDataAvailable(nsnull, PR_TRUE, &r);

                mCurRow++;
                if (mCurRow == mHeight) {
                    mState = RECV_DONE;
                    return mObserver->OnStopFrame(nsnull, 0);
                }
                mCurCol = 0;
            }
        } while ((mState == RECV_DATA) && *mPos);
    }

    return NS_OK;
}


