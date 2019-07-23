






































#include "gfxImageFrame.h"
#include "nsIServiceManager.h"
#include <limits.h>

NS_IMPL_ISUPPORTS2(gfxImageFrame, gfxIImageFrame, nsIInterfaceRequestor)

gfxImageFrame::gfxImageFrame() :
  mInitialized(PR_FALSE),
  mMutable(PR_TRUE),
  mTimeout(100),
  mDisposalMethod(0), 
  mBlendMethod(1) 
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
    NS_WARNING("width or height too large");
    return NS_ERROR_FAILURE;
  }
  tmp = tmp * 4;
  if (tmp / 4 != aWidth * aHeight) {
    NS_WARNING("width or height too large");
    return NS_ERROR_FAILURE;
  }

  if ( (aDepth != 8) && (aDepth != 24) ){
    NS_ERROR("This Depth is not supported");
    return NS_ERROR_FAILURE;
  }

  
  const PRInt32 k64KLimit = 0x0000FFFF;
  if ( aWidth > k64KLimit || aHeight > k64KLimit ){
    NS_WARNING("image too big");
    return NS_ERROR_FAILURE;
  }
  
#if defined(XP_MACOSX)
  
  if (aHeight > SHRT_MAX) {
    NS_WARNING("image too big");
    return NS_ERROR_FAILURE;
  }
#endif

  nsresult rv;

  mOffset.MoveTo(aX, aY);
  mSize.SizeTo(aWidth, aHeight);

  mFormat = aFormat;

  mImage = do_CreateInstance("@mozilla.org/gfx/image;1", &rv);
  NS_ASSERTION(mImage, "creation of image failed");
  if (NS_FAILED(rv)) return rv;

  nsMaskRequirements maskReq;

  switch (aFormat) {
  case gfxIFormats::BGR:
  case gfxIFormats::RGB:
    maskReq = nsMaskRequirements_kNoMask;
    break;

  case gfxIFormats::BGRA:
  case gfxIFormats::RGBA:
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

  rv = mImage->Init(aWidth, aHeight, aDepth, maskReq);
  if (NS_FAILED(rv)) return rv;

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

  *aData = mImage->GetBits();
  *length = mImage->GetLineStride() * mSize.height;

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


NS_IMETHODIMP gfxImageFrame::GetBlendMethod(PRInt32 *aBlendMethod)
{
  if (!mInitialized)
    return NS_ERROR_NOT_INITIALIZED;

  *aBlendMethod = mBlendMethod;
  return NS_OK;
}
NS_IMETHODIMP gfxImageFrame::SetBlendMethod(PRInt32 aBlendMethod)
{
  if (!mInitialized)
    return NS_ERROR_NOT_INITIALIZED;

  mBlendMethod = (PRInt8)aBlendMethod;
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
