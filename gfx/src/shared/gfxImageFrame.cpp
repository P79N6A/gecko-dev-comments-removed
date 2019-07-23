






































#include "gfxImageFrame.h"
#include "nsIServiceManager.h"

NS_IMPL_ISUPPORTS2(gfxImageFrame, gfxIImageFrame, nsIInterfaceRequestor)

gfxImageFrame::gfxImageFrame() :
  mInitialized(PR_FALSE),
  mMutable(PR_TRUE),
  mHasBackgroundColor(PR_FALSE),
  mTimeout(100),
  mBackgroundColor(0),
  mDisposalMethod(0)
{
  
}

gfxImageFrame::~gfxImageFrame()
{
  
}


NS_IMETHODIMP gfxImageFrame::Init(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight, gfx_format aFormat,gfx_depth aDepth)
{
  if (mInitialized)
    return NS_ERROR_FAILURE;

  if (aWidth <= 0 || aHeight <= 0) {
    NS_ASSERTION(0, "error - negative image size\n");
    return NS_ERROR_FAILURE;
  }

  
  PRInt32 tmp = aWidth * aHeight;
  if (tmp / aHeight != aWidth) {
    NS_ASSERTION(0, "width or height too large\n");
    return NS_ERROR_FAILURE;
  }
  tmp = tmp * 4;
  if (tmp / 4 != aWidth * aHeight) {
    NS_ASSERTION(0, "width or height too large\n");
    return NS_ERROR_FAILURE;
  }

  if ( (aDepth != 8) && (aDepth != 24) ){
    NS_ERROR("This Depth is not supported\n");
    return NS_ERROR_FAILURE;
  }

  
  const PRInt32 k64KLimit = 0x0000FFFF;
  if ( aWidth > k64KLimit || aHeight > k64KLimit ){
    NS_ERROR("image too big");
    return NS_ERROR_FAILURE;
  }

  nsresult rv;

  mOffset.MoveTo(aX, aY);
  mSize.SizeTo(aWidth, aHeight);

  mFormat = aFormat;

  mImage = do_CreateInstance("@mozilla.org/gfx/image;1", &rv);
  NS_ASSERTION(mImage, "creation of image failed");
  if (NS_FAILED(rv)) return rv;

  gfx_depth depth = aDepth;
  nsMaskRequirements maskReq;

  switch (aFormat) {
  case gfxIFormats::BGR:
  case gfxIFormats::RGB:
    maskReq = nsMaskRequirements_kNoMask;
    break;

  case gfxIFormats::BGRA:
  case gfxIFormats::RGBA:
#ifdef DEBUG
    printf("we can't do this with the old image code\n");
#endif
    maskReq = nsMaskRequirements_kNeeds8Bit;
    break;

  case gfxIFormats::BGR_A1:
  case gfxIFormats::RGB_A1:
    maskReq = nsMaskRequirements_kNeeds1Bit;
    break;

  case gfxIFormats::BGR_A8:
  case gfxIFormats::RGB_A8:
    maskReq = nsMaskRequirements_kNeeds8Bit;
    break;

  default:
#ifdef DEBUG
    printf("unsupported gfx_format\n");
#endif
    break;
  }

  rv = mImage->Init(aWidth, aHeight, depth, maskReq);
  if (NS_FAILED(rv)) return rv;

  mTopToBottom = mImage->GetIsRowOrderTopToBottom();

  mInitialized = PR_TRUE;
  return NS_OK;
}



NS_IMETHODIMP gfxImageFrame::GetMutable(PRBool *aMutable)
{
  if (!mInitialized)
    return NS_ERROR_NOT_INITIALIZED;

  NS_ASSERTION(mInitialized, "gfxImageFrame::GetMutable called on non-inited gfxImageFrame");
  *aMutable = mMutable;
  return NS_OK;
}

NS_IMETHODIMP gfxImageFrame::SetMutable(PRBool aMutable)
{
  if (!mInitialized)
    return NS_ERROR_NOT_INITIALIZED;

  mMutable = aMutable;

  if (!aMutable)
    mImage->Optimize(nsnull);

  return NS_OK;
}


