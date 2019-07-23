




































#ifndef nsRenderingContextImpl_h___
#define nsRenderingContextImpl_h___

#include "gfxCore.h"
#include "nsIRenderingContext.h"
#include "nsPoint.h"
#include "nsSize.h"

#ifdef MOZ_CAIRO_GFX
class gfxContext;
#endif

typedef struct {	
    double x;	  
    double dx;	
    int i;	    
} Edge;

class nsRenderingContextImpl : public nsIRenderingContext
{


public:


protected:
  nsTransform2D		  *mTranMatrix;				
  int               mAct;		        		
  Edge              *mActive;	      		

public:
  nsRenderingContextImpl();




  
  NS_IMETHOD FlushRect(const nsRect& aRect);
  NS_IMETHOD FlushRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight);

  




  NS_IMETHOD GetPenMode(nsPenMode &aPenMode) { return NS_ERROR_FAILURE;}

  virtual void* GetNativeGraphicData(nsIRenderingContext::GraphicDataType aType)
  { return nsnull; }

  




  NS_IMETHOD SetPenMode(nsPenMode aPenMode) { return NS_ERROR_FAILURE;}
  
  NS_IMETHOD PushTranslation(PushedTranslation* aState);
  NS_IMETHOD PopTranslation(PushedTranslation* aState);
  NS_IMETHOD SetTranslation(nscoord aX, nscoord aY);

  





  virtual PRInt32 GetMaxStringLength() { return 1; }

  



  NS_IMETHOD SetRightToLeftText(PRBool aIsRTL);
  NS_IMETHOD GetRightToLeftText(PRBool* aIsRTL);

#ifndef MOZ_CAIRO_GFX
  NS_IMETHOD DrawImage(imgIContainer *aImage, const nsRect & aSrcRect, const nsRect & aDestRect);
#endif
  NS_IMETHOD DrawTile(imgIContainer *aImage, nscoord aXOffset, nscoord aYOffset, const nsRect * aTargetRect);

  NS_IMETHOD GetClusterInfo(const PRUnichar *aText,
                            PRUint32 aLength,
                            PRUint8 *aClusterStarts);
  virtual PRInt32 GetPosition(const PRUnichar *aText,
                              PRUint32 aLength,
                              nsPoint aPt);

  NS_IMETHOD GetRangeWidth(const PRUnichar *aText,
                           PRUint32 aLength,
                           PRUint32 aStart,
                           PRUint32 aEnd,
                           PRUint32 &aWidth);
  NS_IMETHOD GetRangeWidth(const char *aText,
                           PRUint32 aLength,
                           PRUint32 aStart,
                           PRUint32 aEnd,
                           PRUint32 &aWidth);

  
  NS_IMETHOD GetWidth(char aC, nscoord &aWidth) = 0;
  NS_IMETHOD GetWidth(PRUnichar aC, nscoord &aWidth,
                      PRInt32 *aFontID = nsnull) = 0;

  
  
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

  NS_IMETHOD RenderEPS(const nsRect& aRect, FILE *aDataFile);

#ifdef MOZ_CAIRO_GFX
  NS_IMETHOD Init(nsIDeviceContext* aContext, gfxASurface* aThebesSurface) { return NS_ERROR_NOT_IMPLEMENTED; }
  NS_IMETHOD Init(nsIDeviceContext* aContext, gfxContext* aThebesContext) { return NS_ERROR_NOT_IMPLEMENTED; }
#endif

protected:
  virtual ~nsRenderingContextImpl();

  








  PRBool RectFitsInside(const nsRect& aRect, PRInt32 aWidth, PRInt32 aHeight) const;

  








  PRBool BothRectsFitInside(const nsRect& aRect1, const nsRect& aRect2, PRInt32 aWidth, PRInt32 aHeight, nsRect& aNewSize) const;

  








  void CalculateDiscreteSurfaceSize(const nsRect& aMaxBackbufferSize, const nsRect& aRequestedSize, nsRect& aSize);

  






  void GetDrawingSurfaceSize(const nsRect& aMaxBackbufferSize, const nsRect& aRequestedSize, nsRect& aSurfaceSize);

public:

protected:
  nsPenMode   mPenMode;
private:
  static nsIDrawingSurface*  gBackbuffer;         
    
  static nsSize            gLargestRequestedSize;

};

#endif 
