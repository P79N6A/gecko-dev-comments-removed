




































#include "nsImageMac.h"
#include "nsRenderingContextMac.h"
#include "nsDeviceContextMac.h"
#include "nsRegionPool.h"
#include "prmem.h"


#define BITS_PER_COMPONENT  8

#define COMPS_PER_PIXEL     4

#define BITS_PER_PIXEL      BITS_PER_COMPONENT * COMPS_PER_PIXEL



#define USE_CGPATTERN_TILING

#if 0


static OSStatus PrintRgnRectProc(UInt16 message, RgnHandle rgn, const Rect *inRect, void *refCon)
{
  UInt32*   rectCount = (UInt32*)refCon;
  
  switch (message)
  {
    case kQDRegionToRectsMsgInit:
      printf("Dumping region 0x%X\n", rgn);
      break;
      
    case kQDRegionToRectsMsgParse:
      printf("Rect %d t,l,r,b: %ld, %ld, %ld, %ld\n", *rectCount, inRect->top, inRect->left, inRect->right, inRect->bottom);
      (*rectCount)++;
      break;
      
    case kQDRegionToRectsMsgTerminate:
      printf("\n");
      break;
  }
  
  return noErr;
}

static void PrintRegionOutline(RgnHandle inRgn)
{
  static RegionToRectsUPP sCountRectProc = nsnull;
  if (!sCountRectProc)
    sCountRectProc = NewRegionToRectsUPP(PrintRgnRectProc);
  
  UInt32    rectCount = 0;  
  ::QDRegionToRects(inRgn, kQDParseRegionFromTopLeft, sCountRectProc, &rectCount);
}
#endif

#pragma mark -




nsImageMac::nsImageMac()
: mImageBits(nsnull)
, mImage(nsnull)
, mWidth(0)
, mHeight(0)
, mRowBytes(0)
, mBytesPerPixel(0)   
, mAlphaBits(nsnull)
, mAlphaRowBytes(0)
, mAlphaDepth(0)
, mPendingUpdate(PR_FALSE)
, mOptimized(PR_FALSE)
, mDecodedX1(PR_INT32_MAX)
, mDecodedY1(PR_INT32_MAX)
, mDecodedX2(0)
, mDecodedY2(0)
{
}


nsImageMac::~nsImageMac()
{
  if (mImage)
    ::CGImageRelease(mImage);

  if (mImageBits)
    PR_Free(mImageBits);

  if (mAlphaBits)
    PR_Free(mAlphaBits);
}


NS_IMPL_ISUPPORTS2(nsImageMac, nsIImage, nsIImageMac)