NS_IMETHODIMP gfxImageFrame::GetX(PRInt32 *aX)
{
  if (!mInitialized)
    return NS_ERROR_NOT_INITIALIZED;

  *aX = mOffset.x;
  return NS_OK;
}


NS_IMETHODIMP gfxImageFrame::GetY(PRInt32 *aY)
{
  if (!mInitialized)
    return NS_ERROR_NOT_INITIALIZED;

  *aY = mOffset.y;
  return NS_OK;
}



NS_IMETHODIMP gfxImageFrame::GetWidth(PRInt32 *aWidth)
{
  if (!mInitialized)
    return NS_ERROR_NOT_INITIALIZED;

  *aWidth = mSize.width;
  return NS_OK;
}


NS_IMETHODIMP gfxImageFrame::GetHeight(PRInt32 *aHeight)
{
  if (!mInitialized)
    return NS_ERROR_NOT_INITIALIZED;

  *aHeight = mSize.height;
  return NS_OK;
}


NS_IMETHODIMP gfxImageFrame::GetRect(nsIntRect &aRect)
{
  if (!mInitialized)
    return NS_ERROR_NOT_INITIALIZED;

  aRect.SetRect(mOffset.x, mOffset.y, mSize.width, mSize.height);

  return NS_OK;
}


NS_IMETHODIMP gfxImageFrame::GetFormat(gfx_format *aFormat)
{
  if (!mInitialized)
    return NS_ERROR_NOT_INITIALIZED;

  *aFormat = mFormat;
  return NS_OK;
}


NS_IMETHODIMP gfxImageFrame::GetNeedsBackground(PRBool *aNeedsBackground)
{
  if (!mInitialized)
    return NS_ERROR_NOT_INITIALIZED;

  *aNeedsBackground = (mFormat != gfxIFormats::RGB && 
                       mFormat != gfxIFormats::BGR) ||
                      !mImage->GetIsImageComplete();
  return NS_OK;
}



NS_IMETHODIMP gfxImageFrame::GetImageBytesPerRow(PRUint32 *aBytesPerRow)
{
  if (!mInitialized)
    return NS_ERROR_NOT_INITIALIZED;

  *aBytesPerRow = mImage->GetLineStride();
  return NS_OK;
}


NS_IMETHODIMP gfxImageFrame::GetImageDataLength(PRUint32 *aBitsLength)
{
  if (!mInitialized)
    return NS_ERROR_NOT_INITIALIZED;

  *aBitsLength = mImage->GetLineStride() * mSize.height;
  return NS_OK;
}


NS_IMETHODIMP gfxImageFrame::GetImageData(PRUint8 **aData, PRUint32 *length)
{
  if (!mInitialized)
    return NS_ERROR_NOT_INITIALIZED;

  NS_ASSERTION(mMutable, "trying to get data on an immutable frame");

  *aData = mImage->GetBits();
  *length = mImage->GetLineStride() * mSize.height;

  return NS_OK;
}


NS_IMETHODIMP gfxImageFrame::SetImageData(const PRUint8 *aData, PRUint32 aLength, PRInt32 aOffset)
{
  return SetData(aData, aLength, aOffset, PR_FALSE);
}

