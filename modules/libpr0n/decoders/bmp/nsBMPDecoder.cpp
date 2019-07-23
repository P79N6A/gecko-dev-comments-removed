









































#include <stdlib.h>

#include "nsBMPDecoder.h"

#include "nsIInputStream.h"
#include "nsIComponentManager.h"
#include "imgIContainerObserver.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"

#include "imgILoad.h"
#include "nsIImage.h"

#include "prlog.h"

#ifdef PR_LOGGING
PRLogModuleInfo *gBMPLog = PR_NewLogModule("BMPDecoder");
#endif


#define LINE(row) ((mBIH.height < 0) ? (-mBIH.height - (row)) : ((row) - 1))
#define PIXEL_OFFSET(row, col) (LINE(row) * mBIH.width + col)

NS_IMPL_ISUPPORTS1(nsBMPDecoder, imgIDecoder)

nsBMPDecoder::nsBMPDecoder()
{
    mColors = nsnull;
    mRow = nsnull;
    mCurPos = mPos = mNumColors = mRowBytes = 0;
    mOldLine = mCurLine = 1; 
    mState = eRLEStateInitial;
    mStateData = 0;
    mLOH = WIN_HEADER_LENGTH;
}

nsBMPDecoder::~nsBMPDecoder()
{
    delete[] mColors;
    if (mRow)
        free(mRow);
}

NS_IMETHODIMP nsBMPDecoder::Init(imgILoad *aLoad)
{
    PR_LOG(gBMPLog, PR_LOG_DEBUG, ("nsBMPDecoder::Init(%p)\n", aLoad));
    mObserver = do_QueryInterface(aLoad);

    nsresult rv;
    mImage = do_CreateInstance("@mozilla.org/image/container;1", &rv);
    if (NS_FAILED(rv))
        return rv;

    mFrame = do_CreateInstance("@mozilla.org/gfx/image/frame;2", &rv);
    if (NS_FAILED(rv))
        return rv;

    return aLoad->SetImage(mImage);
}

NS_IMETHODIMP nsBMPDecoder::Close()
{
    PR_LOG(gBMPLog, PR_LOG_DEBUG, ("nsBMPDecoder::Close()\n"));
    if (mObserver) {
        mObserver->OnStopFrame(nsnull, mFrame);
        mObserver->OnStopContainer(nsnull, mImage);
        mObserver->OnStopDecode(nsnull, NS_OK, nsnull);
        mObserver = nsnull;
    }
    mImage = nsnull;
    mFrame = nsnull;
    return NS_OK;
}

NS_IMETHODIMP nsBMPDecoder::Flush()
{
    mFrame->SetMutable(PR_FALSE);
    return NS_OK;
}

NS_METHOD nsBMPDecoder::ReadSegCb(nsIInputStream* aIn, void* aClosure,
                             const char* aFromRawSegment, PRUint32 aToOffset,
                             PRUint32 aCount, PRUint32 *aWriteCount) {
    nsBMPDecoder *decoder = reinterpret_cast<nsBMPDecoder*>(aClosure);
    *aWriteCount = aCount;
    return decoder->ProcessData(aFromRawSegment, aCount);
}

NS_IMETHODIMP nsBMPDecoder::WriteFrom(nsIInputStream *aInStr, PRUint32 aCount, PRUint32 *aRetval)
{
    PR_LOG(gBMPLog, PR_LOG_DEBUG, ("nsBMPDecoder::WriteFrom(%p, %lu, %p)\n", aInStr, aCount, aRetval));

    return aInStr->ReadSegments(ReadSegCb, this, aCount, aRetval);
}





static void calcBitmask(PRUint32 aMask, PRUint8& aBegin, PRUint8& aLength)
{
    
    PRUint8 pos;
    PRBool started = PR_FALSE;
    aBegin = aLength = 0;
    for (pos = 0; pos <= 31; pos++) {
        if (!started && (aMask & (1 << pos))) {
            aBegin = pos;
            started = PR_TRUE;
        }
        else if (started && !(aMask & (1 << pos))) {
            aLength = pos - aBegin;
            break;
        }
    }
}