nsresult
nsImageMac::Init(PRInt32 aWidth, PRInt32 aHeight, PRInt32 aDepth,
                 nsMaskRequirements aMaskRequirements)
{
  

  mWidth = aWidth;
  mHeight = aHeight;

  switch (aMaskRequirements)
  {
    case nsMaskRequirements_kNeeds1Bit:
      mAlphaDepth = 1;
      break;

    case nsMaskRequirements_kNeeds8Bit:
      mAlphaDepth = 8;
      break;

    case nsMaskRequirements_kNoMask:
    default:
      break; 
  }

  
  
  mRowBytes = CalculateRowBytes(aWidth, aDepth == 24 ? 32 : aDepth);
  mImageBits = (PRUint8*) PR_Malloc(mHeight * mRowBytes * sizeof(PRUint8));
  if (!mImageBits)
    return NS_ERROR_OUT_OF_MEMORY;

  if (mAlphaDepth)
  {
    mAlphaRowBytes = CalculateRowBytes(aWidth, mAlphaDepth);
    mAlphaBits = (PRUint8*) PR_Malloc(mHeight * mAlphaRowBytes * sizeof(PRUint8));
    if (!mAlphaBits) {
      PR_Free(mImageBits);
      mImageBits = nsnull;
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  return NS_OK;
}






void
nsImageMac::ImageUpdated(nsIDeviceContext *aContext, PRUint8 aFlags,
                         nsRect *aUpdateRect)
{
  mPendingUpdate = PR_TRUE;

  mDecodedX1 = PR_MIN(mDecodedX1, aUpdateRect->x);
  mDecodedY1 = PR_MIN(mDecodedY1, aUpdateRect->y);

  if (aUpdateRect->YMost() > mDecodedY2)
    mDecodedY2 = aUpdateRect->YMost();

  if (aUpdateRect->XMost() > mDecodedX2)
    mDecodedX2 = aUpdateRect->XMost();
}




PRBool nsImageMac::GetIsImageComplete() {
  return mDecodedX1 == 0 &&
         mDecodedY1 == 0 &&
         mDecodedX2 == mWidth &&
         mDecodedY2 == mHeight;
}

void DataProviderReleaseFunc(void *info, const void *data, size_t size)
{
  PR_Free(const_cast<void*>(data));
}





nsresult
nsImageMac::EnsureCachedImage()
{
  
  if (!mPendingUpdate)
    return NS_OK;

  if (mImage) {
    ::CGImageRelease(mImage);
    mImage = NULL;
  }

  PRUint8* imageData = NULL;  
  CGImageAlphaInfo alphaInfo; 
  void (*releaseFunc)(void *info, const void *data, size_t size) = NULL;

  switch (mAlphaDepth)
  {
    case 8:
    {
      
      
      
      imageData = (PRUint8*) PR_Malloc(mWidth * mHeight * COMPS_PER_PIXEL);
      if (!imageData)
        return NS_ERROR_OUT_OF_MEMORY;
      PRUint8* tmp = imageData;

      for (PRInt32 y = 0; y < mHeight; y++)
      {
        PRInt32 rowStart = mRowBytes * y;
        PRInt32 alphaRowStart = mAlphaRowBytes * y;
        for (PRInt32 x = 0; x < mWidth; x++)
        {
          
          
          PRUint8 alpha = mAlphaBits[alphaRowStart + x];
          *tmp++ = alpha;
          PRUint32 offset = rowStart + COMPS_PER_PIXEL * x;
          FAST_DIVIDE_BY_255(*tmp++, mImageBits[offset + 1] * alpha);
          FAST_DIVIDE_BY_255(*tmp++, mImageBits[offset + 2] * alpha);
          FAST_DIVIDE_BY_255(*tmp++, mImageBits[offset + 3] * alpha);
        }
      }

      
      
      
      
      releaseFunc = DataProviderReleaseFunc;
      alphaInfo = kCGImageAlphaPremultipliedFirst;
      break;
    }

    case 1:
    {
      for (PRInt32 y = 0; y < mHeight; y++)
      {
        PRInt32 rowStart = mRowBytes * y;
        PRUint8* alphaRow = mAlphaBits + mAlphaRowBytes * y;
        for (PRInt32 x = 0; x < mWidth; x++)
        {
          
          mImageBits[rowStart + COMPS_PER_PIXEL * x] =
                                            GetAlphaBit(alphaRow, x) ? 255 : 0;
        }
      }

      alphaInfo = kCGImageAlphaPremultipliedFirst;
      imageData = mImageBits;
      break;
    }

    case 0:
    default:
    {
      alphaInfo = kCGImageAlphaNoneSkipFirst;
      imageData = mImageBits;
      break;
    }
  }

  CGColorSpaceRef cs = ::CGColorSpaceCreateDeviceRGB();
  StColorSpaceReleaser csReleaser(cs);
  CGDataProviderRef prov = ::CGDataProviderCreateWithData(NULL, imageData,
                                                          mRowBytes * mHeight,
                                                          releaseFunc);
  mImage = ::CGImageCreate(mWidth, mHeight, BITS_PER_COMPONENT, BITS_PER_PIXEL,
                           mRowBytes, cs, alphaInfo, prov, NULL, TRUE,
                           kCGRenderingIntentDefault);
  ::CGDataProviderRelease(prov);

  mPendingUpdate = PR_FALSE;
  return NS_OK;
}


NS_IMETHODIMP
nsImageMac::Draw(nsIRenderingContext &aContext, nsIDrawingSurface* aSurface,
                 PRInt32 aSX, PRInt32 aSY, PRInt32 aSWidth, PRInt32 aSHeight,
                 PRInt32 aDX, PRInt32 aDY, PRInt32 aDWidth, PRInt32 aDHeight)
{
  if (mDecodedX2 < mDecodedX1 || mDecodedY2 < mDecodedY1)
    return NS_OK;

  nsresult rv = EnsureCachedImage();
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 srcWidth = aSWidth;
  PRInt32 srcHeight = aSHeight;
  PRInt32 dstWidth = aDWidth;
  PRInt32 dstHeight = aDHeight;

  
  
  

  PRInt32 j = aSX + aSWidth;
  PRInt32 z;
  if (j > mDecodedX2) {
    z = j - mDecodedX2;
    aDWidth -= z*dstWidth/srcWidth;
    aSWidth -= z;
  }
  if (aSX < mDecodedX1) {
    aDX += (mDecodedX1 - aSX)*dstWidth/srcWidth;
    aSX = mDecodedX1;
  }

  j = aSY + aSHeight;
  if (j > mDecodedY2) {
    z = j - mDecodedY2;
    aDHeight -= z*dstHeight/srcHeight;
    aSHeight -= z;
  }
  if (aSY < mDecodedY1) {
    aDY += (mDecodedY1 - aSY)*dstHeight/srcHeight;
    aSY = mDecodedY1;
  }

  if (aDWidth <= 0 || aDHeight <= 0 || aSWidth <= 0 || aSHeight <= 0)
    return NS_OK;

  nsDrawingSurfaceMac* surface = static_cast<nsDrawingSurfaceMac*>(aSurface);
  CGContextRef context = surface->StartQuartzDrawing();

  CGRect srcRect = ::CGRectMake(aSX, aSY, aSWidth, aSHeight);
  CGRect destRect = ::CGRectMake(aDX, aDY, aDWidth, aDHeight);

  CGRect drawRect = ::CGRectMake(0, 0, mWidth, mHeight);
  if (!::CGRectEqualToRect(srcRect, destRect))
  {
    
    float sx = ::CGRectGetWidth(destRect) / ::CGRectGetWidth(srcRect);
    float sy = ::CGRectGetHeight(destRect) / ::CGRectGetHeight(srcRect);
    float dx = ::CGRectGetMinX(destRect) - (::CGRectGetMinX(srcRect) * sx);
    float dy = ::CGRectGetMinY(destRect) - (::CGRectGetMinY(srcRect) * sy);
    drawRect = ::CGRectMake(dx, dy, mWidth * sx, mHeight * sy);
  }

  ::CGContextClipToRect(context, destRect);
  ::CGContextDrawImage(context, drawRect, mImage);
  surface->EndQuartzDrawing(context);

  return NS_OK;
}


NS_IMETHODIMP
nsImageMac::Draw(nsIRenderingContext &aContext, 
                 nsIDrawingSurface* aSurface,
                 PRInt32 aX, PRInt32 aY, 
                 PRInt32 aWidth, PRInt32 aHeight)
{

  return Draw(aContext, aSurface, 0, 0, mWidth, mHeight, aX, aY, aWidth, aHeight);
}


NS_IMETHODIMP
nsImageMac::DrawToImage(nsIImage* aDstImage, PRInt32 aDX, PRInt32 aDY,
                        PRInt32 aDWidth, PRInt32 aDHeight)
{
  nsImageMac* dest = static_cast<nsImageMac*>(aDstImage);

  nsresult rv = EnsureCachedImage();
  NS_ENSURE_SUCCESS(rv, rv);
  rv = dest->EnsureCachedImage();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = NS_ERROR_FAILURE;

  
  
  
  PRInt32 width = dest->GetWidth();
  PRInt32 height = dest->GetHeight();
  PRInt32 bytesPerRow = dest->GetLineStride();
  PRInt32 totalBytes = height * bytesPerRow;
  PRUint8* bitmap = (PRUint8*) PR_Malloc(totalBytes);
  if (!bitmap)
    return NS_ERROR_OUT_OF_MEMORY;

  CGColorSpaceRef cs = ::CGColorSpaceCreateDeviceRGB();
  StColorSpaceReleaser csReleaser(cs);
  CGContextRef bitmapContext =
                  ::CGBitmapContextCreate(bitmap, width, height,
                                          BITS_PER_COMPONENT, bytesPerRow,
                                          cs, kCGImageAlphaPremultipliedFirst);

  if (bitmapContext)
  {
    CGRect destRect = ::CGRectMake(0, 0, width, height);
    CGRect drawRect = ::CGRectMake(aDX, aDY, aDWidth, aDHeight);

    
    ::CGContextClearRect(bitmapContext, destRect);

    
    ::CGContextDrawImage(bitmapContext, destRect, dest->mImage);
    ::CGContextDrawImage(bitmapContext, drawRect, mImage);

    ::CGContextRelease(bitmapContext);

    CGImageAlphaInfo alphaInfo = ::CGImageGetAlphaInfo(dest->mImage);

    
    CGDataProviderRef prov = ::CGDataProviderCreateWithData(NULL, bitmap,
                                                            totalBytes, NULL);
    CGImageRef newImageRef = ::CGImageCreate(width, height, BITS_PER_COMPONENT,
                                             BITS_PER_PIXEL, bytesPerRow, cs,
                                             alphaInfo, prov, NULL, TRUE,
                                             kCGRenderingIntentDefault);
    if (newImageRef)
    {
      
      dest->AdoptImage(newImageRef, bitmap);
      ::CGImageRelease(newImageRef);
      rv = NS_OK;
    }

    ::CGDataProviderRelease(prov);
  }

  return rv;
}

void
nsImageMac::AdoptImage(CGImageRef aNewImage, PRUint8* aNewBitmap)
{
  NS_PRECONDITION(aNewImage != nsnull && aNewBitmap != nsnull,
                  "null ptr");
  if (!aNewImage || !aNewBitmap)
    return;

  
  ::CGImageRelease(mImage);
  if (mImageBits)
    PR_Free(mImageBits);

  
  mImage = aNewImage;
  ::CGImageRetain(mImage);
  mImageBits = aNewBitmap;
}


#pragma mark -


nsresult
nsImageMac::Optimize(nsIDeviceContext* aContext)
{
  nsresult rv = EnsureCachedImage();
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  if (mImageBits && mAlphaDepth == 8) {
    PR_Free(mImageBits);
    mImageBits = NULL;
  }

  
  
  if (mAlphaBits) {
    PR_Free(mAlphaBits);
    mAlphaBits = NULL;
  }

  mOptimized = PR_TRUE;

  return NS_OK;
}


NS_IMETHODIMP
nsImageMac::LockImagePixels(PRBool aMaskPixels)
{
  if (!mOptimized)
    return NS_OK;

  
  
  
  PRUint8* imageBits = (PRUint8*) PR_Malloc(mHeight * mRowBytes);
  if (!imageBits)
    return NS_ERROR_OUT_OF_MEMORY;
  CGColorSpaceRef cs = ::CGColorSpaceCreateDeviceRGB();
  StColorSpaceReleaser csReleaser(cs);
  CGContextRef bitmapContext =
      ::CGBitmapContextCreate(imageBits, mWidth, mHeight, BITS_PER_COMPONENT,
                              mRowBytes, cs, kCGImageAlphaPremultipliedFirst);
  if (!bitmapContext) {
    PR_Free(imageBits);
    return NS_ERROR_FAILURE;
  }

  
  CGRect drawRect = ::CGRectMake(0, 0, mWidth, mHeight);
  ::CGContextClearRect(bitmapContext, drawRect);
  ::CGContextDrawImage(bitmapContext, drawRect, mImage);
  ::CGContextRelease(bitmapContext);

  
  

  if (mAlphaDepth) {
    
    mAlphaBits = (PRUint8*) PR_Malloc(mHeight * mAlphaRowBytes *
                                      sizeof(PRUint8));
    if (!mAlphaBits) {
      PR_Free(imageBits);
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  switch (mAlphaDepth)
  {
    case 8:
    {
      
      
      PRUint8* tmp = imageBits;
      mImageBits = (PRUint8*) PR_Malloc(mHeight * mRowBytes * sizeof(PRUint8));
      if (!mImageBits) {
        PR_Free(mAlphaBits);
        mAlphaBits = nsnull;
        PR_Free(imageBits);
        return NS_ERROR_OUT_OF_MEMORY;
      }

      
      for (PRInt32 y = 0; y < mHeight; y++)
      {
        PRInt32 rowStart = mRowBytes * y;
        PRInt32 alphaRowStart = mAlphaRowBytes * y;
        for (PRInt32 x = 0; x < mWidth; x++)
        {
          PRUint8 alpha = *tmp++;
          mAlphaBits[alphaRowStart + x] = alpha;
          PRUint32 offset = rowStart + COMPS_PER_PIXEL * x;
          if (alpha) {
            mImageBits[offset + 1] = ((PRUint32) *tmp++) * 255 / alpha;
            mImageBits[offset + 2] = ((PRUint32) *tmp++) * 255 / alpha;
            mImageBits[offset + 3] = ((PRUint32) *tmp++) * 255 / alpha;
          }
          else {
            tmp += 3;
            mImageBits[offset + 1] =
             mImageBits[offset + 2] =
             mImageBits[offset + 3] = 0;
          }
        }
      }
      break;
    }

    
    
    

    case 1:
    {
      
      for (PRInt32 y = 0; y < mHeight; y++)
      {
        PRInt32 rowStart = mRowBytes * y;
        PRUint8* alphaRow = mAlphaBits + mAlphaRowBytes * y;
        for (PRInt32 x = 0; x < mWidth; x++)
        {
          if (imageBits[rowStart + COMPS_PER_PIXEL * x])
            SetAlphaBit(alphaRow, x);
          else
            ClearAlphaBit(alphaRow, x);
        }
      }
    }

    case 0:
    default:
      break;
  }

  PR_Free(imageBits);
  return NS_OK;
}


NS_IMETHODIMP
nsImageMac::UnlockImagePixels(PRBool aMaskPixels)
{
  if (mOptimized)
    Optimize(nsnull);

  return NS_OK;
}


#pragma mark -


















PRInt32 nsImageMac::CalculateRowBytesInternal(PRUint32 aWidth, PRUint32 aDepth, PRBool aAllow2Bytes)
{
    PRInt32 rowBits = aWidth * aDepth;
    
    return (rowBits > 24 || !aAllow2Bytes) ?
        ((aWidth * aDepth + 31) / 32) * 4 :
        ((aWidth * aDepth + 15) / 16) * 2;
}






PRInt32 nsImageMac::CalculateRowBytes(PRUint32 aWidth, PRUint32 aDepth)
{
    return CalculateRowBytesInternal(aWidth, aDepth, PR_FALSE);
}


PRBool nsImageMac::RenderingToPrinter(nsIRenderingContext &aContext)
{
  nsCOMPtr<nsIDeviceContext> dc;                   
  aContext.GetDeviceContext(*getter_AddRefs(dc));
  
  nsDeviceContextMac* theDevContext = reinterpret_cast<nsDeviceContextMac*>(dc.get());
  return theDevContext && theDevContext->IsPrinter();
}


#pragma mark -











NS_IMETHODIMP
nsImageMac::ConvertToPICT(PicHandle* outPicture)
{
  *outPicture = nsnull;
  Rect picFrame  = {0, 0, mHeight, mWidth};

  PRUint32 pixelDepth = ::CGImageGetBitsPerPixel(mImage);

  
  GWorldPtr tempGWorld = NULL;
  ::NewGWorld(&tempGWorld, pixelDepth, &picFrame, nsnull, nsnull, 0);
  if (!tempGWorld)
    return NS_ERROR_FAILURE;

  PixMapHandle tempPixMap = ::GetGWorldPixMap(tempGWorld);
  if (tempPixMap)
  {
    StPixelLocker tempPixLocker(tempPixMap);      

    
    {
      StGWorldPortSetter setter(tempGWorld);
      ::BackColor(whiteColor);
      ::EraseRect(&picFrame);
    }

    
    GWorldPtr currPort;
    GDHandle currDev;
    ::GetGWorld(&currPort, &currDev);
    ::SetGWorld(tempGWorld, nsnull);

    ::SetOrigin(0, 0);
    ::ForeColor(blackColor);
    ::BackColor(whiteColor);

    
    
    
    PRUint8* bitmap = (PRUint8*) ::GetPixBaseAddr(tempPixMap);

    PRUint32 bytesPerPixel = ::CGImageGetBitsPerPixel(mImage) / 8;
    PRUint32 bitmapWidth = (mWidth + 4) & ~0x3;
    PRUint32 bitmapRowBytes = bitmapWidth * bytesPerPixel;

    CGColorSpaceRef cs = ::CGColorSpaceCreateDeviceRGB();
    StColorSpaceReleaser csReleaser(cs);
    CGContextRef bitmapContext =
                    ::CGBitmapContextCreate(bitmap, bitmapWidth, mHeight,
                                            BITS_PER_COMPONENT,
                                            bitmapRowBytes, cs,
                                            kCGImageAlphaPremultipliedFirst);

    NS_ASSERTION(bitmapContext, "Failed to create bitmap context");

    if (bitmapContext)
    {
      
      ::CGContextTranslateCTM(bitmapContext, 0, mHeight);
      ::CGContextScaleCTM(bitmapContext, 1, -1);

      
      CGRect drawRect = ::CGRectMake(0, 0, mWidth, mHeight);
      ::CGContextDrawImage(bitmapContext, drawRect, mImage);
      ::CGContextRelease(bitmapContext);

      PicHandle thePicture = ::OpenPicture(&picFrame);
      if (thePicture)
      {
        
        ::CopyBits(::GetPortBitMapForCopyBits(tempGWorld),
                   ::GetPortBitMapForCopyBits(tempGWorld),
                   &picFrame, &picFrame, ditherCopy, nsnull);

        ::ClosePicture();
        if (QDError() == noErr)
          *outPicture = thePicture;
      }
    }

    ::SetGWorld(currPort, currDev);     
  }

  ::DisposeGWorld(tempGWorld);        

  return *outPicture ? NS_OK : NS_ERROR_FAILURE;
}


NS_IMETHODIMP
nsImageMac::ConvertFromPICT(PicHandle inPicture)
{
  NS_WARNING("ConvertFromPICT is not implemented.");
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsImageMac::GetCGImageRef(CGImageRef* aCGImageRef)
{
  nsresult rv = EnsureCachedImage();
  if (NS_FAILED(rv)) return rv;

  *aCGImageRef = mImage;
  return NS_OK;
}


#pragma mark -

nsresult
nsImageMac::SlowTile(nsIRenderingContext &aContext, nsIDrawingSurface* aSurface,
                     PRInt32 aSXOffset, PRInt32 aSYOffset,
                     PRInt32 aPadX, PRInt32 aPadY, const nsRect &aTileRect)
{
  PRInt32
    validX = 0,
    validY = 0,
    validWidth  = mWidth,
    validHeight = mHeight;
  
  
  
  if (mDecodedY2 < mHeight) {
    validHeight = mDecodedY2 - mDecodedY1;
  }
  if (mDecodedX2 < mWidth) {
    validWidth = mDecodedX2 - mDecodedX1;
  }
  if (mDecodedY1 > 0) {   
    validHeight -= mDecodedY1;
    validY = mDecodedY1;
  }
  if (mDecodedX1 > 0) {
    validWidth -= mDecodedX1;
    validX = mDecodedX1; 
  }

  PRInt32 aY0 = aTileRect.y - aSYOffset,
          aX0 = aTileRect.x - aSXOffset,
          aY1 = aTileRect.y + aTileRect.height,
          aX1 = aTileRect.x + aTileRect.width;

  for (PRInt32 y = aY0; y < aY1; y += mHeight + aPadY)
    for (PRInt32 x = aX0; x < aX1; x += mWidth + aPadX)
    {
      Draw(aContext, aSurface,
           0, 0, PR_MIN(validWidth, aX1-x), PR_MIN(validHeight, aY1-y),     
           x, y, PR_MIN(validWidth, aX1-x), PR_MIN(validHeight, aY1-y));    
    }

  return NS_OK;
}


nsresult
nsImageMac::DrawTileQuickly(nsIRenderingContext &aContext,
                            nsIDrawingSurface* aSurface,
                            PRInt32 aSXOffset, PRInt32 aSYOffset,
                            const nsRect &aTileRect)
{
  CGColorSpaceRef cs = ::CGColorSpaceCreateDeviceRGB();
  StColorSpaceReleaser csReleaser(cs);

  PRUint32 tiledCols = (aTileRect.width + aSXOffset + mWidth - 1) / mWidth;
  PRUint32 tiledRows = (aTileRect.height + aSYOffset + mHeight - 1) / mHeight;

  
  
  PRUint32 bitmapWidth = tiledCols * mWidth;
  PRUint32 bitmapHeight = tiledRows * mHeight;
  PRUint32 bitmapRowBytes = tiledCols * mRowBytes;
  PRUint32 totalBytes = bitmapHeight * bitmapRowBytes;
  PRUint8* bitmap = (PRUint8*) PR_Malloc(totalBytes);
  if (!bitmap)
    return NS_ERROR_OUT_OF_MEMORY;

  CGContextRef bitmapContext;
  bitmapContext = ::CGBitmapContextCreate(bitmap, bitmapWidth, bitmapHeight,
                                          BITS_PER_COMPONENT, bitmapRowBytes,
                                          cs, kCGImageAlphaPremultipliedFirst);

  if (bitmapContext != NULL)
  {
    
    ::CGContextClearRect(bitmapContext,
                         ::CGRectMake(0, 0, bitmapWidth, bitmapHeight));
  
    
    
    
    CGRect drawRect = ::CGRectMake(0, bitmapHeight - mHeight, mWidth, mHeight);
    ::CGContextDrawImage(bitmapContext, drawRect, mImage);
    ::CGContextRelease(bitmapContext);

    
    
    
    
    PRUint32 tileHeight = mHeight;
    for (PRUint32 destCol = 1; destCol < tiledCols; destCol *= 2)
    {
      PRUint8* srcLine = bitmap;
      PRUint32 bytesToCopy = destCol * mRowBytes;
      PRUint8* destLine = srcLine + bytesToCopy;
      if (destCol * 2 > tiledCols)
      {
        bytesToCopy = (tiledCols - destCol) * mRowBytes;
      }
      for (PRUint32 row = 0; row < tileHeight; row++)
      {
        memcpy(destLine, srcLine, bytesToCopy);
        srcLine += bitmapRowBytes;
        destLine += bitmapRowBytes;
      }
    }

    for (PRUint32 destRow = 1; destRow < tiledRows; destRow *= 2)
    {
      PRUint32 tileRowBytes = mHeight * bitmapRowBytes;
      PRUint32 bytesToCopy = destRow * tileRowBytes;
      PRUint8* dest = bitmap + bytesToCopy;
      if (destRow * 2 > tiledRows)
      {
        bytesToCopy = (tiledRows - destRow) * tileRowBytes;
      }
      memcpy(dest, bitmap, bytesToCopy);
    }

    
    CGDataProviderRef prov = ::CGDataProviderCreateWithData(NULL, bitmap,
                                                            totalBytes, NULL);
    CGImageRef tiledImage = ::CGImageCreate(bitmapWidth, bitmapHeight,
                                            BITS_PER_COMPONENT, BITS_PER_PIXEL,
                                            bitmapRowBytes, cs,
                                            kCGImageAlphaPremultipliedFirst,
                                            prov, NULL, TRUE,
                                            kCGRenderingIntentDefault);
    ::CGDataProviderRelease(prov);

    nsDrawingSurfaceMac* surface = static_cast<nsDrawingSurfaceMac*>(aSurface);
    CGContextRef context = surface->StartQuartzDrawing();

    CGRect srcRect = ::CGRectMake(aSXOffset, aSYOffset, aTileRect.width,
                                  aTileRect.height);
    CGRect destRect = ::CGRectMake(aTileRect.x, aTileRect.y, aTileRect.width,
                                   aTileRect.height);

    drawRect = ::CGRectMake(0, 0, bitmapWidth, bitmapHeight);
    if (!::CGRectEqualToRect(srcRect, destRect))
    {
      
      float sx = ::CGRectGetWidth(destRect) / ::CGRectGetWidth(srcRect);
      float sy = ::CGRectGetHeight(destRect) / ::CGRectGetHeight(srcRect);
      float dx = ::CGRectGetMinX(destRect) - (::CGRectGetMinX(srcRect) * sx);
      float dy = ::CGRectGetMinY(destRect) - (::CGRectGetMinY(srcRect) * sy);
      drawRect = ::CGRectMake(dx, dy, bitmapWidth * sx, bitmapHeight * sy);
    }

    ::CGContextClipToRect(context, destRect);
    ::CGContextDrawImage(context, drawRect, tiledImage);

    ::CGImageRelease(tiledImage);
    surface->EndQuartzDrawing(context);
  }

  PR_Free(bitmap);

  return NS_OK;
}

void
DrawTileAsPattern(void *aInfo, CGContextRef aContext)
{
  CGImageRef image = static_cast<CGImageRef> (aInfo);

  float width = ::CGImageGetWidth(image);
  float height = ::CGImageGetHeight(image);
  CGRect drawRect = ::CGRectMake(0, 0, width, height);
  ::CGContextDrawImage(aContext, drawRect, image);
}

nsresult
nsImageMac::DrawTileWithQuartz(nsIDrawingSurface* aSurface,
                               PRInt32 aSXOffset, PRInt32 aSYOffset,
                               PRInt32 aPadX, PRInt32 aPadY,
                               const nsRect &aTileRect)
{
  nsDrawingSurfaceMac* surface = static_cast<nsDrawingSurfaceMac*>(aSurface);
  CGContextRef context = surface->StartQuartzDrawing();
  ::CGContextSaveGState(context);

  static const CGPatternCallbacks callbacks = {0, &DrawTileAsPattern, NULL};

  
  CGAffineTransform patternTrans = CGContextGetCTM(context);
  patternTrans = CGAffineTransformTranslate(patternTrans, aTileRect.x, aTileRect.y);

  CGPatternRef pattern;
  pattern = ::CGPatternCreate(mImage, ::CGRectMake(0, 0, mWidth, mHeight),
                              patternTrans, mWidth + aPadX, mHeight + aPadY,
                              kCGPatternTilingConstantSpacing,
                              TRUE, &callbacks);

  CGColorSpaceRef patternSpace = ::CGColorSpaceCreatePattern(NULL);
  ::CGContextSetFillColorSpace(context, patternSpace);
  ::CGColorSpaceRelease(patternSpace);

  float alpha = 1.0f;
  ::CGContextSetFillPattern(context, pattern, &alpha);
  ::CGPatternRelease(pattern);
  
  
  
  ::CGContextSetPatternPhase(context, CGSizeMake(-aSXOffset, aSYOffset));

  CGRect tileRect = ::CGRectMake(aTileRect.x,
                                 aTileRect.y,
                                 aTileRect.width,
                                 aTileRect.height);

  ::CGContextFillRect(context, tileRect);
  
  ::CGContextRestoreGState(context);
  surface->EndQuartzDrawing(context);
  return NS_OK;
}


NS_IMETHODIMP nsImageMac::DrawTile(nsIRenderingContext &aContext,
                                   nsIDrawingSurface* aSurface,
                                   PRInt32 aSXOffset, PRInt32 aSYOffset,
                                   PRInt32 aPadX, PRInt32 aPadY,
                                   const nsRect &aTileRect)
{
  nsresult rv = EnsureCachedImage();
  NS_ENSURE_SUCCESS(rv, rv);

  if (mDecodedX2 < mDecodedX1 || mDecodedY2 < mDecodedY1)
    return NS_OK;

  PRUint32 tiledCols = (aTileRect.width + aSXOffset + mWidth - 1) / mWidth;
  PRUint32 tiledRows = (aTileRect.height + aSYOffset + mHeight - 1) / mHeight;

  
  
  
  
#ifdef USE_CGPATTERN_TILING
  const PRUint32 kTilingCopyThreshold = 16;   
#else
  const PRUint32 kTilingCopyThreshold = 64;
#endif
  if (tiledCols * tiledRows < kTilingCopyThreshold)
    return SlowTile(aContext, aSurface, aSXOffset, aSYOffset, 0, 0, aTileRect);
  
#ifdef USE_CGPATTERN_TILING
  rv = DrawTileWithQuartz(aSurface, aSXOffset, aSYOffset, aPadX, aPadY, aTileRect);
#else
  
  if (!aPadX && !aPadY)
    rv = DrawTileQuickly(aContext, aSurface, aSXOffset, aSYOffset, aTileRect);
  else
    rv = SlowTile(aContext, aSurface, aSXOffset, aSYOffset, aPadX, aPadY, aTileRect);
#endif 

  return rv;
}
