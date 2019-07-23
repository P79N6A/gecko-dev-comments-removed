






































#include "nsImageWin.h"
#include "nsRenderingContextWin.h"
#include "nsDeviceContextWin.h"
#include "imgScaler.h"
#include "nsComponentManagerUtils.h"

static PRInt32 GetPlatform()
{
  OSVERSIONINFO versionInfo;


  versionInfo.dwOSVersionInfoSize = sizeof(versionInfo);
  ::GetVersionEx(&versionInfo);
  return versionInfo.dwPlatformId;
}

static PRInt32 GetOsMajorVersion()
{
  OSVERSIONINFO versionInfo;

  versionInfo.dwOSVersionInfoSize = sizeof(versionInfo);
  ::GetVersionEx(&versionInfo);
  return versionInfo.dwMajorVersion;
}


PRInt32 nsImageWin::gPlatform = GetPlatform();
PRInt32 nsImageWin::gOsMajorVersion = GetOsMajorVersion();






nsImageWin::nsImageWin()
  : mImageBits(nsnull)
  , mHBitmap(nsnull)
  , mAlphaBits(nsnull)
  , mColorMap(nsnull)
  , mBHead(nsnull)
  , mDIBTemp(PR_FALSE)
  , mNumBytesPixel(0)
  , mNumPaletteColors(0)
  , mSizeImage(0)
  , mRowBytes(0)
  , mIsOptimized(PR_FALSE)
  , mDecodedX1(PR_INT32_MAX)
  , mDecodedY1(PR_INT32_MAX)
  , mDecodedX2(0)
  , mDecodedY2(0)
  , mIsLocked(PR_FALSE)
  , mAlphaDepth(0)
  , mARowBytes(0)
  , mImageCache(0)
  , mInitialized(PR_FALSE)
  , mWantsOptimization(PR_FALSE)
  , mTimer(nsnull)
  , mImagePreMultiplied(PR_FALSE)
{
}






nsImageWin :: ~nsImageWin()
{
  if (mTimer)
    mTimer->Cancel();

  CleanUpDDB();
  CleanUpDIB();

  if (mBHead) {
    delete[] mBHead;
    mBHead = nsnull;
  }
  if (mAlphaBits) {
    delete [] mAlphaBits;
    mAlphaBits = nsnull;
  }
}


NS_IMPL_ISUPPORTS1(nsImageWin, nsIImage)






