




































#include "nsCOMPtr.h"
#include "nsRenderingContextImpl.h"
#include "nsIDeviceContext.h"
#include "nsIImage.h"
#include "nsTransform2D.h"
#include "nsIRegion.h"
#include "nsIFontMetrics.h"
#include <stdlib.h>


nsIDrawingSurface* nsRenderingContextImpl::gBackbuffer = nsnull;
nsRect nsRenderingContextImpl::gBackbufferBounds = nsRect(0, 0, 0, 0);
nsSize nsRenderingContextImpl::gLargestRequestedSize = nsSize(0, 0);






nsRenderingContextImpl :: nsRenderingContextImpl()
: mTranMatrix(nsnull)
, mAct(0)
, mActive(nsnull)
, mPenMode(nsPenMode_kNone)
{
}





nsRenderingContextImpl :: ~nsRenderingContextImpl()
{


}


NS_IMETHODIMP nsRenderingContextImpl::GetBackbuffer(const nsRect &aRequestedSize, const nsRect &aMaxSize, PRBool aForBlending, nsIDrawingSurface* &aBackbuffer)
{
  
  
  
  
  return AllocateBackbuffer(aRequestedSize, aMaxSize, aBackbuffer, PR_TRUE, 0);
}

nsresult nsRenderingContextImpl::AllocateBackbuffer(const nsRect &aRequestedSize, const nsRect &aMaxSize, nsIDrawingSurface* &aBackbuffer, PRBool aCacheBackbuffer, PRUint32 aSurfFlags)
{
  nsRect newBounds;
  nsresult rv = NS_OK;

   if (! aCacheBackbuffer) {
    newBounds = aRequestedSize;
  } else {
    GetDrawingSurfaceSize(aMaxSize, aRequestedSize, newBounds);
  }

  if ((nsnull == gBackbuffer)
      || (gBackbufferBounds.width != newBounds.width)
      || (gBackbufferBounds.height != newBounds.height))
    {
      if (gBackbuffer) {
        
        DestroyDrawingSurface(gBackbuffer);
        gBackbuffer = nsnull;
      }

      rv = CreateDrawingSurface(newBounds, aSurfFlags, gBackbuffer);
      
      if (NS_SUCCEEDED(rv)) {
        gBackbufferBounds = newBounds;
        SelectOffScreenDrawingSurface(gBackbuffer);
      } else {
        gBackbufferBounds.SetRect(0,0,0,0);
        gBackbuffer = nsnull;
      }
    } else {
      SelectOffScreenDrawingSurface(gBackbuffer);

      nsCOMPtr<nsIDeviceContext>  dx;
      GetDeviceContext(*getter_AddRefs(dx));
      nsRect bounds = aRequestedSize;
      bounds *= dx->AppUnitsPerDevPixel();

      SetClipRect(bounds, nsClipCombine_kReplace);
    }

  aBackbuffer = gBackbuffer;
  return rv;
}