nsresult gfxImageFrame::SetData(const PRUint8 *aData, PRUint32 aLength, 
                                PRInt32 aOffset, PRBool aSetAlpha)
{
  if (!mInitialized)
    return NS_ERROR_NOT_INITIALIZED;

  NS_ASSERTION(mMutable, "trying to set data on an immutable frame");
  NS_ASSERTION(!(aOffset<0), "can't have a negative offset");
  if (!mMutable || aOffset < 0)
    return NS_ERROR_FAILURE;

  if (aSetAlpha && !mImage->GetHasAlphaMask())
    return NS_ERROR_NOT_INITIALIZED;

  if (aLength == 0)
    return NS_OK;

  mImage->LockImagePixels(aSetAlpha);
  PRUint8 *imgData = aSetAlpha ? mImage->GetAlphaBits() : mImage->GetBits();
  const PRUint32 rowStride = aSetAlpha ? mImage->GetAlphaLineStride() : mImage->GetLineStride();
  const PRUint32 dataLength = rowStride * mSize.height;
  const PRUint32 numRowsToSet = 1 + ((aLength-1) / rowStride);
  const PRUint32 firstRowToSet = (aOffset / rowStride);

  
  
  if ((((PRUint32)aOffset + aLength) > dataLength) || !imgData) {
    mImage->UnlockImagePixels(aSetAlpha);
    return NS_ERROR_FAILURE;
  }

  if (mTopToBottom) {
    
    if (aData)
      memcpy(imgData + aOffset, aData, aLength);
    else
      memset(imgData + aOffset, 0, aLength);
  } else {
    
    
    PRUint32 xOffset = aOffset % rowStride;
    PRUint32 yOffset = (mSize.height - firstRowToSet - 1) * rowStride;
    if (aData) {
      
      for (PRUint32 i=0; i<numRowsToSet; i++) {
        PRUint32 lengthOfRowToSet = rowStride - xOffset;
        lengthOfRowToSet = PR_MIN(lengthOfRowToSet, aLength);
        memcpy(imgData + yOffset + xOffset, aData, lengthOfRowToSet);
        aData += lengthOfRowToSet;
        aLength -= lengthOfRowToSet;
        yOffset -= rowStride;
        xOffset = 0;
      }
    } else {
      
      if (xOffset) {
        
        PRUint32 lengthOfRowToSet = rowStride - xOffset;
        lengthOfRowToSet = PR_MIN(lengthOfRowToSet, aLength);
        memset(imgData + yOffset + xOffset, 0, lengthOfRowToSet);
        aLength -= lengthOfRowToSet;
        yOffset -= rowStride;
      }
      if (aLength > rowStride) {
        
        const PRUint32 wholeRows = rowStride * (PRUint32)(aLength / rowStride);
        memset(imgData + yOffset - (wholeRows - rowStride), 0, wholeRows);
        aLength -= wholeRows;
        yOffset -= wholeRows;
      }
      if (aLength) {
        
        memset(imgData + yOffset, 0, aLength);
      }
    }
  }
  mImage->UnlockImagePixels(aSetAlpha);

  if (!aSetAlpha) {
    
    nsIntRect r(0, firstRowToSet, mSize.width, numRowsToSet);
    mImage->ImageUpdated(nsnull, nsImageUpdateFlags_kBitsChanged, &r);
  }
  return NS_OK;
}


NS_IMETHODIMP gfxImageFrame::LockImageData()
{
  if (!mInitialized)
    return NS_ERROR_NOT_INITIALIZED;

  return mImage->LockImagePixels(PR_FALSE);
}


NS_IMETHODIMP gfxImageFrame::UnlockImageData()
{
  if (!mInitialized)
    return NS_ERROR_NOT_INITIALIZED;

  return mImage->UnlockImagePixels(PR_FALSE);
}


NS_IMETHODIMP gfxImageFrame::GetAlphaBytesPerRow(PRUint32 *aBytesPerRow)
{
  if (!mInitialized || !mImage->GetHasAlphaMask())
    return NS_ERROR_NOT_INITIALIZED;

  *aBytesPerRow = mImage->GetAlphaLineStride();
  return NS_OK;
}


NS_IMETHODIMP gfxImageFrame::GetAlphaDataLength(PRUint32 *aBitsLength)
{
  if (!mInitialized || !mImage->GetHasAlphaMask())
    return NS_ERROR_NOT_INITIALIZED;

  *aBitsLength = mImage->GetAlphaLineStride() * mSize.height;
  return NS_OK;
}


