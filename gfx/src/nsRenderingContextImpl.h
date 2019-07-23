




































#ifndef nsRenderingContextImpl_h___
#define nsRenderingContextImpl_h___

#include "gfxCore.h"
#include "nsIRenderingContext.h"
#include "nsPoint.h"
#include "nsSize.h"

#ifdef MOZ_CAIRO_GFX
class gfxContext;
#endif

class nsRenderingContextImpl : public nsIRenderingContext
{



public:
  nsRenderingContextImpl();




  





  virtual PRInt32 GetMaxStringLength() = 0;

  



  NS_IMETHOD GetRightToLeftText(PRBool* aIsRTL);

  
  
  NS_IMETHOD GetWidth(const nsString& aString, nscoord &aWidth,
                      PRInt32 *aFontID = nsnull);
  NS_IMETHOD GetWidth(const char* aString, nscoord& aWidth);
  NS_IMETHOD DrawString(const nsString& aString, nscoord aX, nscoord aY,
                        PRInt32 aFontID = -1,
                        const nscoord* aSpacing = nsnull);

  
  NS_IMETHOD GetWidth(const char* aString, PRUint32 aLength,
                      nscoord& aWidth);
  NS_IMETHOD GetWidth(const PRUnichar *aString, PRUint32 aLength,
                      nscoord &aWidth, PRInt32 *aFontID = nsnull);

  NS_IMETHOD GetTextDimensions(const char* aString, PRUint32 aLength,
                               nsTextDimensions& aDimensions);
  NS_IMETHOD GetTextDimensions(const PRUnichar* aString, PRUint32 aLength,
                               nsTextDimensions& aDimensions, PRInt32* aFontID = nsnull);

#if defined(_WIN32) || defined(XP_OS2) || defined(MOZ_X11) || defined(XP_BEOS)
  NS_IMETHOD GetTextDimensions(const char*       aString,
                               PRInt32           aLength,
                               PRInt32           aAvailWidth,
                               PRInt32*          aBreaks,
                               PRInt32           aNumBreaks,
                               nsTextDimensions& aDimensions,
                               PRInt32&          aNumCharsFit,
                               nsTextDimensions& aLastWordDimensions,
                               PRInt32*          aFontID = nsnull);

  NS_IMETHOD GetTextDimensions(const PRUnichar*  aString,
                               PRInt32           aLength,
                               PRInt32           aAvailWidth,
                               PRInt32*          aBreaks,
                               PRInt32           aNumBreaks,
                               nsTextDimensions& aDimensions,
                               PRInt32&          aNumCharsFit,
                               nsTextDimensions& aLastWordDimensions,
                               PRInt32*          aFontID = nsnull);
#endif
#ifdef MOZ_MATHML
  NS_IMETHOD
  GetBoundingMetrics(const char*        aString,
                     PRUint32           aLength,
                     nsBoundingMetrics& aBoundingMetrics);
  NS_IMETHOD
  GetBoundingMetrics(const PRUnichar*   aString,
                     PRUint32           aLength,
                     nsBoundingMetrics& aBoundingMetrics,
                     PRInt32*           aFontID = nsnull);
#endif
  NS_IMETHOD DrawString(const char *aString, PRUint32 aLength,
                        nscoord aX, nscoord aY,
                        const nscoord* aSpacing = nsnull);
  NS_IMETHOD DrawString(const PRUnichar *aString, PRUint32 aLength,
                        nscoord aX, nscoord aY,
                        PRInt32 aFontID = -1,
                        const nscoord* aSpacing = nsnull);

  
  NS_IMETHOD GetWidthInternal(const char* aString, PRUint32 aLength,
                              nscoord& aWidth)
  { return NS_ERROR_NOT_IMPLEMENTED; }
  NS_IMETHOD GetWidthInternal(const PRUnichar *aString, PRUint32 aLength,
                              nscoord &aWidth, PRInt32 *aFontID = nsnull)
  { return NS_ERROR_NOT_IMPLEMENTED; }

  NS_IMETHOD GetTextDimensionsInternal(const char* aString, PRUint32 aLength,
                                       nsTextDimensions& aDimensions)
  { return NS_ERROR_NOT_IMPLEMENTED; }
  NS_IMETHOD GetTextDimensionsInternal(const PRUnichar* aString, PRUint32 aLength,
                                       nsTextDimensions& aDimensions, PRInt32* aFontID = nsnull)
  { return NS_ERROR_NOT_IMPLEMENTED; }

#if defined(_WIN32) || defined(XP_OS2) || defined(MOZ_X11) || defined(XP_BEOS)
  NS_IMETHOD GetTextDimensionsInternal(const char*       aString,
                                       PRInt32           aLength,
                                       PRInt32           aAvailWidth,
                                       PRInt32*          aBreaks,
                                       PRInt32           aNumBreaks,
                                       nsTextDimensions& aDimensions,
                                       PRInt32&          aNumCharsFit,
                                       nsTextDimensions& aLastWordDimensions,
                                       PRInt32*          aFontID = nsnull)
  { return NS_ERROR_NOT_IMPLEMENTED; }

  NS_IMETHOD GetTextDimensionsInternal(const PRUnichar*  aString,
                                       PRInt32           aLength,
                                       PRInt32           aAvailWidth,
                                       PRInt32*          aBreaks,
                                       PRInt32           aNumBreaks,
                                       nsTextDimensions& aDimensions,
                                       PRInt32&          aNumCharsFit,
                                       nsTextDimensions& aLastWordDimensions,
                                       PRInt32*          aFontID = nsnull)
  { return NS_ERROR_NOT_IMPLEMENTED; }
#endif
#ifdef MOZ_MATHML
  NS_IMETHOD
  GetBoundingMetricsInternal(const char*        aString,
                             PRUint32           aLength,
                             nsBoundingMetrics& aBoundingMetrics)
  { return NS_ERROR_NOT_IMPLEMENTED; }
  NS_IMETHOD
  GetBoundingMetricsInternal(const PRUnichar*   aString,
                             PRUint32           aLength,
                             nsBoundingMetrics& aBoundingMetrics,
                             PRInt32*           aFontID = nsnull)
  { return NS_ERROR_NOT_IMPLEMENTED; }
#endif
  NS_IMETHOD DrawStringInternal(const char *aString, PRUint32 aLength,
                                nscoord aX, nscoord aY,
                                const nscoord* aSpacing = nsnull)
  { return NS_ERROR_NOT_IMPLEMENTED; }
  NS_IMETHOD DrawStringInternal(const PRUnichar *aString, PRUint32 aLength,
                                nscoord aX, nscoord aY,
                                PRInt32 aFontID = -1,
                                const nscoord* aSpacing = nsnull)
  { return NS_ERROR_NOT_IMPLEMENTED; }

#ifdef MOZ_CAIRO_GFX
  NS_IMETHOD Init(nsIDeviceContext* aContext, gfxASurface* aThebesSurface) { return NS_ERROR_NOT_IMPLEMENTED; }
  NS_IMETHOD Init(nsIDeviceContext* aContext, gfxContext* aThebesContext) { return NS_ERROR_NOT_IMPLEMENTED; }
#endif

protected:
  virtual ~nsRenderingContextImpl();

};

#endif 