NS_IMETHODIMP nsRenderingContextImpl::ReleaseBackbuffer(void)
{
  
  
  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextImpl::DestroyCachedBackbuffer(void)
{
  if (gBackbuffer) {
    DestroyDrawingSurface(gBackbuffer);
    gBackbuffer = nsnull;
  }
  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextImpl::UseBackbuffer(PRBool* aUseBackbuffer)
{
  *aUseBackbuffer = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextImpl::PushTranslation(PushedTranslation* aState)
{
  
  
  
  
  
  
  
  
  
  
  
  
  
  nsTransform2D *theTransform; 
  GetCurrentTransform(theTransform);
  NS_ASSERTION(theTransform != nsnull, "The rendering context transform is null");
  theTransform->GetTranslation(&aState->mSavedX, &aState->mSavedY);
  
  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextImpl::PopTranslation(PushedTranslation* aState)
{
  nsTransform2D *theTransform; 
  GetCurrentTransform(theTransform);
  NS_ASSERTION(theTransform != nsnull, "The rendering context transform is null");
  theTransform->SetTranslation(aState->mSavedX, aState->mSavedY);

  return NS_OK;
}

NS_IMETHODIMP nsRenderingContextImpl::SetTranslation(nscoord aX, nscoord aY)
{
  nsTransform2D *theTransform; 
  GetCurrentTransform(theTransform);
  NS_ASSERTION(theTransform != nsnull, "The rendering context transform is null");
  theTransform->SetTranslation(aX, aY);
  return NS_OK;
}

PRBool nsRenderingContextImpl::RectFitsInside(const nsRect& aRect, PRInt32 aWidth, PRInt32 aHeight) const
{
  if (aRect.width > aWidth)
    return (PR_FALSE);

  if (aRect.height > aHeight)
    return (PR_FALSE);

  return PR_TRUE;
}

PRBool nsRenderingContextImpl::BothRectsFitInside(const nsRect& aRect1, const nsRect& aRect2, PRInt32 aWidth, PRInt32 aHeight, nsRect& aNewSize) const
{
  if (PR_FALSE == RectFitsInside(aRect1, aWidth, aHeight)) {
    return PR_FALSE;
  }

  if (PR_FALSE == RectFitsInside(aRect2, aWidth, aHeight)) {
    return PR_FALSE;
  }

  aNewSize.width = aWidth;
  aNewSize.height = aHeight;

  return PR_TRUE;
}

void nsRenderingContextImpl::GetDrawingSurfaceSize(const nsRect& aMaxBackbufferSize, const nsRect& aRequestedSize, nsRect& aNewSize) 
{ 
  CalculateDiscreteSurfaceSize(aMaxBackbufferSize, aRequestedSize, aNewSize);
  aNewSize.MoveTo(aRequestedSize.x, aRequestedSize.y);
}

void nsRenderingContextImpl::CalculateDiscreteSurfaceSize(const nsRect& aMaxBackbufferSize, const nsRect& aRequestedSize, nsRect& aSurfaceSize) 
{
  
  nscoord height;
  nscoord width;

  nsCOMPtr<nsIDeviceContext>  dx;
  GetDeviceContext(*getter_AddRefs(dx));
  dx->GetDeviceSurfaceDimensions(width, height);

  PRInt32 p2a = dx->AppUnitsPerDevPixel();
  PRInt32 screenHeight = NSAppUnitsToIntPixels(height, p2a);
  PRInt32 screenWidth = NSAppUnitsToIntPixels(width, p2a);

  

  
  if (BothRectsFitInside(aRequestedSize, aMaxBackbufferSize, screenWidth / 8, screenHeight / 8, aSurfaceSize)) {
    return;
  }

  
  if (BothRectsFitInside(aRequestedSize, aMaxBackbufferSize, screenWidth / 4, screenHeight / 4, aSurfaceSize)) {
    return;
  }

  
  if (BothRectsFitInside(aRequestedSize, aMaxBackbufferSize, screenWidth / 2, screenHeight / 2, aSurfaceSize)) {
    return;
  }

  
  if (BothRectsFitInside(aRequestedSize, aMaxBackbufferSize, (screenWidth * 3) / 4, (screenHeight * 3) / 4, aSurfaceSize)) {
    return;
  }

  
  if (BothRectsFitInside(aRequestedSize, aMaxBackbufferSize, (screenWidth * 3) / 4, screenHeight, aSurfaceSize)) {
    return;
  }

  
  if (BothRectsFitInside(aRequestedSize, aMaxBackbufferSize, screenWidth, screenHeight, aSurfaceSize)) {
    return;
  }

  
  if (BothRectsFitInside(aRequestedSize, aMaxBackbufferSize, gLargestRequestedSize.width, gLargestRequestedSize.height, aSurfaceSize)) {
    return;
  } else {
    gLargestRequestedSize.width = PR_MAX(aRequestedSize.width, aMaxBackbufferSize.width);
    gLargestRequestedSize.height = PR_MAX(aRequestedSize.height, aMaxBackbufferSize.height);
    aSurfaceSize.width = gLargestRequestedSize.width;
    aSurfaceSize.height = gLargestRequestedSize.height;
    
  }
}







NS_IMETHODIMP
nsRenderingContextImpl::SetRightToLeftText(PRBool aIsRTL)
{
  return NS_OK;
}

NS_IMETHODIMP
nsRenderingContextImpl::GetRightToLeftText(PRBool* aIsRTL)
{
  *aIsRTL = PR_FALSE;
  return NS_OK;
}

#include "imgIContainer.h"
#include "gfxIImageFrame.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"

#ifndef MOZ_CAIRO_GFX
NS_IMETHODIMP nsRenderingContextImpl::DrawImage(imgIContainer *aImage, const nsRect & aSrcRect, const nsRect & aDestRect)
{
  nsRect dr = aDestRect;
  mTranMatrix->TransformCoord(&dr.x, &dr.y, &dr.width, &dr.height);

  
  
  
  
  
  
  
  nsRect sr(aDestRect.TopLeft(), aSrcRect.Size());
  mTranMatrix->TransformCoord(&sr.x, &sr.y, &sr.width, &sr.height);
  
  if (sr.IsEmpty() || dr.IsEmpty())
    return NS_OK;

  sr.MoveTo(aSrcRect.TopLeft());
  mTranMatrix->TransformNoXLateCoord(&sr.x, &sr.y);

  nsCOMPtr<gfxIImageFrame> iframe;
  aImage->GetCurrentFrame(getter_AddRefs(iframe));
  if (!iframe) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIImage> img(do_GetInterface(iframe));
  if (!img) return NS_ERROR_FAILURE;

  nsIDrawingSurface *surface = nsnull;
  GetDrawingSurface(&surface);
  if (!surface) return NS_ERROR_FAILURE;

  
  
  nsRect iframeRect;
  iframe->GetRect(iframeRect);
  
  if (iframeRect.x > 0) {
    
    sr.x -= iframeRect.x;

    nscoord scaled_x = sr.x;
    if (dr.width != sr.width) {
      PRFloat64 scale_ratio = PRFloat64(dr.width) / PRFloat64(sr.width);
      scaled_x = NSToCoordRound(scaled_x * scale_ratio);
    }
    if (sr.x < 0) {
      dr.x -= scaled_x;
      sr.width += sr.x;
      dr.width += scaled_x;
      if (sr.width <= 0 || dr.width <= 0)
        return NS_OK;
      sr.x = 0;
    } else if (sr.x > iframeRect.width) {
      return NS_OK;
    }
  }

  if (iframeRect.y > 0) {
    
    sr.y -= iframeRect.y;

    nscoord scaled_y = sr.y;
    if (dr.height != sr.height) {
      PRFloat64 scale_ratio = PRFloat64(dr.height) / PRFloat64(sr.height);
      scaled_y = NSToCoordRound(scaled_y * scale_ratio);
    }
    if (sr.y < 0) {
      dr.y -= scaled_y;
      sr.height += sr.y;
      dr.height += scaled_y;
      if (sr.height <= 0 || dr.height <= 0)
        return NS_OK;
      sr.y = 0;
    } else if (sr.y > iframeRect.height) {
      return NS_OK;
    }
  }

  
  
  nsCOMPtr<nsIRegion> clipRegion;
  GetClipRegion(getter_AddRefs(clipRegion));
  if (clipRegion && !clipRegion->ContainsRect(dr.x, dr.y, dr.width, dr.height))
    return NS_OK;

  return img->Draw(*this, surface, sr.x, sr.y, sr.width, sr.height,
                   dr.x, dr.y, dr.width, dr.height);
}
#endif


NS_IMETHODIMP
nsRenderingContextImpl::DrawTile(imgIContainer *aImage,
                                 nscoord aXImageStart, nscoord aYImageStart,
                                 const nsRect * aTargetRect)
{
  nsRect dr(*aTargetRect);
  mTranMatrix->TransformCoord(&dr.x, &dr.y, &dr.width, &dr.height);
  mTranMatrix->TransformCoord(&aXImageStart, &aYImageStart);

  
  if (dr.IsEmpty())
    return NS_OK;

  nscoord width, height;
  aImage->GetWidth(&width);
  aImage->GetHeight(&height);

  if (width == 0 || height == 0)
    return NS_OK;

  nscoord xOffset = (dr.x - aXImageStart) % width;
  nscoord yOffset = (dr.y - aYImageStart) % height;

  nsCOMPtr<gfxIImageFrame> iframe;
  aImage->GetCurrentFrame(getter_AddRefs(iframe));
  if (!iframe) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIImage> img(do_GetInterface(iframe));
  if (!img) return NS_ERROR_FAILURE;

  nsIDrawingSurface *surface = nsnull;
  GetDrawingSurface(&surface);
  if (!surface) return NS_ERROR_FAILURE;

  
  nsRect iframeRect;
  iframe->GetRect(iframeRect);
  PRInt32 padx = width - iframeRect.width;
  PRInt32 pady = height - iframeRect.height;

  return img->DrawTile(*this, surface,
                       xOffset - iframeRect.x, yOffset - iframeRect.y,
                       padx, pady,
                       dr);
}

NS_IMETHODIMP
nsRenderingContextImpl::FlushRect(const nsRect& aRect)
{
  return NS_OK;
}

NS_IMETHODIMP
nsRenderingContextImpl::FlushRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{
    return NS_OK;
}

NS_IMETHODIMP
nsRenderingContextImpl::GetClusterInfo(const PRUnichar *aText,
                                       PRUint32 aLength,
                                       PRUint8 *aClusterStarts)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

PRInt32
nsRenderingContextImpl::GetPosition(const PRUnichar *aText,
                                    PRUint32 aLength,
                                    nsPoint aPt)
{
  return -1;
}

NS_IMETHODIMP
nsRenderingContextImpl::GetRangeWidth(const PRUnichar *aText,
                                      PRUint32 aLength,
                                      PRUint32 aStart,
                                      PRUint32 aEnd,
                                      PRUint32 &aWidth)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsRenderingContextImpl::GetRangeWidth(const char *aText,
                                      PRUint32 aLength,
                                      PRUint32 aStart,
                                      PRUint32 aEnd,
                                      PRUint32 &aWidth)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}



#define MAX_GFX_TEXT_BUF_SIZE 8000
static PRInt32 GetMaxChunkLength(nsRenderingContextImpl* aContext)
{
  PRInt32 len = aContext->GetMaxStringLength();
  return PR_MIN(len, MAX_GFX_TEXT_BUF_SIZE);
}

static PRInt32 FindSafeLength(nsRenderingContextImpl* aContext,
                              const PRUnichar *aString, PRUint32 aLength,
                              PRUint32 aMaxChunkLength)
{
  if (aLength <= aMaxChunkLength)
    return aLength;
  
  PRUint8 buffer[MAX_GFX_TEXT_BUF_SIZE + 1];
  
  PRUint32 clusterHint;
  aContext->GetHints(clusterHint);
  clusterHint &= NS_RENDERING_HINT_TEXT_CLUSTERS;

  PRInt32 len = aMaxChunkLength;

  if (clusterHint) {
    nsresult rv =
      aContext->GetClusterInfo(aString, aMaxChunkLength + 1, buffer);
    if (NS_FAILED(rv))
      return len;
  }

  
  while (len > 0 &&
         (NS_IS_LOW_SURROGATE(aString[len]) || (clusterHint && !buffer[len]))) {
    len--;
  }
  if (len == 0) {
    
    
    
    
    return aMaxChunkLength;
  }
  return len;
}

static PRInt32 FindSafeLength(nsRenderingContextImpl* aContext,
                              const char *aString, PRUint32 aLength,
                              PRUint32 aMaxChunkLength)
{
  
  return PR_MIN(aLength, aMaxChunkLength);
}

NS_IMETHODIMP
nsRenderingContextImpl::GetWidth(const nsString& aString, nscoord &aWidth,
                                 PRInt32 *aFontID)
{
  return GetWidth(aString.get(), aString.Length(), aWidth, aFontID);
}

NS_IMETHODIMP
nsRenderingContextImpl::GetWidth(const char* aString, nscoord& aWidth)
{
  return GetWidth(aString, strlen(aString), aWidth);
}

NS_IMETHODIMP
nsRenderingContextImpl::DrawString(const nsString& aString, nscoord aX, nscoord aY,
                                   PRInt32 aFontID, const nscoord* aSpacing)
{
  return DrawString(aString.get(), aString.Length(), aX, aY, aFontID, aSpacing);
}

NS_IMETHODIMP
nsRenderingContextImpl::GetWidth(const char* aString, PRUint32 aLength,
                                 nscoord& aWidth)
{
  PRUint32 maxChunkLength = GetMaxChunkLength(this);
  aWidth = 0;
  while (aLength > 0) {
    PRInt32 len = FindSafeLength(this, aString, aLength, maxChunkLength);
    nscoord width;
    nsresult rv = GetWidthInternal(aString, len, width);
    if (NS_FAILED(rv))
      return rv;
    aWidth += width;
    aLength -= len;
    aString += len;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsRenderingContextImpl::GetWidth(const PRUnichar *aString, PRUint32 aLength,
                                 nscoord &aWidth, PRInt32 *aFontID)
{
  PRUint32 maxChunkLength = GetMaxChunkLength(this);
  aWidth = 0;
  
  if (aFontID) {
    *aFontID = 0;
  }
  
  while (aLength > 0) {
    PRInt32 len = FindSafeLength(this, aString, aLength, maxChunkLength);
    nscoord width;
    nsresult rv = GetWidthInternal(aString, len, width);
    if (NS_FAILED(rv))
      return rv;
    aWidth += width;
    aLength -= len;
    aString += len;
  }
  return NS_OK;
}  

NS_IMETHODIMP
nsRenderingContextImpl::GetTextDimensions(const char* aString, PRUint32 aLength,
                                          nsTextDimensions& aDimensions)
{
  PRUint32 maxChunkLength = GetMaxChunkLength(this);
  if (aLength <= maxChunkLength)
    return GetTextDimensionsInternal(aString, aLength, aDimensions);
 
  PRBool firstIteration = PR_TRUE;
  while (aLength > 0) {
    PRInt32 len = FindSafeLength(this, aString, aLength, maxChunkLength);
    nsTextDimensions dimensions;
    nsresult rv = GetTextDimensionsInternal(aString, len, dimensions);
    if (NS_FAILED(rv))
      return rv;
    if (firstIteration) {
      
      
      
      aDimensions = dimensions;
    } else {
      aDimensions.Combine(dimensions);
    }
    aLength -= len;
    aString += len;
    firstIteration = PR_FALSE;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsRenderingContextImpl::GetTextDimensions(const PRUnichar* aString, PRUint32 aLength,
                                          nsTextDimensions& aDimensions, PRInt32* aFontID)
{
  PRUint32 maxChunkLength = GetMaxChunkLength(this);
  if (aLength <= maxChunkLength)
    return GetTextDimensionsInternal(aString, aLength, aDimensions);
    
  if (aFontID) {
    *aFontID = nsnull;
  }
 
  PRBool firstIteration = PR_TRUE;
  while (aLength > 0) {
    PRInt32 len = FindSafeLength(this, aString, aLength, maxChunkLength);
    nsTextDimensions dimensions;
    nsresult rv = GetTextDimensionsInternal(aString, len, dimensions);
    if (NS_FAILED(rv))
      return rv;
    if (firstIteration) {
      
      
      
      aDimensions = dimensions;
    } else {
      aDimensions.Combine(dimensions);
    }
    aLength -= len;
    aString += len;
    firstIteration = PR_FALSE;
  }
  return NS_OK;  
}

#if defined(_WIN32) || defined(XP_OS2) || defined(MOZ_X11) || defined(XP_BEOS)
NS_IMETHODIMP
nsRenderingContextImpl::GetTextDimensions(const char*       aString,
                                          PRInt32           aLength,
                                          PRInt32           aAvailWidth,
                                          PRInt32*          aBreaks,
                                          PRInt32           aNumBreaks,
                                          nsTextDimensions& aDimensions,
                                          PRInt32&          aNumCharsFit,
                                          nsTextDimensions& aLastWordDimensions,
                                          PRInt32*          aFontID)
{
  PRUint32 maxChunkLength = GetMaxChunkLength(this);
  if (aLength <= PRInt32(maxChunkLength))
    return GetTextDimensionsInternal(aString, aLength, aAvailWidth, aBreaks, aNumBreaks,
                                     aDimensions, aNumCharsFit, aLastWordDimensions, aFontID);

  if (aFontID) {
    *aFontID = 0;
  }
  
  
  PRInt32 x = 0;
  PRInt32 wordCount;
  for (wordCount = 0; wordCount < aNumBreaks; ++wordCount) {
    PRInt32 lastBreak = wordCount > 0 ? aBreaks[wordCount - 1] : 0;
    nsTextDimensions dimensions;
    
    NS_ASSERTION(aBreaks[wordCount] > lastBreak, "Breaks must be monotonically increasing");
    NS_ASSERTION(aBreaks[wordCount] <= aLength, "Breaks can't exceed string length");
   
     

    nsresult rv =
      GetTextDimensions(aString + lastBreak, aBreaks[wordCount] - lastBreak,
                        dimensions);
    if (NS_FAILED(rv))
      return rv;
    x += dimensions.width;
    
    if (x > aAvailWidth && wordCount > 0)
      break;
    
    
    if (wordCount == 0) {
      aDimensions = dimensions;
    } else {
      aDimensions.Combine(aLastWordDimensions);
    }
    aNumCharsFit = aBreaks[wordCount];
    aLastWordDimensions = dimensions;
  }
  
  aDimensions.width = x;
  return NS_OK;
}

NS_IMETHODIMP
nsRenderingContextImpl::GetTextDimensions(const PRUnichar*  aString,
                                          PRInt32           aLength,
                                          PRInt32           aAvailWidth,
                                          PRInt32*          aBreaks,
                                          PRInt32           aNumBreaks,
                                          nsTextDimensions& aDimensions,
                                          PRInt32&          aNumCharsFit,
                                          nsTextDimensions& aLastWordDimensions,
                                          PRInt32*          aFontID)
{
  PRUint32 maxChunkLength = GetMaxChunkLength(this);
  if (aLength <= PRInt32(maxChunkLength))
    return GetTextDimensionsInternal(aString, aLength, aAvailWidth, aBreaks, aNumBreaks,
                                     aDimensions, aNumCharsFit, aLastWordDimensions, aFontID);

  if (aFontID) {
    *aFontID = 0;
  }

  
  PRInt32 x = 0;
  PRInt32 wordCount;
  for (wordCount = 0; wordCount < aNumBreaks; ++wordCount) {
    PRInt32 lastBreak = wordCount > 0 ? aBreaks[wordCount - 1] : 0;
    
    NS_ASSERTION(aBreaks[wordCount] > lastBreak, "Breaks must be monotonically increasing");
    NS_ASSERTION(aBreaks[wordCount] <= aLength, "Breaks can't exceed string length");
    
    nsTextDimensions dimensions;
    
    nsresult rv =
      GetTextDimensions(aString + lastBreak, aBreaks[wordCount] - lastBreak,
                        dimensions);
    if (NS_FAILED(rv))
      return rv;
    x += dimensions.width;
    
    if (x > aAvailWidth && wordCount > 0)
      break;
    
    
    if (wordCount == 0) {
      aDimensions = dimensions;
    } else {
      aDimensions.Combine(aLastWordDimensions);
    }
    aNumCharsFit = aBreaks[wordCount];
    aLastWordDimensions = dimensions;
  }
  
  aDimensions.width = x;
  return NS_OK;
}
#endif

#ifdef MOZ_MATHML
NS_IMETHODIMP
nsRenderingContextImpl::GetBoundingMetrics(const char*        aString,
                                           PRUint32           aLength,
                                           nsBoundingMetrics& aBoundingMetrics)
{
  PRUint32 maxChunkLength = GetMaxChunkLength(this);
  if (aLength <= maxChunkLength)
    return GetBoundingMetricsInternal(aString, aLength, aBoundingMetrics);

  PRBool firstIteration = PR_TRUE;
  while (aLength > 0) {
    PRInt32 len = FindSafeLength(this, aString, aLength, maxChunkLength);
    nsBoundingMetrics metrics;
    nsresult rv = GetBoundingMetricsInternal(aString, len, metrics);
    if (NS_FAILED(rv))
      return rv;
    if (firstIteration) {
      
      
      
      aBoundingMetrics = metrics;
    } else {
      aBoundingMetrics += metrics;
    }
    aLength -= len;
    aString += len;
    firstIteration = PR_FALSE;
  }  
  return NS_OK;
}

NS_IMETHODIMP
nsRenderingContextImpl::GetBoundingMetrics(const PRUnichar*   aString,
                                           PRUint32           aLength,
                                           nsBoundingMetrics& aBoundingMetrics,
                                           PRInt32*           aFontID)
{
  PRUint32 maxChunkLength = GetMaxChunkLength(this);
  if (aLength <= maxChunkLength)
    return GetBoundingMetricsInternal(aString, aLength, aBoundingMetrics, aFontID);

  if (aFontID) {
    *aFontID = 0;
  }

  PRBool firstIteration = PR_TRUE;
  while (aLength > 0) {
    PRInt32 len = FindSafeLength(this, aString, aLength, maxChunkLength);
    nsBoundingMetrics metrics;
    nsresult rv = GetBoundingMetricsInternal(aString, len, metrics);
    if (NS_FAILED(rv))
      return rv;
    if (firstIteration) {
      
      
      
      aBoundingMetrics = metrics;
    } else {
      aBoundingMetrics += metrics;
    }
    aLength -= len;
    aString += len;
    firstIteration = PR_FALSE;
  }  
  return NS_OK;
}
#endif

NS_IMETHODIMP
nsRenderingContextImpl::DrawString(const char *aString, PRUint32 aLength,
                                   nscoord aX, nscoord aY,
                                   const nscoord* aSpacing)
{
  PRUint32 maxChunkLength = GetMaxChunkLength(this);
  while (aLength > 0) {
    PRInt32 len = FindSafeLength(this, aString, aLength, maxChunkLength);
    nsresult rv = DrawStringInternal(aString, len, aX, aY);
    if (NS_FAILED(rv))
      return rv;
    aLength -= len;

    if (aLength > 0) {
      nscoord width;
      rv = GetWidthInternal(aString, len, width);
      if (NS_FAILED(rv))
        return rv;
      aX += width;
      aString += len;
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsRenderingContextImpl::DrawString(const PRUnichar *aString, PRUint32 aLength,
                                   nscoord aX, nscoord aY,
                                   PRInt32 aFontID,
                                   const nscoord* aSpacing)
{
  PRUint32 maxChunkLength = GetMaxChunkLength(this);
  if (aLength <= maxChunkLength) {
    return DrawStringInternal(aString, aLength, aX, aY, aFontID, aSpacing);
  }

  PRBool isRTL = PR_FALSE;
  GetRightToLeftText(&isRTL);

  if (isRTL) {
    nscoord totalWidth = 0;
    if (aSpacing) {
      for (PRUint32 i = 0; i < aLength; ++i) {
        totalWidth += aSpacing[i];
      }
    } else {
      nsresult rv = GetWidth(aString, aLength, totalWidth);
      if (NS_FAILED(rv))
        return rv;
    }
    aX += totalWidth;
  }
  
  while (aLength > 0) {
    PRInt32 len = FindSafeLength(this, aString, aLength, maxChunkLength);
    nscoord width = 0;
    if (aSpacing) {
      for (PRInt32 i = 0; i < len; ++i) {
        width += aSpacing[i];
      }
    } else {
      nsresult rv = GetWidthInternal(aString, len, width);
      if (NS_FAILED(rv))
        return rv;
    }

    if (isRTL) {
      aX -= width;
    }
    nsresult rv = DrawStringInternal(aString, len, aX, aY, aFontID, aSpacing);
    if (NS_FAILED(rv))
      return rv;
    aLength -= len;
    if (!isRTL) {
      aX += width;
    }
    aString += len;
    if (aSpacing) {
      aSpacing += len;
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsRenderingContextImpl::RenderEPS(const nsRect& aRect, FILE *aDataFile)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