NS_METHOD nsBMPDecoder::CalcBitShift()
{
    PRUint8 begin, length;
    
    calcBitmask(mBitFields.red, begin, length);
    mBitFields.redRightShift = begin;
    mBitFields.redLeftShift = 8 - length;
    
    calcBitmask(mBitFields.green, begin, length);
    mBitFields.greenRightShift = begin;
    mBitFields.greenLeftShift = 8 - length;
    
    calcBitmask(mBitFields.blue, begin, length);
    mBitFields.blueRightShift = begin;
    mBitFields.blueLeftShift = 8 - length;
    return NS_OK;
}

NS_METHOD nsBMPDecoder::ProcessData(const char* aBuffer, PRUint32 aCount)
{
    PR_LOG(gBMPLog, PR_LOG_DEBUG, ("nsBMPDecoder::ProcessData(%p, %lu)", aBuffer, aCount));
    if (!aCount || !mCurLine) 
        return NS_OK;

    nsresult rv;
    if (mPos < BFH_LENGTH) { 
        PRUint32 toCopy = BFH_LENGTH - mPos;
        if (toCopy > aCount)
            toCopy = aCount;
        memcpy(mRawBuf + mPos, aBuffer, toCopy);
        mPos += toCopy;
        aCount -= toCopy;
        aBuffer += toCopy;
    }
    if (mPos == BFH_LENGTH) {
        rv = mObserver->OnStartDecode(nsnull);
        NS_ENSURE_SUCCESS(rv, rv);
        ProcessFileHeader();
        if (mBFH.signature[0] != 'B' || mBFH.signature[1] != 'M')
            return NS_ERROR_FAILURE;
        if (mBFH.bihsize == OS2_BIH_LENGTH)
            mLOH = OS2_HEADER_LENGTH;
    }
    if (mPos >= BFH_LENGTH && mPos < mLOH) { 
        PRUint32 toCopy = mLOH - mPos;
        if (toCopy > aCount)
            toCopy = aCount;
        memcpy(mRawBuf + (mPos - BFH_LENGTH), aBuffer, toCopy);
        mPos += toCopy;
        aCount -= toCopy;
        aBuffer += toCopy;
    }
    if (mPos == mLOH) {
        ProcessInfoHeader();
        PR_LOG(gBMPLog, PR_LOG_DEBUG, ("BMP image is %lix%lix%lu. compression=%lu\n",
            mBIH.width, mBIH.height, mBIH.bpp, mBIH.compression));
        
        if (mBIH.bpp != 1 && mBIH.bpp != 4 && mBIH.bpp != 8 &&
            mBIH.bpp != 16 && mBIH.bpp != 24 && mBIH.bpp != 32)
          return NS_ERROR_UNEXPECTED;

        if (mBIH.bpp <= 8) {
            mNumColors = 1 << mBIH.bpp;
            if (mBIH.colors && mBIH.colors < mNumColors)
                mNumColors = mBIH.colors;

            mColors = new colorTable[mNumColors];
            if (!mColors)
                return NS_ERROR_OUT_OF_MEMORY;
        }
        else if (mBIH.compression != BI_BITFIELDS && mBIH.bpp == 16) {
            
            mBitFields.red   = 0x7C00;
            mBitFields.green = 0x03E0;
            mBitFields.blue  = 0x001F;
            CalcBitShift();
        }
        
        
        const PRInt32 k64KWidth = 0x0000FFFF;
        if (mBIH.width < 0 || mBIH.width > k64KWidth)
            return NS_ERROR_FAILURE;

        PRUint32 real_height = (mBIH.height > 0) ? mBIH.height : -mBIH.height;
        rv = mImage->Init(mBIH.width, real_height, mObserver);
        NS_ENSURE_SUCCESS(rv, rv);
        rv = mObserver->OnStartContainer(nsnull, mImage);
        NS_ENSURE_SUCCESS(rv, rv);
        mOldLine = mCurLine = real_height;

        if ((mBIH.compression == BI_RLE8) || (mBIH.compression == BI_RLE4)) {
            rv = mFrame->Init(0, 0, mBIH.width, real_height, RLE_GFXFORMAT_ALPHA, 24);
        } else {
            
            mRow = (PRUint8*)malloc((mBIH.width * mBIH.bpp)/8 + 4);
            
            
            
            if (!mRow) {
                return NS_ERROR_OUT_OF_MEMORY;
            }
            rv = mFrame->Init(0, 0, mBIH.width, real_height, BMP_GFXFORMAT, 24);
        }
        NS_ENSURE_SUCCESS(rv, rv);

        PRUint32 imageLength;
        mFrame->GetImageData((PRUint8**)&mImageData, &imageLength);
        if (!mImageData)
            return NS_ERROR_FAILURE;

        
        if ((mBIH.compression == BI_RLE8) || (mBIH.compression == BI_RLE4)) {
            if (((mBIH.compression == BI_RLE8) && (mBIH.bpp != 8)) 
             || ((mBIH.compression == BI_RLE4) && (mBIH.bpp != 4) && (mBIH.bpp != 1))) {
                PR_LOG(gBMPLog, PR_LOG_DEBUG, ("BMP RLE8/RLE4 compression only supports 8/4 bits per pixel\n"));
                return NS_ERROR_FAILURE;
            }
            
            memset(mImageData, 0, imageLength);
        }

        rv = mImage->AppendFrame(mFrame);
        NS_ENSURE_SUCCESS(rv, rv);
        mObserver->OnStartFrame(nsnull, mFrame);
        NS_ENSURE_SUCCESS(rv, rv);
    }
    PRUint8 bpc; 
    bpc = (mBFH.bihsize == OS2_BIH_LENGTH) ? 3 : 4; 
    if (mColors && (mPos >= mLOH && (mPos < (mLOH + mNumColors * bpc)))) {
        
        PRUint32 colorBytes = mPos - mLOH; 
        PRUint8 colorNum = colorBytes / bpc; 
        PRUint8 at = colorBytes % bpc;
        while (aCount && (mPos < (mLOH + mNumColors * bpc))) {
            switch (at) {
                case 0:
                    mColors[colorNum].blue = *aBuffer;
                    break;
                case 1:
                    mColors[colorNum].green = *aBuffer;
                    break;
                case 2:
                    mColors[colorNum].red = *aBuffer;
                    colorNum++;
                    break;
                case 3:
                    
                    break;
            }
            mPos++; aBuffer++; aCount--;
            at = (at + 1) % bpc;
        }
    }
    else if (aCount && mBIH.compression == BI_BITFIELDS && mPos < (WIN_HEADER_LENGTH + BITFIELD_LENGTH)) {
        
        
        PRUint32 toCopy = (WIN_HEADER_LENGTH + BITFIELD_LENGTH) - mPos;
        if (toCopy > aCount)
            toCopy = aCount;
        memcpy(mRawBuf + (mPos - WIN_HEADER_LENGTH), aBuffer, toCopy);
        mPos += toCopy;
        aBuffer += toCopy;
        aCount -= toCopy;
    }
    if (mBIH.compression == BI_BITFIELDS && mPos == WIN_HEADER_LENGTH + BITFIELD_LENGTH) {
        mBitFields.red = LITTLE_TO_NATIVE32(*(PRUint32*)mRawBuf);
        mBitFields.green = LITTLE_TO_NATIVE32(*(PRUint32*)(mRawBuf + 4));
        mBitFields.blue = LITTLE_TO_NATIVE32(*(PRUint32*)(mRawBuf + 8));
        CalcBitShift();
    }
    while (aCount && (mPos < mBFH.dataoffset)) { 
        mPos++; aBuffer++; aCount--;
    }
    if (aCount && ++mPos >= mBFH.dataoffset) {
        
        
        if (!mBIH.compression || mBIH.compression == BI_BITFIELDS) {
            PRUint32 rowSize = (mBIH.bpp * mBIH.width + 7) / 8; 
            if (rowSize % 4)
                rowSize += (4 - (rowSize % 4)); 
            PRUint32 toCopy;
            do {
                toCopy = rowSize - mRowBytes;
                if (toCopy) {
                    if (toCopy > aCount)
                        toCopy = aCount;
                    memcpy(mRow + mRowBytes, aBuffer, toCopy);
                    aCount -= toCopy;
                    aBuffer += toCopy;
                    mRowBytes += toCopy;
                }
                if (rowSize == mRowBytes) {
                    
                    PRUint8* p = mRow;
                    PRUint32* d = mImageData + PIXEL_OFFSET(mCurLine, 0);
                    PRUint32 lpos = mBIH.width;
                    switch (mBIH.bpp) {
                      case 1:
                        while (lpos > 0) {
                          PRInt8 bit;
                          PRUint8 idx;
                          for (bit = 7; bit >= 0 && lpos > 0; bit--) {
                              idx = (*p >> bit) & 1;
                              SetPixel(d, idx, mColors);
                              --lpos;
                          }
                          ++p;
                        }
                        break;
                      case 4:
                        while (lpos > 0) {
                          Set4BitPixel(d, *p, lpos, mColors);
                          ++p;
                        }
                        break;
                      case 8:
                        while (lpos > 0) {
                          SetPixel(d, *p, mColors);
                          --lpos;
                          ++p;
                        }
                        break;
                      case 16:
                        while (lpos > 0) {
                          PRUint16 val = LITTLE_TO_NATIVE16(*(PRUint16*)p);
                          SetPixel(d,
                                  (val & mBitFields.red) >> mBitFields.redRightShift << mBitFields.redLeftShift,
                                  (val & mBitFields.green) >> mBitFields.greenRightShift << mBitFields.greenLeftShift,
                                  (val & mBitFields.blue) >> mBitFields.blueRightShift << mBitFields.blueLeftShift);
                          --lpos;
                          p+=2;
                        }
                        break;
                      case 32:
                      case 24:
                        while (lpos > 0) {
                          SetPixel(d, p[2], p[1], p[0]);
                          p += 2;
                          --lpos;
                          if (mBIH.bpp == 32)
                            p++; 
                          ++p;
                        }
                        break;
                      default:
                        NS_NOTREACHED("Unsupported color depth, but earlier check didn't catch it");
                    }
                    mCurLine --;
                    if (mCurLine == 0) { 
                        break;
                    }
                    mRowBytes = 0;

                }
            } while (aCount > 0);
        } 
        else if ((mBIH.compression == BI_RLE8) || (mBIH.compression == BI_RLE4)) {
            if (((mBIH.compression == BI_RLE8) && (mBIH.bpp != 8)) 
             || ((mBIH.compression == BI_RLE4) && (mBIH.bpp != 4) && (mBIH.bpp != 1))) {
                PR_LOG(gBMPLog, PR_LOG_DEBUG, ("BMP RLE8/RLE4 compression only supports 8/4 bits per pixel\n"));
                return NS_ERROR_FAILURE;
            }

            while (aCount > 0) {
                PRUint8 byte;

                switch(mState) {
                    case eRLEStateInitial:
                        mStateData = (PRUint8)*aBuffer++;
                        aCount--;

                        mState = eRLEStateNeedSecondEscapeByte;
                        continue;

                    case eRLEStateNeedSecondEscapeByte:
                        byte = *aBuffer++;
                        aCount--;
                        if (mStateData != RLE_ESCAPE) { 
                            
                            
                            
                            
                            
                            
                            mState = eRLEStateInitial;
                            PRUint32 pixelsNeeded = PR_MIN((PRUint32)(mBIH.width - mCurPos), mStateData);
                            if (pixelsNeeded) {
                                PRUint32* d = mImageData + PIXEL_OFFSET(mCurLine, mCurPos);
                                mCurPos += pixelsNeeded;
                                if (mBIH.compression == BI_RLE8) {
                                    do {
                                        SetPixel(d, byte, mColors);
                                        pixelsNeeded --;
                                    } while (pixelsNeeded);
                                } else {
                                    do {
                                        Set4BitPixel(d, byte, pixelsNeeded, mColors);
                                    } while (pixelsNeeded);
                                }
                            }
                            continue;
                        }

                        switch(byte) {
                            case RLE_ESCAPE_EOL:
                                
                                mCurLine --;
                                mCurPos = 0;
                                mState = eRLEStateInitial;
                                break;

                            case RLE_ESCAPE_EOF: 
                                mCurPos = mCurLine = 0;
                                break;

                            case RLE_ESCAPE_DELTA:
                                mState = eRLEStateNeedXDelta;
                                continue;

                            default : 
                                
                                mStateData = byte;
                                if (mCurPos + mStateData > (PRUint32)mBIH.width) {
                                    
                                    
                                    mStateData -= mBIH.width & 1;
                                    if (mCurPos + mStateData > (PRUint32)mBIH.width)
                                        return NS_ERROR_FAILURE;
                                }

                                
                                
                                
                                
                                
                                
                                
                                
                                
                                if (((mStateData - 1) & mBIH.compression) != 0)
                                    mState = eRLEStateAbsoluteMode;
                                else
                                    mState = eRLEStateAbsoluteModePadded;
                                continue;
                        }
                        break;

                    case eRLEStateNeedXDelta:
                        
                        byte = *aBuffer++;
                        aCount--;
                        mCurPos += byte;
                        if (mCurPos > mBIH.width)
                            mCurPos = mBIH.width;

                        mState = eRLEStateNeedYDelta;
                        continue;

                    case eRLEStateNeedYDelta:
                        
                        byte = *aBuffer++;
                        aCount--;
                        mState = eRLEStateInitial;
                        mCurLine -= PR_MIN(byte, mCurLine);
                        break;

                    case eRLEStateAbsoluteMode: 
                    case eRLEStateAbsoluteModePadded:
                        if (mStateData) {
                            
                            
                            
                            
                            PRUint32* d = mImageData + PIXEL_OFFSET(mCurLine, mCurPos);
                            PRUint32* oldPos = d;
                            if (mBIH.compression == BI_RLE8) {
                                while (aCount > 0 && mStateData > 0) {
                                    byte = *aBuffer++;
                                    aCount--;
                                    SetPixel(d, byte, mColors);
                                    mStateData--;
                                }
                            } else {
                                while (aCount > 0 && mStateData > 0) {
                                    byte = *aBuffer++;
                                    aCount--;
                                    Set4BitPixel(d, byte, mStateData, mColors);
                                }
                            }
                            mCurPos += d - oldPos;
                        }

                        if (mStateData == 0) {
                            
                            

                            if (mState == eRLEStateAbsoluteMode) { 
                                mState = eRLEStateInitial;
                            } else if (aCount > 0) {               
                                
                                
                                aBuffer++;
                                aCount--;
                                mState = eRLEStateInitial;
                            }
                        }
                        
                        continue;

                    default :
                        NS_NOTREACHED("BMP RLE decompression: unknown state!");
                        return NS_ERROR_FAILURE;
                }
                
                
                if (mCurLine == 0) { 
                    break;
                }
            }
        }
    }
    
    const PRUint32 rows = mOldLine - mCurLine;
    if (rows) {
        nsIntRect r(0, LINE(mCurLine), mBIH.width, rows);

        
        nsCOMPtr<nsIImage> img(do_GetInterface(mFrame));
        if (!img)
            return PR_FALSE;
        img->ImageUpdated(nsnull, nsImageUpdateFlags_kBitsChanged, &r);

        mObserver->OnDataAvailable(nsnull, mFrame, &r);
        mOldLine = mCurLine;
    }

    return NS_OK;
}