NS_IMETHODIMP gfxImageFrame::GetAlphaData(PRUint8 **aData, PRUint32 *length)
{
  if (!mInitialized || !mImage->GetHasAlphaMask())
    return NS_ERROR_NOT_INITIALIZED;

  NS_ASSERTION(mMutable, "trying to get data on an immutable frame");

  *aData = mImage->GetAlphaBits();
  *length = mImage->GetAlphaLineStride() * mSize.height;

  return NS_OK;
}


NS_IMETHODIMP gfxImageFrame::SetAlphaData(const PRUint8 *aData, PRUint32 aLength, PRInt32 aOffset)
{
  return SetData(aData, aLength, aOffset, PR_TRUE);
}


NS_IMETHODIMP gfxImageFrame::LockAlphaData()
{
  if (!mInitialized || !mImage->GetHasAlphaMask())
    return NS_ERROR_NOT_INITIALIZED;

  return mImage->LockImagePixels(PR_TRUE);
}


NS_IMETHODIMP gfxImageFrame::UnlockAlphaData()
{
  if (!mInitialized || !mImage->GetHasAlphaMask())
    return NS_ERROR_NOT_INITIALIZED;

  return mImage->UnlockImagePixels(PR_TRUE);
}






NS_IMETHODIMP gfxImageFrame::DrawTo(gfxIImageFrame* aDst, PRInt32 aDX, PRInt32 aDY, PRInt32 aDWidth, PRInt32 aDHeight)
{
  if (!mInitialized)
    return NS_ERROR_NOT_INITIALIZED;

  nsCOMPtr<nsIImage> img(do_GetInterface(aDst));
  return mImage->DrawToImage(img, aDX, aDY, aDWidth, aDHeight);
}



NS_IMETHODIMP gfxImageFrame::GetTimeout(PRInt32 *aTimeout)
{
  if (!mInitialized)
    return NS_ERROR_NOT_INITIALIZED;

  
  
  
  
  
  
  
  
  
  
  
  
  
  if (mTimeout >= 0 && mTimeout <= 10)
    *aTimeout = 100;
  else
    *aTimeout = mTimeout;

  return NS_OK;
}

NS_IMETHODIMP gfxImageFrame::SetTimeout(PRInt32 aTimeout)
{
  if (!mInitialized)
    return NS_ERROR_NOT_INITIALIZED;

  mTimeout = aTimeout;
  return NS_OK;
}


NS_IMETHODIMP gfxImageFrame::GetFrameDisposalMethod(PRInt32 *aFrameDisposalMethod)
{
  if (!mInitialized)
    return NS_ERROR_NOT_INITIALIZED;

  *aFrameDisposalMethod = mDisposalMethod;
  return NS_OK;
}
NS_IMETHODIMP gfxImageFrame::SetFrameDisposalMethod(PRInt32 aFrameDisposalMethod)
{
  if (!mInitialized)
    return NS_ERROR_NOT_INITIALIZED;

  mDisposalMethod = aFrameDisposalMethod;
  return NS_OK;
}


NS_IMETHODIMP gfxImageFrame::GetBackgroundColor(gfx_color *aBackgroundColor)
{
  if (!mInitialized || !mHasBackgroundColor)
    return NS_ERROR_NOT_INITIALIZED;

  *aBackgroundColor = mBackgroundColor;
  return NS_OK;
}
NS_IMETHODIMP gfxImageFrame::SetBackgroundColor(gfx_color aBackgroundColor)
{
  if (!mInitialized)
    return NS_ERROR_NOT_INITIALIZED;

  mBackgroundColor = aBackgroundColor;
  mHasBackgroundColor = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP gfxImageFrame::GetInterface(const nsIID & aIID, void * *result)
{
  if (!mInitialized)
    return NS_ERROR_NOT_INITIALIZED;

  NS_ENSURE_ARG_POINTER(result);

  if (NS_SUCCEEDED(QueryInterface(aIID, result)))
    return NS_OK;
  if (mImage && aIID.Equals(NS_GET_IID(nsIImage)))
    return mImage->QueryInterface(aIID, result);

  return NS_NOINTERFACE;
}