nsresult nsImageWin::Init(PRInt32 aWidth, PRInt32 aHeight, PRInt32 aDepth,
                          nsMaskRequirements aMaskRequirements)
{
  if (mInitialized)
    return NS_ERROR_FAILURE;

  if (8 == aDepth) {
    mNumPaletteColors = 256;
    mNumBytesPixel = 1;
  } else if (24 == aDepth) {
    mNumBytesPixel = 3;
  } else {
    NS_ASSERTION(PR_FALSE, "unexpected image depth");
    return NS_ERROR_UNEXPECTED;
  }

  
  const PRInt32 k64KLimit = 0x0000FFFF;
  if (aWidth > k64KLimit || aHeight > k64KLimit)
    return NS_ERROR_FAILURE;

  if (0 == mNumPaletteColors) {
    
    mBHead = (LPBITMAPINFOHEADER)new char[sizeof(BITMAPINFO)];
  } else {
    
    
    
    mBHead = (LPBITMAPINFOHEADER)new char[sizeof(BITMAPINFOHEADER) +
                                          (256 * sizeof(WORD))];
  }
  if (!mBHead)
    return NS_ERROR_OUT_OF_MEMORY;

  mBHead->biSize = sizeof(BITMAPINFOHEADER);
  mBHead->biWidth = aWidth;
  mBHead->biHeight = aHeight;
  mBHead->biPlanes = 1;
  mBHead->biBitCount = (WORD)aDepth;
  mBHead->biCompression = BI_RGB;
  mBHead->biSizeImage = 0;     
  mBHead->biXPelsPerMeter = 0;
  mBHead->biYPelsPerMeter = 0;
  mBHead->biClrUsed = mNumPaletteColors;
  mBHead->biClrImportant = mNumPaletteColors;

  
  mRowBytes = CalcBytesSpan(mBHead->biWidth);
  mSizeImage = mRowBytes * mBHead->biHeight; 

  
  mImageBits = new unsigned char[mSizeImage];
  if (!mImageBits) {
    delete[] mBHead;
    mBHead = nsnull;
    return NS_ERROR_OUT_OF_MEMORY;
  }

  
  









  if (256 == mNumPaletteColors) {
    
    WORD* palIndx = (WORD*)(((LPBYTE)mBHead) + mBHead->biSize);
    for (WORD index = 0; index < 256; index++) {
      *palIndx++ = index;
    }
  }

  
  if (aMaskRequirements != nsMaskRequirements_kNoMask) {
    if (nsMaskRequirements_kNeeds1Bit == aMaskRequirements) {
      mARowBytes = (aWidth + 7) / 8;
      mAlphaDepth = 1;
    }else{
      
      
      mARowBytes = aWidth;
      mAlphaDepth = 8;
    }

    
    mARowBytes = (mARowBytes + 3) & ~0x3;
    mAlphaBits = new unsigned char[mARowBytes * aHeight];
    if (!mAlphaBits) {
      delete[] mBHead;
      mBHead = nsnull;
      delete[] mImageBits;
      mImageBits = nsnull;
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }


  
  mColorMap = new nsColorMap;

  if (mColorMap != nsnull) {
    mColorMap->NumColors = mNumPaletteColors;
    mColorMap->Index = nsnull;
    if (mColorMap->NumColors > 0) {
      mColorMap->Index = new PRUint8[3 * mColorMap->NumColors];

      
      
      
      memset(mColorMap->Index, 0, sizeof(PRUint8) * (3 * mColorMap->NumColors));
    }
  }

  mInitialized = PR_TRUE;
  return NS_OK;
}






void 
nsImageWin :: ImageUpdated(nsIDeviceContext *aContext, PRUint8 aFlags, nsRect *aUpdateRect)
{
  mDecodedX1 = PR_MIN(mDecodedX1, aUpdateRect->x);
  mDecodedY1 = PR_MIN(mDecodedY1, aUpdateRect->y);

  if (aUpdateRect->YMost() > mDecodedY2)
    mDecodedY2 = aUpdateRect->YMost();
  if (aUpdateRect->XMost() > mDecodedX2)
    mDecodedX2 = aUpdateRect->XMost();
}




PRBool nsImageWin::GetIsImageComplete() {
  return mInitialized &&
         mDecodedX1 == 0 &&
         mDecodedY1 == 0 &&
         mDecodedX2 == mBHead->biWidth &&
         mDecodedY2 == mBHead->biHeight;
}



struct MONOBITMAPINFO {
  BITMAPINFOHEADER  bmiHeader;
  RGBQUAD           bmiColors[2];


  MONOBITMAPINFO(LONG aWidth, LONG aHeight)
  {
    memset(&bmiHeader, 0, sizeof(bmiHeader));
    bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmiHeader.biWidth = aWidth;
    bmiHeader.biHeight = aHeight;
    bmiHeader.biPlanes = 1;
    bmiHeader.biBitCount = 1;


    
    
    
    
    
    bmiColors[0].rgbBlue = 255;
    bmiColors[0].rgbGreen = 255;
    bmiColors[0].rgbRed = 255;
    bmiColors[0].rgbReserved = 0;
    bmiColors[1].rgbBlue = 0;
    bmiColors[1].rgbGreen = 0;
    bmiColors[1].rgbRed = 0;
    bmiColors[1].rgbReserved = 0;
  }
};


struct ALPHA8BITMAPINFO {
  BITMAPINFOHEADER  bmiHeader;
  RGBQUAD           bmiColors[256];


  ALPHA8BITMAPINFO(LONG aWidth, LONG aHeight)
  {
    memset(&bmiHeader, 0, sizeof(bmiHeader));
    bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmiHeader.biWidth = aWidth;
    bmiHeader.biHeight = aHeight;
    bmiHeader.biPlanes = 1;
    bmiHeader.biBitCount = 8;


    
     int i;
     for(i=0; i < 256; i++){
      bmiColors[i].rgbBlue = 255-i;
      bmiColors[i].rgbGreen = 255-i;
      bmiColors[i].rgbRed = 255-i;
      bmiColors[1].rgbReserved = 0;
     }
  }
};


struct ALPHA24BITMAPINFO {
  BITMAPINFOHEADER  bmiHeader;


  ALPHA24BITMAPINFO(LONG aWidth, LONG aHeight)
  {
    memset(&bmiHeader, 0, sizeof(bmiHeader));
    bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmiHeader.biWidth = aWidth;
    bmiHeader.biHeight = aHeight;
    bmiHeader.biPlanes = 1;
    bmiHeader.biBitCount = 24;
  }
};


struct ALPHA32BITMAPINFO {
  BITMAPINFOHEADER  bmiHeader;


  ALPHA32BITMAPINFO(LONG aWidth, LONG aHeight)
  {
    memset(&bmiHeader, 0, sizeof(bmiHeader));
    bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmiHeader.biWidth = aWidth;
    bmiHeader.biHeight = aHeight;
    bmiHeader.biPlanes = 1;
    bmiHeader.biBitCount = 32;
  }
};



static void CompositeBitsInMemory(HDC aTheHDC,
                                  int aDX, int aDY,
                                  int aDWidth, int aDHeight,
                                  int aSX, int aSY,
                                  int aSWidth, int aSHeight,
                                  PRInt32 aSrcy,
                                  PRUint8 *aAlphaBits,
                                  MONOBITMAPINFO *aBMI,
                                  PRUint8* aImageBits,
                                  LPBITMAPINFOHEADER aBHead,
                                  PRInt16 aNumPaletteColors);




#define MASKBLT_ROP MAKEROP4((DWORD)0x00AA0029, SRCCOPY)


void nsImageWin::CreateImageWithAlphaBits(HDC TheHDC)
{
  unsigned char *imageWithAlphaBits;
  ALPHA32BITMAPINFO bmi(mBHead->biWidth, mBHead->biHeight);
  mHBitmap = ::CreateDIBSection(TheHDC, (LPBITMAPINFO)&bmi, DIB_RGB_COLORS,
                                (LPVOID *)&imageWithAlphaBits, NULL, 0);


  if (!mHBitmap) {
    mIsOptimized = PR_FALSE;
    return;
  }
  
  if (256 == mNumPaletteColors) {
    for (int y = 0; y < mBHead->biHeight; y++) {
      unsigned char *imageWithAlphaRow = imageWithAlphaBits + y * mBHead->biWidth * 4;
      unsigned char *imageRow = mImageBits + y * mRowBytes;
      unsigned char *alphaRow = mAlphaBits + y * mARowBytes;


      for (int x = 0; x < mBHead->biWidth;
           x++, imageWithAlphaRow += 4, imageRow++, alphaRow++) {
        FAST_DIVIDE_BY_255(imageWithAlphaRow[0],
                           mColorMap->Index[3 * (*imageRow)] * *alphaRow);
        FAST_DIVIDE_BY_255(imageWithAlphaRow[1],
                           mColorMap->Index[3 * (*imageRow) + 1] * *alphaRow);
        FAST_DIVIDE_BY_255(imageWithAlphaRow[2],
                           mColorMap->Index[3 * (*imageRow) + 2] * *alphaRow);
        imageWithAlphaRow[3] = *alphaRow;
      }
    }
  } else if (mImagePreMultiplied) {
    for (int y = 0; y < mBHead->biHeight; y++) {
      unsigned char *imageWithAlphaRow = imageWithAlphaBits + y * mBHead->biWidth * 4;
      unsigned char *imageRow = mImageBits + y * mRowBytes;
      unsigned char *alphaRow = mAlphaBits + y * mARowBytes;

      for (int x = 0; x < mBHead->biWidth;
          x++, imageWithAlphaRow += 4, imageRow += 3, alphaRow++) {
        memcpy(imageWithAlphaRow, imageRow, 3);
        imageWithAlphaRow[3] = *alphaRow;
      }
    }
  } else {
    for (int y = 0; y < mBHead->biHeight; y++) {
      unsigned char *imageWithAlphaRow = imageWithAlphaBits + y * mBHead->biWidth * 4;
      unsigned char *imageRow = mImageBits + y * mRowBytes;
      unsigned char *alphaRow = mAlphaBits + y * mARowBytes;


      for (int x = 0; x < mBHead->biWidth;
          x++, imageWithAlphaRow += 4, imageRow += 3, alphaRow++) {
        FAST_DIVIDE_BY_255(imageWithAlphaRow[0], imageRow[0] * *alphaRow);
        FAST_DIVIDE_BY_255(imageWithAlphaRow[1], imageRow[1] * *alphaRow);
        FAST_DIVIDE_BY_255(imageWithAlphaRow[2], imageRow[2] * *alphaRow);
        imageWithAlphaRow[3] = *alphaRow;
      }
    }
  }
  mIsOptimized = PR_TRUE;
}





NS_IMETHODIMP 
nsImageWin::Draw(nsIRenderingContext &aContext, nsIDrawingSurface* aSurface,
                 PRInt32 aSX, PRInt32 aSY, PRInt32 aSWidth, PRInt32 aSHeight,
                 PRInt32 aDX, PRInt32 aDY, PRInt32 aDWidth, PRInt32 aDHeight)
{
  if (mBHead == nsnull || 
      aSWidth < 0 || aDWidth < 0 || aSHeight < 0 || aDHeight < 0)
    return NS_ERROR_FAILURE;

  if (0 == aSWidth || 0 == aDWidth || 0 == aSHeight || 0 == aDHeight)
    return NS_OK;

  if (mDecodedX2 < mDecodedX1 || mDecodedY2 < mDecodedY1)
    return NS_OK;

  PRInt32 origSHeight = aSHeight, origDHeight = aDHeight;
  PRInt32 origSWidth = aSWidth, origDWidth = aDWidth;

  
  if (aSX + aSWidth > mDecodedX2) {
    aDWidth -= ((aSX + aSWidth - mDecodedX2) * origDWidth) / origSWidth;
    aSWidth -= (aSX + aSWidth) - mDecodedX2;
  }
  if (aSX < mDecodedX1) {
    aDX += ((mDecodedX1 - aSX) * origDWidth) / origSWidth;
    aSX = mDecodedX1;
  }

  if (aSY + aSHeight > mDecodedY2) {
    aDHeight -= ((aSY + aSHeight - mDecodedY2) * origDHeight) / origSHeight;
    aSHeight -= (aSY + aSHeight) - mDecodedY2;
  }
  if (aSY < mDecodedY1) {
    aDY += ((mDecodedY1 - aSY) * origDHeight) / origSHeight;
    aSY = mDecodedY1;
  }

  if (aDWidth <= 0 || aDHeight <= 0)
    return NS_OK;

  
  PRInt32 srcy = mBHead->biHeight - (aSY + aSHeight);

  HDC     TheHDC;
  ((nsDrawingSurfaceWin *)aSurface)->GetDC(&TheHDC);
  if (!TheHDC)
    return NS_ERROR_FAILURE;

  
  PRInt32 canRaster;
  ((nsDrawingSurfaceWin *)aSurface)->GetTECHNOLOGY(&canRaster);

  CreateDDB();

  PRBool didComposite = PR_FALSE;
  if (!mIsOptimized || !mHBitmap) {
    DWORD rop = SRCCOPY;

    if (mAlphaBits) {
      if (1 == mAlphaDepth) {
        MONOBITMAPINFO  bmi(mBHead->biWidth, mBHead->biHeight);

        if (canRaster == DT_RASPRINTER) {
          CompositeBitsInMemory(TheHDC, aDX, aDY, aDWidth, aDHeight,
                                aSX, aSY, aSWidth, aSHeight,
                                srcy, mAlphaBits, &bmi, mImageBits, mBHead,
                                mNumPaletteColors);
          didComposite = PR_TRUE;
        } else {
          
          ::StretchDIBits(TheHDC, aDX, aDY, aDWidth, aDHeight,
                          aSX, srcy, aSWidth, aSHeight, mAlphaBits,
                          (LPBITMAPINFO)&bmi, DIB_RGB_COLORS, SRCAND);
          rop = SRCPAINT;
        }
      } else if (8 == mAlphaDepth) {
        nsresult rv = DrawComposited(TheHDC, aDX, aDY, aDWidth, aDHeight,
                                     aSX, srcy, aSWidth, aSHeight,
                                     origDWidth, origDHeight);
        if (NS_FAILED(rv)) {
          ((nsDrawingSurfaceWin *)aSurface)->ReleaseDC();
          return rv;
        }
        didComposite = PR_TRUE;
      }
    } 

    
    if (!didComposite) {
      ::StretchDIBits(TheHDC, aDX, aDY, aDWidth, aDHeight,
                      aSX, srcy, aSWidth, aSHeight, mImageBits,
                      (LPBITMAPINFO)mBHead, 256 == mNumPaletteColors ? 
                      DIB_PAL_COLORS : DIB_RGB_COLORS, rop);
    }
  } else {
    
    DWORD rop = SRCCOPY;

    if (canRaster == DT_RASPRINTER) {
      
      if (mAlphaBits && mAlphaDepth == 1) {
        MONOBITMAPINFO  bmi(mBHead->biWidth, mBHead->biHeight);
        if (mImageBits) {
          CompositeBitsInMemory(TheHDC, aDX, aDY, aDWidth, aDHeight,
                                aSX, aSY, aSWidth, aSHeight, srcy,
                                mAlphaBits, &bmi, mImageBits, mBHead,
                                mNumPaletteColors);
          didComposite = PR_TRUE;  
        } else {
          ConvertDDBtoDIB(); 
          if (mImageBits) {  
            CompositeBitsInMemory(TheHDC, aDX, aDY, aDWidth, aDHeight,
                                  aSX, aSY, aSWidth, aSHeight, srcy,
                                  mAlphaBits,
                                  &bmi, mImageBits, mBHead,
                                  mNumPaletteColors);
            
            delete [] mImageBits;
            mImageBits = nsnull;
            didComposite = PR_TRUE;         
          } else {
            NS_WARNING("Could not composite bits in memory because conversion to DIB failed\n");
          }
        } 
      } 

      if (!didComposite && 
          (GetDeviceCaps(TheHDC, RASTERCAPS) & (RC_BITBLT | RC_STRETCHBLT)))
        PrintDDB(aSurface, aDX, aDY, aDWidth, aDHeight,
                 aSX, srcy, aSWidth, aSHeight, rop);

    } else { 
      

      
      
      nsIDeviceContext    *dx;
      aContext.GetDeviceContext(dx);
      
      nsIDrawingSurface*     ds;
      static_cast<nsDeviceContextWin*>(dx)->GetDrawingSurface(aContext, ds);

      nsDrawingSurfaceWin *srcDS = (nsDrawingSurfaceWin *)ds;
      if (!srcDS) {
        NS_RELEASE(dx);
        ((nsDrawingSurfaceWin *)aSurface)->ReleaseDC();
        return NS_ERROR_FAILURE;
      }      

      HDC                 srcDC;
      srcDS->GetDC(&srcDC);

      
      if (mAlphaBits && mAlphaDepth == 1) {
        MONOBITMAPINFO  bmi(mBHead->biWidth, mBHead->biHeight);
        ::StretchDIBits(TheHDC, aDX, aDY, aDWidth, aDHeight,
                        aSX, srcy, aSWidth, aSHeight, mAlphaBits,
                        (LPBITMAPINFO)&bmi, DIB_RGB_COLORS, SRCAND);
        rop = SRCPAINT;
      }

      
      HBITMAP oldBits = (HBITMAP)::SelectObject(srcDC, mHBitmap);
      if (8 == mAlphaDepth) {
        BLENDFUNCTION blendFunction;
        blendFunction.BlendOp = AC_SRC_OVER;
        blendFunction.BlendFlags = 0;
        blendFunction.SourceConstantAlpha = 255;
        blendFunction.AlphaFormat = 1;  
        gAlphaBlend(TheHDC, aDX, aDY, aDWidth, aDHeight, 
                    srcDC, aSX, aSY, aSWidth, aSHeight, blendFunction);
      } else { 
        ::StretchBlt(TheHDC, aDX, aDY, aDWidth, aDHeight, 
                     srcDC, aSX, aSY, aSWidth, aSHeight, rop);
      }
      ::SelectObject(srcDC, oldBits);
      srcDS->ReleaseDC();
      NS_RELEASE(dx);
    }
  }
  ((nsDrawingSurfaceWin *)aSurface)->ReleaseDC();

  return NS_OK;
}





void nsImageWin::DrawComposited24(unsigned char *aBits,
                                  PRUint8 *aImageRGB, PRUint32 aStrideRGB,
                                  PRUint8 *aImageAlpha, PRUint32 aStrideAlpha,
                                  int aWidth, int aHeight)
{
  PRInt32 targetRowBytes = ((aWidth * 3) + 3) & ~3;
  for (int y = 0; y < aHeight; y++) {
    unsigned char *targetRow = aBits + y * targetRowBytes;
    unsigned char *imageRow = aImageRGB + y * aStrideRGB;
    unsigned char *alphaRow = aImageAlpha + y * aStrideAlpha;

    for (int x = 0; x < aWidth;
         x++, targetRow += 3, imageRow += 3, alphaRow++) {
      unsigned alpha = *alphaRow;
      MOZ_BLEND(targetRow[0], targetRow[0], imageRow[0], alpha);
      MOZ_BLEND(targetRow[1], targetRow[1], imageRow[1], alpha);
      MOZ_BLEND(targetRow[2], targetRow[2], imageRow[2], alpha);
    }
  }
}






nsresult nsImageWin::DrawComposited(HDC TheHDC, int aDX, int aDY,
                                    int aDWidth, int aDHeight,
                                    int aSX, int aSY, int aSWidth, int aSHeight,
                                    int aOrigDWidth, int aOrigDHeight)
{
  HDC memDC = ::CreateCompatibleDC(TheHDC);
  if (!memDC)
    return NS_ERROR_OUT_OF_MEMORY;
  unsigned char *screenBits;

  PRBool scaling = PR_FALSE;
  
  if ((aDWidth != aSWidth) || (aDHeight != aSHeight)) {
    scaling = PR_TRUE;
    aDWidth = aOrigDWidth;
    aDHeight = aOrigDHeight;
    aSWidth = mBHead->biWidth;
    aSHeight = mBHead->biHeight;
  }

  ALPHA24BITMAPINFO bmi(aDWidth, aDHeight);
  HBITMAP tmpBitmap = ::CreateDIBSection(memDC, (LPBITMAPINFO)&bmi, DIB_RGB_COLORS,
                                         (LPVOID *)&screenBits, NULL, 0);
  if (!tmpBitmap) {
    ::DeleteDC(memDC);
    NS_WARNING("nsImageWin::DrawComposited failed to create tmpBitmap\n");
    return NS_ERROR_OUT_OF_MEMORY;
  }
  HBITMAP oldBitmap = (HBITMAP)::SelectObject(memDC, tmpBitmap);
  if (!oldBitmap || oldBitmap == (HBITMAP)GDI_ERROR) {
    ::DeleteObject(tmpBitmap);
    ::DeleteDC(memDC);
    return NS_ERROR_OUT_OF_MEMORY;
  }

  
  BOOL retval = ::BitBlt(memDC, 0, 0, aDWidth, aDHeight,
                         TheHDC, aDX, aDY, SRCCOPY);
  if (!retval || !::GdiFlush()) {
    
    ::SelectObject(memDC, oldBitmap);
    ::DeleteObject(tmpBitmap);
    ::DeleteDC(memDC);
    return NS_ERROR_FAILURE;
  }

  PRUint8 *imageRGB, *imageAlpha;
  PRUint32 strideRGB, strideAlpha;

  if (scaling) {
    
    imageRGB = (PRUint8 *)nsMemory::Alloc(3*aDWidth*aDHeight);
    imageAlpha = (PRUint8 *)nsMemory::Alloc(aDWidth*aDHeight);

    if (!imageRGB || !imageAlpha) {
      if (imageRGB)
        nsMemory::Free(imageRGB);
      if (imageAlpha)
        nsMemory::Free(imageAlpha);
      ::SelectObject(memDC, oldBitmap);
      ::DeleteObject(tmpBitmap);
      ::DeleteDC(memDC);
      return NS_ERROR_FAILURE;
    }

    strideRGB = 3 * aDWidth;
    strideAlpha = aDWidth;
    RectStretch(aSWidth, aSHeight, aDWidth, aDHeight,
                0, 0, aDWidth-1, aDHeight-1,
                mImageBits, mRowBytes, imageRGB, strideRGB, 24);
    RectStretch(aSWidth, aSHeight, aDWidth, aDHeight,
                0, 0, aDWidth-1, aDHeight-1,
                mAlphaBits, mARowBytes, imageAlpha, strideAlpha, 8);
  } else {
    imageRGB = mImageBits + aSY * mRowBytes + aSX * 3;
    imageAlpha = mAlphaBits + aSY * mARowBytes + aSX;
    strideRGB = mRowBytes;
    strideAlpha = mARowBytes;
  }

  
  DrawComposited24(screenBits, imageRGB, strideRGB, imageAlpha, strideAlpha,
                   aDWidth, aDHeight);

  if (scaling) {
    
    nsMemory::Free(imageRGB);
    nsMemory::Free(imageAlpha);
  }

  
  
  if (scaling) {
    
    retval = ::StretchBlt(TheHDC, aDX, aDY,
                          aDWidth, (mDecodedY2*aDHeight + aSHeight - 1)/aSHeight,
                          memDC, 0, 0,
                          aDWidth, (mDecodedY2*aDHeight + aSHeight - 1)/aSHeight,
                          SRCCOPY);
  } else {
    retval = ::StretchBlt(TheHDC, aDX, aDY, aDWidth, aDHeight,
                          memDC, 0, 0, aDWidth, aDHeight, SRCCOPY);
  }

  if (!retval) {
    ::SelectObject(memDC, oldBitmap);
    ::DeleteObject(tmpBitmap);
    ::DeleteDC(memDC);
    return NS_ERROR_FAILURE;
  }

  
  ::SelectObject(memDC, oldBitmap);
  ::DeleteObject(tmpBitmap);
  ::DeleteDC(memDC);
  return NS_OK;
}






NS_IMETHODIMP nsImageWin :: Draw(nsIRenderingContext &aContext, nsIDrawingSurface* aSurface,
         PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight)
{
  return Draw(aContext, aSurface, 0, 0, mBHead->biWidth, mBHead->biHeight, aX, aY, aWidth, aHeight);
}





NS_IMETHODIMP nsImageWin::DrawTile(nsIRenderingContext &aContext,
                                   nsIDrawingSurface* aSurface,
                                   PRInt32 aSXOffset, PRInt32 aSYOffset,
                                   PRInt32 aPadX, PRInt32 aPadY,
                                   const nsRect &aDestRect)
{
  NS_ASSERTION(!aDestRect.IsEmpty(), "DrawTile doesn't work with empty rects");
  if (mDecodedX2 < mDecodedX1 || mDecodedY2 < mDecodedY1)
    return NS_OK;

  float           scale;
  unsigned char   *targetRow,*imageRow,*alphaRow;
  PRInt32         x0, y0, x1, y1, destScaledWidth, destScaledHeight;
  PRInt32         validWidth,validHeight,validX,validY,targetRowBytes;
  PRInt32         x,y,width,height,canRaster;
  nsCOMPtr<nsIDeviceContext> theDeviceContext;
  HDC             theHDC;
  nscoord         ScaledTileWidth,ScaledTileHeight;
  PRBool          padded = (aPadX || aPadY);

  ((nsDrawingSurfaceWin *)aSurface)->GetTECHNOLOGY(&canRaster);
  aContext.GetDeviceContext(*getter_AddRefs(theDeviceContext));

  
  
  if ((canRaster != DT_RASPRINTER) && (256 != mNumPaletteColors) &&
      !(mAlphaDepth == 8 && !mIsOptimized) && !padded)
    if (ProgressiveDoubleBlit(theDeviceContext, aSurface,
                              aSXOffset, aSYOffset, aDestRect))
      return NS_OK;
  theDeviceContext->GetCanonicalPixelScale(scale);

  destScaledWidth  = PR_MAX(PRInt32(mBHead->biWidth*scale), 1);
  destScaledHeight = PR_MAX(PRInt32(mBHead->biHeight*scale), 1);
  
  validX = 0;
  validY = 0;
  validWidth  = mBHead->biWidth;
  validHeight = mBHead->biHeight;
  
  
  
  if (mDecodedY2 < mBHead->biHeight) {
    validHeight = mDecodedY2 - mDecodedY1;
    destScaledHeight = PR_MAX(PRInt32(validHeight*scale), 1);
  }
  if (mDecodedX2 < mBHead->biWidth) {
    validWidth = mDecodedX2 - mDecodedX1;
    destScaledWidth = PR_MAX(PRInt32(validWidth*scale), 1);
  }
  if (mDecodedY1 > 0) {   
    validHeight -= mDecodedY1;
    destScaledHeight = PR_MAX(PRInt32(validHeight*scale), 1);
    validY = mDecodedY1;
  }
  if (mDecodedX1 > 0) {
    validWidth -= mDecodedX1;
    destScaledWidth = PR_MAX(PRInt32(validWidth*scale), 1);
    validX = mDecodedX1; 
  }

  
  y0 = aDestRect.y - aSYOffset;
  x0 = aDestRect.x - aSXOffset;
  y1 = aDestRect.y + aDestRect.height;
  x1 = aDestRect.x + aDestRect.width;


  
  
  ScaledTileWidth = PR_MAX(PRInt32(mBHead->biWidth*scale), 1);
  ScaledTileHeight = PR_MAX(PRInt32(mBHead->biHeight*scale), 1);

  
  if (mAlphaDepth == 8 && !mIsOptimized && !padded) {
    unsigned char *screenBits=nsnull,*adjAlpha,*adjImage,*adjScreen;
    HDC           memDC=nsnull;
    HBITMAP       tmpBitmap=nsnull,oldBitmap;
    unsigned char alpha;
    PRInt32       targetBytesPerPixel,imageBytesPerPixel;

    if (!mImageBits) {
      ConvertDDBtoDIB();
    }

    
    ((nsDrawingSurfaceWin *)aSurface)->GetDC(&theHDC);
    if (theHDC) {
    
      memDC = CreateCompatibleDC(theHDC);
      width = aDestRect.width;
      height = aDestRect.height;

      ALPHA24BITMAPINFO bmi(width, height);
      tmpBitmap = ::CreateDIBSection(memDC, (LPBITMAPINFO)&bmi, DIB_RGB_COLORS, (LPVOID *)&screenBits, NULL, 0);
      oldBitmap = (HBITMAP)::SelectObject(memDC, tmpBitmap);

      
      targetRowBytes = (bmi.bmiHeader.biWidth * bmi.bmiHeader.biBitCount) >> 5;  
      if (((PRUint32)bmi.bmiHeader.biWidth * bmi.bmiHeader.biBitCount) & 0x1F) { 
        targetRowBytes++;     
      }
      targetRowBytes <<= 2;   

      targetBytesPerPixel = bmi.bmiHeader.biBitCount/8;
    }

    if (!tmpBitmap) {
      if (memDC) {
        ::DeleteDC(memDC);
      }
      
      NS_WARNING("The Creation of the tmpBitmap failed \n");
    } else {
      
      
      ::StretchBlt(memDC, 0, 0, width, height,theHDC, aDestRect.x, aDestRect.y, width, height, SRCCOPY);
      ::GdiFlush();
  
      imageBytesPerPixel = mBHead->biBitCount/8;

      
      
      adjScreen = screenBits + ((height-1) * targetRowBytes);
      adjImage = mImageBits + ((validHeight-1) * mRowBytes);
      adjAlpha = mAlphaBits + ((validHeight-1) * mARowBytes);


      for (int y = 0,byw=aSYOffset; y < height; y++,byw++) {

        if (byw >= ScaledTileHeight) {
          byw = 0;
        }

        targetRow = adjScreen - (y * targetRowBytes);
        imageRow = adjImage - (byw * mRowBytes);
        alphaRow = adjAlpha - (byw * mARowBytes);

        
        imageRow += (aSXOffset*imageBytesPerPixel);
        alphaRow += aSXOffset;

        for (int x=0,bxw=aSXOffset;x<width;x++,targetRow+=targetBytesPerPixel,imageRow+=imageBytesPerPixel,bxw++, alphaRow++) {
          
          if (bxw>=ScaledTileWidth) {
            bxw = 0;
            imageRow = adjImage - (byw * mRowBytes);
            alphaRow = adjAlpha - (byw * mARowBytes);
          }

          alpha = *alphaRow;

          MOZ_BLEND(targetRow[0], targetRow[0], imageRow[0], alpha);
          MOZ_BLEND(targetRow[1], targetRow[1], imageRow[1], alpha);
          MOZ_BLEND(targetRow[2], targetRow[2], imageRow[2], alpha);
        }
      }

      
      ::StretchBlt(theHDC, aDestRect.x, aDestRect.y, width, height,memDC, 0, 0, width, height, SRCCOPY);

      ::SelectObject(memDC, oldBitmap);
      ::DeleteObject(tmpBitmap);
      ::DeleteDC(memDC);
  
    return(NS_OK);
    } 
  }

  
  
  for (y=y0;y<y1;y+=ScaledTileHeight+aPadY*scale) {
    for (x=x0;x<x1;x+=ScaledTileWidth+aPadX*scale) {
    Draw(aContext, aSurface,
         0, 0, PR_MIN(validWidth, x1-x), PR_MIN(validHeight, y1-y),
         x, y, PR_MIN(destScaledWidth, x1-x), PR_MIN(destScaledHeight, y1-y));
    }
  } 
  return(NS_OK);
}




PRBool
nsImageWin::ProgressiveDoubleBlit(nsIDeviceContext *aContext,
                                  nsIDrawingSurface* aSurface,
                                  PRInt32 aSXOffset, PRInt32 aSYOffset,
                                  nsRect aDestRect)
{
  













  HDC theHDC;
  void *screenBits; 
                    

  ((nsDrawingSurfaceWin *)aSurface)->GetDC(&theHDC);
  if (!theHDC)
    return PR_FALSE;

  
  HDC imgDC = ::CreateCompatibleDC(theHDC);
  if (!imgDC) {
    ((nsDrawingSurfaceWin *)aSurface)->ReleaseDC();
    return PR_FALSE;
  }

  CreateDDB();

  nsPaletteInfo palInfo;
  aContext->GetPaletteInfo(palInfo);
  if (palInfo.isPaletteDevice && palInfo.palette) {
#ifndef WINCE
    ::SetStretchBltMode(imgDC, HALFTONE);
#endif
    ::SelectPalette(imgDC, (HPALETTE)palInfo.palette, TRUE);
    ::RealizePalette(imgDC);
  }

  
  HDC maskDC = nsnull;
  HBITMAP oldImgMaskBits = nsnull;
  HBITMAP maskBits;
  HBITMAP mTmpHBitmap = nsnull;
  if (mAlphaDepth == 1) {
    maskDC = ::CreateCompatibleDC(theHDC);
    if (!maskDC) {
      ::DeleteDC(imgDC);
      ((nsDrawingSurfaceWin *)aSurface)->ReleaseDC();
      return PR_FALSE;
    }

    MONOBITMAPINFO bmi(mBHead->biWidth, mBHead->biHeight);
    maskBits  = ::CreateDIBSection(theHDC, (LPBITMAPINFO)&bmi,
                                   DIB_RGB_COLORS, &screenBits, NULL, 0);
    if (!maskBits) {
      ::DeleteDC(imgDC);
      ::DeleteDC(maskDC);
      ((nsDrawingSurfaceWin *)aSurface)->ReleaseDC();
      return PR_FALSE;
    }

    oldImgMaskBits = (HBITMAP)::SelectObject(maskDC, maskBits);
    ::SetDIBitsToDevice(maskDC, 0, 0, mBHead->biWidth, mBHead->biHeight,
                        0, 0, 0, mBHead->biHeight, mAlphaBits,
                        (LPBITMAPINFO)&bmi, DIB_RGB_COLORS);
  }


  
  HBITMAP oldImgBits = nsnull;
  if (!mIsOptimized || !mHBitmap) {
    
    if (gPlatform == VER_PLATFORM_WIN32_WINDOWS) {
      int bytesPerPix = ::GetDeviceCaps(imgDC, BITSPIXEL) / 8;
      if (mBHead->biWidth * mBHead->biHeight * bytesPerPix > 0xFF0000) {
        ::DeleteDC(imgDC);
        if (maskDC) {
          if (oldImgMaskBits)
            ::SelectObject(maskDC, oldImgMaskBits);
          ::DeleteObject(maskBits);
          ::DeleteDC(maskDC);
        }
        ((nsDrawingSurfaceWin *)aSurface)->ReleaseDC();
        return PR_FALSE;
      }
    }
    mTmpHBitmap = ::CreateCompatibleBitmap(theHDC, mDecodedX2, mDecodedY2);
    if (!mTmpHBitmap) {
      ::DeleteDC(imgDC);
      if (maskDC) {
        if (oldImgMaskBits)
          ::SelectObject(maskDC, oldImgMaskBits);
        ::DeleteObject(maskBits);
        ::DeleteDC(maskDC);
      }
      ((nsDrawingSurfaceWin *)aSurface)->ReleaseDC();
      return PR_FALSE;
    }
    oldImgBits = (HBITMAP)::SelectObject(imgDC, mTmpHBitmap);
    ::StretchDIBits(imgDC, 0, 0, mBHead->biWidth, mBHead->biHeight,
                    0, 0, mBHead->biWidth, mBHead->biHeight,
                    mImageBits, (LPBITMAPINFO)mBHead,
                    256 == mNumPaletteColors ? DIB_PAL_COLORS : DIB_RGB_COLORS,
                    SRCCOPY);
  } else {
    oldImgBits = (HBITMAP)::SelectObject(imgDC, mHBitmap);
  }

  PRBool result = PR_TRUE;
  PRBool useAlphaBlend = mAlphaDepth == 8 && mIsOptimized;

  PRInt32 firstWidth = mBHead->biWidth - aSXOffset;
  PRInt32 firstHeight = mBHead->biHeight - aSYOffset;

  if (aDestRect.width > firstWidth + mBHead->biWidth ||
      aDestRect.height > firstHeight + mBHead->biHeight) {
    PRInt32 firstPartialWidth = aSXOffset == 0 ? 0 : firstWidth;
    PRInt32 firstPartialHeight = aSYOffset == 0 ? 0 : firstHeight;
    PRBool hasFullImageWidth = aDestRect.width - firstPartialWidth >= mBHead->biWidth;
    PRBool hasFullImageHeight = aDestRect.height - firstPartialHeight >= mBHead->biHeight;

    HDC dstDC = theHDC;               
    HDC dstMaskDC = nsnull;           
    HBITMAP dstMaskHBitmap = nsnull;  
    HBITMAP oldDstBits, oldDstMaskBits;
    HBITMAP dstHBitmap;

    do {  
      nsRect storedDstRect;

      
      
      if (mAlphaDepth != 0) {
        
        
        

        storedDstRect = aDestRect;
        aDestRect.x = 0;
        aDestRect.y = 0;
        
        
        
        if (aDestRect.height - firstPartialHeight >= mBHead->biHeight * 2)
          aDestRect.height = PR_ROUNDUP(PRInt32(ceil(aDestRect.height / 2.0)),
                                        mBHead->biHeight)
                             + firstPartialHeight;

        
        
        
        
        if (gPlatform == VER_PLATFORM_WIN32_WINDOWS) {
          int bytesPerPix = (mAlphaDepth == 8) ? 4 :
                            ::GetDeviceCaps(theHDC, BITSPIXEL) / 8;
          if (aDestRect.width * aDestRect.height * bytesPerPix > 0xFF0000) {
            result = PR_FALSE;
            break;
          }
        }

        
        dstDC = ::CreateCompatibleDC(theHDC);
        if (!dstDC) {
          result = PR_FALSE;
          break;
        }

        
        if (mAlphaDepth == 8) {
          
          
          
          
          ALPHA32BITMAPINFO bmi(aDestRect.width, aDestRect.height);
          dstHBitmap = ::CreateDIBSection(theHDC, (LPBITMAPINFO)&bmi,
                                          DIB_RGB_COLORS, &screenBits, NULL, 0);
        } else {
          
          dstHBitmap = ::CreateCompatibleBitmap(theHDC, aDestRect.width,
                                                aDestRect.height);
          if (dstHBitmap) {
            dstMaskDC = ::CreateCompatibleDC(theHDC);
            if (dstMaskDC) {
              MONOBITMAPINFO bmi(aDestRect.width, aDestRect.height);
              dstMaskHBitmap = ::CreateDIBSection(theHDC, (LPBITMAPINFO)&bmi,
                                                  DIB_RGB_COLORS, &screenBits,
                                                  NULL, 0);
              if (dstMaskHBitmap) {
                oldDstMaskBits = (HBITMAP)::SelectObject(dstMaskDC,
                                                         dstMaskHBitmap);
              } else {
                result = PR_FALSE;
                break;
              }
            } 
          } 
        }
        if (!dstHBitmap) {
          result = PR_FALSE;
          break;
        }

        oldDstBits = (HBITMAP)::SelectObject(dstDC, dstHBitmap);
      } 


      PRInt32 imgX = hasFullImageWidth ? 0 : aSXOffset;
      PRInt32 imgW = PR_MIN(mBHead->biWidth - imgX, aDestRect.width);
      PRInt32 imgY = hasFullImageHeight ? 0 : aSYOffset;
      PRInt32 imgH = PR_MIN(mBHead->biHeight - imgY, aDestRect.height);

      
      
      nsPoint surfaceDestPoint(aDestRect.x, aDestRect.y);
      if (aSXOffset != 0 && hasFullImageWidth)
        surfaceDestPoint.x += firstWidth;
      if (aSYOffset != 0 && hasFullImageHeight)
        surfaceDestPoint.y += firstHeight;

      
      BlitImage(dstDC, dstMaskDC,
                surfaceDestPoint.x, surfaceDestPoint.y, imgW, imgH,
                imgDC, maskDC, imgX, imgY, PR_FALSE);
      if (!hasFullImageHeight && imgH != aDestRect.height) {
        
        
        BlitImage(dstDC, dstMaskDC, surfaceDestPoint.x, surfaceDestPoint.y + imgH,
                          imgW, aDestRect.height - imgH,
                  imgDC, maskDC, imgX, 0, PR_FALSE);
        imgH = aDestRect.height;
      }
      if (!hasFullImageWidth && imgW != aDestRect.width) {
        BlitImage(dstDC, dstMaskDC, surfaceDestPoint.x + imgW, surfaceDestPoint.y,
                          aDestRect.width - imgW, imgH,
                  imgDC, maskDC, 0, imgY, PR_FALSE);
        imgW = aDestRect.width;
      }


      nsRect surfaceDestRect;
      nsRect surfaceSrcRect(surfaceDestPoint.x, surfaceDestPoint.y, imgW, imgH);

      
      if (hasFullImageWidth) {
        while (surfaceSrcRect.XMost() < aDestRect.XMost()) {
          surfaceDestRect.x = surfaceSrcRect.XMost();
          surfaceDestRect.width = surfaceSrcRect.width;
          if (surfaceDestRect.XMost() >= aDestRect.XMost())
            surfaceDestRect.width = aDestRect.XMost() - surfaceDestRect.x;

          BlitImage(dstDC, dstMaskDC, surfaceDestRect.x, surfaceSrcRect.y,
                            surfaceDestRect.width, surfaceSrcRect.height,
                    dstDC, dstMaskDC, surfaceSrcRect.x, surfaceSrcRect.y,
                    PR_FALSE);
          surfaceSrcRect.width += surfaceDestRect.width;
        }
      }

      
      if (hasFullImageHeight) {
        while (surfaceSrcRect.YMost() < aDestRect.YMost()) {
          surfaceDestRect.y = surfaceSrcRect.YMost();
          surfaceDestRect.height = surfaceSrcRect.height;
          if (surfaceDestRect.YMost() >= aDestRect.YMost())
            surfaceDestRect.height = aDestRect.YMost() - surfaceDestRect.y;

          BlitImage(dstDC, dstMaskDC, surfaceSrcRect.x, surfaceDestRect.y,
                            surfaceSrcRect.width, surfaceDestRect.height,
                    dstDC, dstMaskDC, surfaceSrcRect.x, surfaceSrcRect.y,
                    PR_FALSE);
          surfaceSrcRect.height += surfaceDestRect.height;
        }
      }

      
      if (surfaceDestPoint.y != aDestRect.y && surfaceDestPoint.x != aDestRect.x)
          BlitImage(dstDC, dstMaskDC, aDestRect.x, aDestRect.y,
                    firstWidth, firstHeight,
                    dstDC, dstMaskDC,
                    surfaceDestPoint.x + aSXOffset, surfaceDestPoint.y + aSYOffset,
                    PR_FALSE);
      
      if (surfaceDestPoint.y != aDestRect.y)
          BlitImage(dstDC, dstMaskDC, surfaceDestPoint.x, aDestRect.y,
                    surfaceSrcRect.width, firstHeight,
                    dstDC, dstMaskDC, surfaceDestPoint.x, surfaceDestPoint.y + aSYOffset,
                    PR_FALSE);

      
      if (surfaceDestPoint.x != aDestRect.x)
        BlitImage(dstDC, dstMaskDC, aDestRect.x, surfaceDestPoint.y,
                  firstWidth, surfaceSrcRect.height,
                  dstDC, dstMaskDC, surfaceDestPoint.x + aSXOffset, surfaceDestPoint.y,
                  PR_FALSE);

      if (mAlphaDepth != 0) {
        
        BlitImage(theHDC, theHDC, storedDstRect.x, storedDstRect.y,
                  aDestRect.width, aDestRect.height,
                  dstDC, dstMaskDC, 0, 0, useAlphaBlend);
        
        if (storedDstRect.height > aDestRect.height)
          BlitImage(theHDC, theHDC, storedDstRect.x, storedDstRect.y + aDestRect.height,
                    aDestRect.width, storedDstRect.height - aDestRect.height,
                    dstDC, dstMaskDC, 0, firstPartialHeight, useAlphaBlend);
      }
    } while (PR_FALSE);

    if (mAlphaDepth != 0) {
      if (dstDC) {
        if (dstHBitmap) {
          ::SelectObject(dstDC, oldDstBits);
          ::DeleteObject(dstHBitmap);
        }
        ::DeleteDC(dstDC);
      }
      if (dstMaskDC) {
        if (dstMaskHBitmap) {
          ::SelectObject(dstMaskDC, oldDstMaskBits);
          ::DeleteObject(dstMaskHBitmap);
        }
        ::DeleteDC(dstMaskDC);
      }
    }
  } else {
    
    
    BlitImage(theHDC, theHDC,
              aDestRect.x, aDestRect.y, firstWidth, firstHeight,
              imgDC, maskDC, aSXOffset, aSYOffset, useAlphaBlend);

    
    if (aDestRect.width - firstWidth > 0 && aDestRect.height - firstHeight > 0)
      BlitImage(theHDC, theHDC,
                aDestRect.x + firstWidth, aDestRect.y + firstHeight,
                aDestRect.width - firstWidth, aDestRect.height - firstHeight,
                imgDC, maskDC, 0, 0, useAlphaBlend);

    
    if (aDestRect.height - firstHeight > 0)
      BlitImage(theHDC, theHDC, aDestRect.x, aDestRect.y + firstHeight,
                firstWidth, aDestRect.height - firstHeight,
                imgDC, maskDC, aSXOffset, 0, useAlphaBlend);

    
    if (aDestRect.width - firstWidth > 0)
      BlitImage(theHDC, theHDC, aDestRect.x + firstWidth, aDestRect.y,
                aDestRect.width - firstWidth, firstHeight,
                imgDC, maskDC, 0, aSYOffset, useAlphaBlend);
  }

  if (oldImgBits)
    ::SelectObject(imgDC, oldImgBits);
  if (mTmpHBitmap)
    ::DeleteObject(mTmpHBitmap);
  ::DeleteDC(imgDC);
  if (maskDC) {
    if (oldImgMaskBits)
      ::SelectObject(maskDC, oldImgMaskBits);
    ::DeleteObject(maskBits);
    ::DeleteDC(maskDC);
  }
  ((nsDrawingSurfaceWin *)aSurface)->ReleaseDC();

  return result;
}

void
nsImageWin::BlitImage(HDC aDstDC, HDC aDstMaskDC, PRInt32 aDstX, PRInt32 aDstY,
                      PRInt32 aWidth, PRInt32 aHeight,
                      HDC aSrcDC, HDC aSrcMaskDC, PRInt32 aSrcX, PRInt32 aSrcY,
                      PRBool aUseAlphaBlend)
{
  if (aUseAlphaBlend) {
    BLENDFUNCTION blendFunction;
    blendFunction.BlendOp = AC_SRC_OVER;
    blendFunction.BlendFlags = 0;
    blendFunction.SourceConstantAlpha = 255;
    blendFunction.AlphaFormat = 1;
    gAlphaBlend(aDstDC, aDstX, aDstY, aWidth, aHeight,
                aSrcDC, aSrcX, aSrcY, aWidth, aHeight, blendFunction);
  } else {
    if (aSrcMaskDC) {
      if (aDstMaskDC == aDstDC) {
        ::BitBlt(aDstDC, aDstX, aDstY, aWidth, aHeight,
                 aSrcMaskDC, aSrcX, aSrcY, SRCAND);
        ::BitBlt(aDstDC, aDstX, aDstY, aWidth, aHeight,
                 aSrcDC, aSrcX, aSrcY, SRCPAINT);
      } else {
        ::BitBlt(aDstMaskDC, aDstX, aDstY, aWidth, aHeight,
                 aSrcMaskDC, aSrcX, aSrcY, SRCCOPY);
        ::BitBlt(aDstDC, aDstX, aDstY, aWidth, aHeight,
                  aSrcDC, aSrcX, aSrcY, SRCCOPY);
      }
    } else {
      ::BitBlt(aDstDC, aDstX, aDstY, aWidth, aHeight,
               aSrcDC, aSrcX, aSrcY, SRCCOPY);
    }
  }
}


ALPHABLENDPROC nsImageWin::gAlphaBlend = NULL;


PRBool nsImageWin::CanAlphaBlend(void)
{
#ifdef WINCE
  gAlphaBlend = nsnull;
  return PR_FALSE;
#else
  static PRBool alreadyChecked = PR_FALSE;

  if (!alreadyChecked) {
    OSVERSIONINFO os;
    
    os.dwOSVersionInfoSize = sizeof(os);
    ::GetVersionEx(&os);
    
    
    if (VER_PLATFORM_WIN32_WINDOWS != os.dwPlatformId ||
        os.dwMajorVersion != 4 || os.dwMinorVersion != 10) {
      gAlphaBlend = (ALPHABLENDPROC)::GetProcAddress(::LoadLibrary("msimg32"),
              "AlphaBlend");
    }
    alreadyChecked = PR_TRUE;
  }

  return gAlphaBlend != NULL;
#endif
}







nsresult nsImageWin::Optimize(nsIDeviceContext* aContext)
{
  
  
  mWantsOptimization = PR_TRUE;
  return NS_OK;
}


NS_IMETHODIMP nsImageWin::CreateDDB()
{
  if (!mWantsOptimization || mIsOptimized) {
    
    if (mTimer)
      mTimer->SetDelay(GFX_MS_REMOVE_DBB);

    return NS_OK;
  }

  
  

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  if ((gOsMajorVersion <= VER_OSMAJOR_WIN9598MENT &&
       (mSizeImage >= 0xFF0000 || mSizeImage < 0x20000)) ||
      (mAlphaDepth == 8 && !CanAlphaBlend())) {
    return NS_OK;
  }

  HDC TheHDC = ::CreateCompatibleDC(NULL);
  
  if (TheHDC != NULL){
    
    
    
    int rasterCaps = ::GetDeviceCaps(TheHDC, RASTERCAPS);
    if (RC_PALETTE == (rasterCaps & RC_PALETTE)) {
      ::DeleteDC(TheHDC);
      return NS_OK;
    }
  
    
    int planes = ::GetDeviceCaps(TheHDC, PLANES);
    int bpp = ::GetDeviceCaps(TheHDC, BITSPIXEL);

    HBITMAP  tBitmap = ::CreateBitmap(1,1,planes,bpp,NULL);
    HBITMAP oldbits = (HBITMAP)::SelectObject(TheHDC,tBitmap);

    if (mAlphaDepth == 8) {
      CreateImageWithAlphaBits(TheHDC);
    } else {
      
      
      
      
      
      if (gOsMajorVersion <= VER_OSMAJOR_WIN9598MENT) {
        LPVOID bits;
        mHBitmap = ::CreateDIBSection(TheHDC, (LPBITMAPINFO)mBHead,
          256 == mNumPaletteColors ? DIB_PAL_COLORS : DIB_RGB_COLORS,
          &bits, NULL, 0);

        if (mHBitmap) {
          memcpy(bits, mImageBits, mSizeImage);
          mIsOptimized = PR_TRUE;
        } else {
          mIsOptimized = PR_FALSE;
        }
      } else {
        mHBitmap = ::CreateDIBitmap(TheHDC, mBHead, CBM_INIT, mImageBits,
                                    (LPBITMAPINFO)mBHead,
                                    256 == mNumPaletteColors ? DIB_PAL_COLORS
                                                             : DIB_RGB_COLORS);
        mIsOptimized = (mHBitmap != 0);
      }
    }
    if (mIsOptimized) {
      ::GdiFlush();
      CleanUpDIB();

      if (mTimer) {
        mTimer->SetDelay(GFX_MS_REMOVE_DBB);
      } else {
        mTimer = do_CreateInstance("@mozilla.org/timer;1");
        if (mTimer)
          mTimer->InitWithFuncCallback(nsImageWin::TimerCallBack, this,
                                       GFX_MS_REMOVE_DBB,
                                       nsITimer::TYPE_ONE_SHOT);
      }
    }
    ::SelectObject(TheHDC,oldbits);
    ::DeleteObject(tBitmap);
    ::DeleteDC(TheHDC);
  }

  return NS_OK;
}


NS_IMETHODIMP nsImageWin::RemoveDDB()
{
  if (!mIsOptimized && mHBitmap == nsnull)
    return NS_OK;

  if (!mImageBits) {
    nsresult rv = ConvertDDBtoDIB();
    if (NS_FAILED(rv))
      return rv;
  }

  CleanUpDDB();
  return NS_OK;
}








PRInt32  
nsImageWin :: CalcBytesSpan(PRUint32  aWidth)
{
  PRInt32 spanBytes;


  spanBytes = (aWidth * mBHead->biBitCount) >> 5;


  if (((PRUint32)mBHead->biWidth * mBHead->biBitCount) & 0x1F) 
    spanBytes++;


  spanBytes <<= 2;


  return(spanBytes);
}






void
nsImageWin::CleanUpDIB()
{
  if (mImageBits != nsnull) {
    delete [] mImageBits;
    mImageBits = nsnull;
  }


  
  if (mColorMap != nsnull){
    delete [] mColorMap->Index;
    delete mColorMap;
    mColorMap = nsnull;
  }
}





void 
nsImageWin :: CleanUpDDB()
{
  if (mHBitmap != nsnull) {
    if (mTimer) {
      mTimer->Cancel();
      mTimer = nsnull;
    }

    ::DeleteObject(mHBitmap);
    mHBitmap = nsnull;
  }
  mIsOptimized = PR_FALSE;
}







nsresult 
nsImageWin::PrintDDB(nsIDrawingSurface* aSurface,
                     PRInt32 aDX, PRInt32 aDY, PRInt32 aDWidth, PRInt32 aDHeight,
                     PRInt32 aSX, PRInt32 aSY, PRInt32 aSWidth, PRInt32 aSHeight,
                     PRUint32 aROP)
{
  HDC   theHDC;
  UINT  palType;


  if (mIsOptimized == PR_TRUE){
    if (mHBitmap != nsnull){
      ConvertDDBtoDIB();
      ((nsDrawingSurfaceWin *)aSurface)->GetDC(&theHDC);


      if (mBHead->biBitCount == 8) {
        palType = DIB_PAL_COLORS;
      } else {
        palType = DIB_RGB_COLORS;
      }


      ::StretchDIBits(theHDC, aDX, aDY, aDWidth, aDHeight,
                      aSX, aSY, aSWidth, aSHeight, mImageBits,
                      (LPBITMAPINFO)mBHead, palType, aROP);


      
      if (mImageBits != nsnull) {
        delete [] mImageBits;
        mImageBits = nsnull;
      }
    }
  }


  return NS_OK;
}








nsresult nsImageWin::ConvertDDBtoDIB()
{
  HDC                 memPrDC;

  if (mImageBits)
    return NS_OK;

  if (!mInitialized || mHBitmap == nsnull)
    return NS_ERROR_FAILURE;

  memPrDC = ::CreateDC("DISPLAY", NULL, NULL, NULL);
  if (!memPrDC)
    return NS_ERROR_FAILURE;

  
  mImageBits = new unsigned char[mSizeImage];
  if (!mImageBits) {
    ::DeleteDC(memPrDC);
    return NS_ERROR_OUT_OF_MEMORY;
  }

  PRInt32 retVal = 
    ::GetDIBits(memPrDC, mHBitmap, 0, mBHead->biHeight,
                mImageBits, (LPBITMAPINFO)mBHead,
                256 == mNumPaletteColors ? DIB_PAL_COLORS : DIB_RGB_COLORS);

  ::GdiFlush();
  ::DeleteDC(memPrDC);

  if (retVal == 0) {
    delete [] mImageBits;
    mImageBits = nsnull;
    return NS_ERROR_FAILURE;
  }
  
  
  if (mAlphaDepth == 8)
    mImagePreMultiplied = PR_TRUE;

  return NS_OK;
}





NS_IMETHODIMP
nsImageWin::LockImagePixels(PRBool aMaskPixels)
{
  



  mIsLocked = PR_TRUE;


  return NS_OK;
}





NS_IMETHODIMP
nsImageWin::UnlockImagePixels(PRBool aMaskPixels)
{
  mIsLocked = PR_FALSE;
  
  if (mDIBTemp == PR_TRUE) {
    
    if (mImageBits != nsnull) {
      delete [] mImageBits;
      mImageBits = nsnull;
      }


    mDIBTemp = PR_FALSE;
  }


  









  return NS_OK;
}











PRUint8*
nsImageWin::GetBits()
{
  
  if (!mImageBits) {
    ConvertDDBtoDIB();
    mDIBTemp = PR_TRUE;   
  }


  return mImageBits;


} 












NS_IMETHODIMP nsImageWin::DrawToImage(nsIImage* aDstImage, nscoord aDX, nscoord aDY, nscoord aDWidth, nscoord aDHeight)
{
  NS_ASSERTION(mAlphaDepth <= 1, "nsImageWin::DrawToImage can only handle 0 & 1 bit Alpha");

  if (mAlphaDepth > 1)
    return NS_ERROR_UNEXPECTED;

  nsImageWin *dest = static_cast<nsImageWin *>(aDstImage);

  if (!dest)
    return NS_ERROR_FAILURE;

  if (aDX >= dest->mBHead->biWidth || aDY >= dest->mBHead->biHeight)
    return NS_OK;

  if (!dest->mImageBits)
    return NS_ERROR_FAILURE;
     
  if (!dest->mIsOptimized) {
    
    PRUint8  *rgbPtr = 0, *alphaPtr = 0;
    PRUint32 rgbStride, alphaStride;
    PRInt32  srcHeight;
    PRUint8  *dstRgbPtr = 0, *dstAlphaPtr = 0;
    PRUint32 dstRgbStride, dstAlphaStride;
    PRInt32  dstHeight;

    rgbPtr = mImageBits;
    rgbStride = mRowBytes;
    alphaPtr = mAlphaBits;
    alphaStride = mARowBytes;
    srcHeight = mBHead->biHeight;

    dstRgbPtr = dest->mImageBits;
    dstRgbStride = dest->mRowBytes;
    dstAlphaPtr = dest->mAlphaBits;
    dstAlphaStride = dest->mARowBytes;
    dstHeight = dest->mBHead->biHeight;


    PRInt32 y;
    PRInt32 ValidWidth = (aDWidth < (dest->mBHead->biWidth - aDX)) ?
                         aDWidth : (dest->mBHead->biWidth - aDX);
    PRInt32 ValidHeight = (aDHeight < (dstHeight - aDY)) ?
                          aDHeight : (dstHeight - aDY);
    PRUint8 *dst;
    PRUint8 *src;

    
    switch (mAlphaDepth) {
    case 1:
      {
        PRUint8 *dstAlpha;
        PRUint8 *alpha;
        PRUint8 offset = aDX & 0x7; 


        for (y=0; y<ValidHeight; y++) {
          dst = dstRgbPtr +
                (dstHeight - (aDY+y) - 1) * dstRgbStride +
                (3 * aDX);
          dstAlpha = dstAlphaPtr + (dstHeight - (aDY+y) - 1) * dstAlphaStride;
          src = rgbPtr + (srcHeight - y - 1)*rgbStride;
          alpha = alphaPtr + (srcHeight - y - 1)*alphaStride;
          for (int x = 0;
               x < ValidWidth;
               x += 8, dst +=  3 * 8, src +=  3 * 8) {
            PRUint8 alphaPixels = *alpha++;
            if (alphaPixels == 0) {
              
              continue;
            }


            
            
            if (x+7 >= ValidWidth) {
              alphaPixels &= 0xff << (8 - (ValidWidth-x)); 
              if (alphaPixels == 0)
                continue;  
            }
            if (offset == 0) {
              dstAlpha[(aDX+x)>>3] |= alphaPixels; 
            } else {
              dstAlpha[(aDX+x)>>3]       |= alphaPixels >> offset;
              
              PRUint8 alphaTemp = alphaPixels << (8U - offset);
              if (alphaTemp & 0xff)
                dstAlpha[((aDX+x)>>3) + 1] |= alphaTemp;
            }


            if (alphaPixels == 0xff) {
              
              
              memcpy(dst,src,8*3);
              continue;
            } else {
              
              
              PRUint8 *d = dst, *s = src;
              for (PRUint8 aMask = 1<<7, j = 0; aMask && j < ValidWidth-x; aMask >>= 1, j++) {
                
                if (alphaPixels & aMask) {
                  
                  d[0] = s[0];
                  d[1] = s[1];
                  d[2] = s[2];
                  
                }
                d += 3;
                s += 3;
              }
            }
          }
        }
      }
      break;
    case 0:
    default:
      dst = dstRgbPtr + (dstHeight - aDY - 1) * dstRgbStride + 3 * aDX;
      src = rgbPtr + (srcHeight - 1) * rgbStride;

      for (y = 0; y < ValidHeight; y++) {
        memcpy(dst, src,  3 * ValidWidth);
        dst -= dstRgbStride;
        src -= rgbStride;
      }
    }
    nsRect rect(aDX, aDY, ValidWidth, ValidHeight);
    dest->ImageUpdated(nsnull, 0, &rect);
  } else {
    
    if (!dest->mHBitmap)
      return NS_ERROR_UNEXPECTED;
      
    HDC dstMemDC = ::CreateCompatibleDC(nsnull);
    HBITMAP oldDstBits;
    DWORD rop;

    oldDstBits = (HBITMAP)::SelectObject(dstMemDC, dest->mHBitmap);
    rop = SRCCOPY;

    if (mAlphaBits) {
      if (1==mAlphaDepth) {
        MONOBITMAPINFO  bmi(mBHead->biWidth, mBHead->biHeight);

        ::StretchDIBits(dstMemDC, aDX, aDY, aDWidth, aDHeight,
                        0, 0,mBHead->biWidth, mBHead->biHeight, mAlphaBits,
                        (LPBITMAPINFO)&bmi, DIB_RGB_COLORS, SRCAND);
        rop = SRCPAINT;
      }
    }

    if (8 == mAlphaDepth) {
      nsresult rv = DrawComposited(dstMemDC, aDX, aDY, aDWidth, aDHeight,
                                   0, 0, mBHead->biWidth, mBHead->biHeight,
                                   aDWidth, aDHeight);
      if (NS_FAILED(rv)) {
        ::SelectObject(dstMemDC, oldDstBits);
        ::DeleteDC(dstMemDC);
        return rv;
      }
    } 
    
    
    if (mIsOptimized && mHBitmap) {
      
      HDC srcMemDC = ::CreateCompatibleDC(nsnull);
      HBITMAP oldSrcBits;
      oldSrcBits = (HBITMAP)::SelectObject(srcMemDC, mHBitmap);
       
      ::StretchBlt(dstMemDC, aDX, aDY, aDWidth, aDHeight, srcMemDC, 
                   0, 0, mBHead->biWidth, mBHead->biHeight, rop);
      
      ::SelectObject(srcMemDC, oldSrcBits);
      ::DeleteDC(srcMemDC);
    } else {
      ::StretchDIBits(dstMemDC, aDX, aDY, aDWidth, aDHeight, 
                      0, 0, mBHead->biWidth, mBHead->biHeight, mImageBits,
                      (LPBITMAPINFO)mBHead, DIB_RGB_COLORS, rop);
    }
    ::SelectObject(dstMemDC, oldDstBits);
    ::DeleteDC(dstMemDC);
  }


  return NS_OK;
}





void 
CompositeBitsInMemory(HDC aTheHDC, int aDX, int aDY, int aDWidth, int aDHeight,
                      int aSX, int aSY, int aSWidth, int aSHeight,PRInt32 aSrcy,
                      PRUint8 *aAlphaBits, MONOBITMAPINFO *aBMI,
                      PRUint8* aImageBits, LPBITMAPINFOHEADER aBHead,
                      PRInt16 aNumPaletteColors)
{
  unsigned char *screenBits;

  HDC memDC = ::CreateCompatibleDC(NULL);

  if(0!=memDC){
    ALPHA24BITMAPINFO offbmi(aSWidth, aSHeight);
    HBITMAP tmpBitmap = ::CreateDIBSection(memDC, (LPBITMAPINFO)&offbmi, DIB_RGB_COLORS,
                                           (LPVOID *)&screenBits, NULL, 0);

    if(0 != tmpBitmap){
      HBITMAP oldBitmap = (HBITMAP)::SelectObject(memDC, tmpBitmap);

      if(0!=oldBitmap) {
        
        ::StretchDIBits(memDC, 0, 0, aSWidth, aSHeight,
                        aSX, aSrcy, aSWidth, aSHeight,
                        aAlphaBits, (LPBITMAPINFO)aBMI,
                        DIB_RGB_COLORS, SRCCOPY); 

        
        ::StretchDIBits(memDC, 0, 0, aSWidth, aSHeight,
                        aSX, aSrcy, aSWidth, aSHeight,
                        aImageBits, (LPBITMAPINFO)aBHead,
                        256 == aNumPaletteColors ? DIB_PAL_COLORS : DIB_RGB_COLORS,
                        SRCPAINT);

        ::GdiFlush();

        
#ifdef _MSC_VER
        __try {
#endif
           ::StretchDIBits(aTheHDC, aDX, aDY, aDWidth, aDHeight,
                          aSX, aSrcy, aSWidth, aSHeight,
                          screenBits, (LPBITMAPINFO)&offbmi,
                          256 == aNumPaletteColors ? DIB_PAL_COLORS : DIB_RGB_COLORS,
                          SRCCOPY);
#ifdef _MSC_VER
        }  __except (EXCEPTION_EXECUTE_HANDLER) {
          
            
          ::StretchDIBits(aTheHDC, aDX, aDY, aDWidth, aDHeight,
                          aSX, aSrcy-1, aSWidth, aSHeight,
                          screenBits, (LPBITMAPINFO)&offbmi,
                          256 == aNumPaletteColors ? DIB_PAL_COLORS : DIB_RGB_COLORS,
                          SRCCOPY);
        }
#endif

        ::SelectObject(memDC, oldBitmap);
      }
      ::DeleteObject(tmpBitmap);
    }
    ::DeleteDC(memDC);
  }
}

void nsImageWin::TimerCallBack(nsITimer *aTimer, void *aClosure)
{
  nsImageWin *entry = static_cast<nsImageWin*>(aClosure);
  if (!entry)
    return;

  if (NS_FAILED(entry->RemoveDDB())) {
    
    
    entry->mTimer = do_CreateInstance("@mozilla.org/timer;1");
    if (entry->mTimer)
      entry->mTimer->InitWithFuncCallback(nsImageWin::TimerCallBack, entry,
                                          GFX_MS_REMOVE_DBB,
                                          nsITimer::TYPE_ONE_SHOT);
  }
}
