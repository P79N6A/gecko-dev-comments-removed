




































#include "nsCOMPtr.h"
#include "nsRenderingContextImpl.h"
#include "nsIDeviceContext.h"
#include "nsIImage.h"
#include "nsIRegion.h"
#include "nsIFontMetrics.h"
#include <stdlib.h>






nsRenderingContextImpl :: nsRenderingContextImpl()
{
}





nsRenderingContextImpl :: ~nsRenderingContextImpl()
{


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