void nsBMPDecoder::ProcessFileHeader()
{
    memset(&mBFH, 0, sizeof(mBFH));
    memcpy(&mBFH.signature, mRawBuf, sizeof(mBFH.signature));
    memcpy(&mBFH.filesize, mRawBuf + 2, sizeof(mBFH.filesize));
    memcpy(&mBFH.reserved, mRawBuf + 6, sizeof(mBFH.reserved));
    memcpy(&mBFH.dataoffset, mRawBuf + 10, sizeof(mBFH.dataoffset));
    memcpy(&mBFH.bihsize, mRawBuf + 14, sizeof(mBFH.bihsize));

    
    mBFH.filesize = LITTLE_TO_NATIVE32(mBFH.filesize);
    mBFH.dataoffset = LITTLE_TO_NATIVE32(mBFH.dataoffset);
    mBFH.bihsize = LITTLE_TO_NATIVE32(mBFH.bihsize);
}

void nsBMPDecoder::ProcessInfoHeader()
{
    memset(&mBIH, 0, sizeof(mBIH));
    if (mBFH.bihsize == 12) { 
        memcpy(&mBIH.width, mRawBuf, 2);
        memcpy(&mBIH.height, mRawBuf + 2, 2);
        memcpy(&mBIH.planes, mRawBuf + 4, sizeof(mBIH.planes));
        memcpy(&mBIH.bpp, mRawBuf + 6, sizeof(mBIH.bpp));
    }
    else {
        memcpy(&mBIH.width, mRawBuf, sizeof(mBIH.width));
        memcpy(&mBIH.height, mRawBuf + 4, sizeof(mBIH.height));
        memcpy(&mBIH.planes, mRawBuf + 8, sizeof(mBIH.planes));
        memcpy(&mBIH.bpp, mRawBuf + 10, sizeof(mBIH.bpp));
        memcpy(&mBIH.compression, mRawBuf + 12, sizeof(mBIH.compression));
        memcpy(&mBIH.image_size, mRawBuf + 16, sizeof(mBIH.image_size));
        memcpy(&mBIH.xppm, mRawBuf + 20, sizeof(mBIH.xppm));
        memcpy(&mBIH.yppm, mRawBuf + 24, sizeof(mBIH.yppm));
        memcpy(&mBIH.colors, mRawBuf + 28, sizeof(mBIH.colors));
        memcpy(&mBIH.important_colors, mRawBuf + 32, sizeof(mBIH.important_colors));
    }

    
    mBIH.width = LITTLE_TO_NATIVE32(mBIH.width);
    mBIH.height = LITTLE_TO_NATIVE32(mBIH.height);
    mBIH.planes = LITTLE_TO_NATIVE16(mBIH.planes);
    mBIH.bpp = LITTLE_TO_NATIVE16(mBIH.bpp);

    mBIH.compression = LITTLE_TO_NATIVE32(mBIH.compression);
    mBIH.image_size = LITTLE_TO_NATIVE32(mBIH.image_size);
    mBIH.xppm = LITTLE_TO_NATIVE32(mBIH.xppm);
    mBIH.yppm = LITTLE_TO_NATIVE32(mBIH.yppm);
    mBIH.colors = LITTLE_TO_NATIVE32(mBIH.colors);
    mBIH.important_colors = LITTLE_TO_NATIVE32(mBIH.important_colors);
}
