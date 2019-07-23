







































#ifndef nsRenderingContextXlib_h___
#define nsRenderingContextXlib_h___

#include "nsIDeviceContext.h"
#include "nsIWidget.h"
#include "nsRenderingContextImpl.h"
#include "nsUnitConversion.h"
#include "nsFont.h"
#include "nsFontMetricsXlib.h"
#include "nsPoint.h"
#include "nsString.h"
#include "nsCRT.h"
#include "nsTransform2D.h"
#include "nsRect.h"
#include "nsVoidArray.h"
#include "nsGfxCIID.h"
#include "nsDrawingSurfaceXlib.h"
#include "nsRegionXlib.h"

class nsFontXlib;

class nsRenderingContextXlibContext
{
public:
  nsGCCacheXlib mGcCache;
};


class nsRenderingContextXlib : public nsRenderingContextImpl
{
public:
  nsRenderingContextXlib();
  virtual ~nsRenderingContextXlib();

  NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

  NS_DECL_ISUPPORTS

  NS_IMETHOD Init(nsIDeviceContext* aContext, nsIWidget *aWindow);
  NS_IMETHOD Init(nsIDeviceContext* aContext, nsIDrawingSurface* aSurface);

  NS_IMETHOD Reset(void);

  NS_IMETHOD GetDeviceContext(nsIDeviceContext *&aContext);

  NS_IMETHOD LockDrawingSurface(PRInt32 aX, PRInt32 aY, PRUint32 aWidth, PRUint32 aHeight,
                                void **aBits, PRInt32 *aStride, PRInt32 *aWidthBytes,
                                PRUint32 aFlags);
  NS_IMETHOD UnlockDrawingSurface(void);

  NS_IMETHOD SelectOffScreenDrawingSurface(nsIDrawingSurface* aSurface);
  NS_IMETHOD GetDrawingSurface(nsIDrawingSurface* *aSurface);
  NS_IMETHOD GetHints(PRUint32& aResult);

  NS_IMETHOD PushState(void);
  NS_IMETHOD PopState(void);

  NS_IMETHOD IsVisibleRect(const nsRect& aRect, PRBool &aVisible);

  NS_IMETHOD SetClipRect(const nsRect& aRect, nsClipCombine aCombine);
  NS_IMETHOD GetClipRect(nsRect &aRect, PRBool &aClipValid);
  NS_IMETHOD SetClipRegion(const nsIRegion& aRegion, nsClipCombine aCombine);
  NS_IMETHOD CopyClipRegion(nsIRegion &aRegion);
  NS_IMETHOD GetClipRegion(nsIRegion **aRegion);

  NS_IMETHOD SetLineStyle(nsLineStyle aLineStyle);
  NS_IMETHOD GetLineStyle(nsLineStyle &aLineStyle);

  NS_IMETHOD SetColor(nscolor aColor);
  NS_IMETHOD GetColor(nscolor &aColor) const;

  NS_IMETHOD SetFont(const nsFont& aFont, nsIAtom* aLangGroup);
  NS_IMETHOD SetFont(nsIFontMetrics *aFontMetrics);

  NS_IMETHOD GetFontMetrics(nsIFontMetrics *&aFontMetrics);

  NS_IMETHOD Translate(nscoord aX, nscoord aY);
  NS_IMETHOD Scale(float aSx, float aSy);
  NS_IMETHOD GetCurrentTransform(nsTransform2D *&aTransform);

  NS_IMETHOD CreateDrawingSurface(const nsRect& aBounds, PRUint32 aSurfFlags, nsIDrawingSurface* &aSurface);
  NS_IMETHOD DestroyDrawingSurface(nsIDrawingSurface* aDS);

  NS_IMETHOD DrawLine(nscoord aX0, nscoord aY0, nscoord aX1, nscoord aY1);
  NS_IMETHOD DrawPolyline(const nsPoint aPoints[], PRInt32 aNumPoints);

  NS_IMETHOD DrawRect(const nsRect& aRect);
  NS_IMETHOD DrawRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight);

  NS_IMETHOD FillRect(const nsRect& aRect);
  NS_IMETHOD FillRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight);

  NS_IMETHOD InvertRect(const nsRect& aRect);
  NS_IMETHOD InvertRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight);

  NS_IMETHOD DrawPolygon(const nsPoint aPoints[], PRInt32 aNumPoints);
  NS_IMETHOD FillPolygon(const nsPoint aPoints[], PRInt32 aNumPoints);

  NS_IMETHOD DrawEllipse(const nsRect& aRect);
  NS_IMETHOD DrawEllipse(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight);
  NS_IMETHOD FillEllipse(const nsRect& aRect);
  NS_IMETHOD FillEllipse(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight);

  NS_IMETHOD DrawArc(const nsRect& aRect,
                     float aStartAngle, float aEndAngle);
  NS_IMETHOD DrawArc(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight,
                     float aStartAngle, float aEndAngle);
  NS_IMETHOD FillArc(const nsRect& aRect,
                     float aStartAngle, float aEndAngle);
  NS_IMETHOD FillArc(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight,
                     float aStartAngle, float aEndAngle);

  NS_IMETHOD GetWidth(char aC, nscoord &aWidth);
  NS_IMETHOD GetWidth(PRUnichar aC, nscoord &aWidth,
                      PRInt32 *aFontID);
  NS_IMETHOD GetWidth(const nsString& aString, nscoord &aWidth,
                      PRInt32 *aFontID);
  NS_IMETHOD GetWidth(const char *aString, nscoord &aWidth);
  NS_IMETHOD GetWidth(const char *aString, PRUint32 aLength, nscoord &aWidth);
  NS_IMETHOD GetWidth(const PRUnichar *aString, PRUint32 aLength, nscoord &aWidth,
                      PRInt32 *aFontID);

  NS_IMETHOD DrawString(const char *aString, PRUint32 aLength,
                        nscoord aX, nscoord aY,
                        const nscoord* aSpacing);
  NS_IMETHOD DrawString(const PRUnichar *aString, PRUint32 aLength,
                        nscoord aX, nscoord aY,
                        PRInt32 aFontID,
                        const nscoord* aSpacing);
  NS_IMETHOD DrawString(const nsString& aString, nscoord aX, nscoord aY,
                        PRInt32 aFontID,
                        const nscoord* aSpacing);

  NS_IMETHOD GetTextDimensions(const char* aString, PRUint32 aLength,
                               nsTextDimensions& aDimensions);
  NS_IMETHOD GetTextDimensions(const PRUnichar *aString, PRUint32 aLength,
                               nsTextDimensions& aDimensions, PRInt32 *aFontID);
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

  NS_IMETHOD CopyOffScreenBits(nsIDrawingSurface* aSrcSurf, PRInt32 aSrcX, PRInt32 aSrcY,
                               const nsRect &aDestBounds, PRUint32 aCopyFlags);

  NS_IMETHOD DrawImage(imgIContainer *aImage, const nsRect & aSrcRect, const nsRect & aDestRect);

  NS_IMETHOD GetBackbuffer(const nsRect &aRequestedSize, const nsRect &aMaxSize,
                           PRBool aForBlending, nsIDrawingSurface* &aBackbuffer);
  NS_IMETHOD ReleaseBackbuffer(void);

#ifdef MOZ_MATHML
  


  NS_IMETHOD GetBoundingMetrics(const char*        aString,
                                PRUint32           aLength,
                                nsBoundingMetrics& aBoundingMetrics);
  
  


  NS_IMETHOD GetBoundingMetrics(const PRUnichar*   aString,
                                PRUint32           aLength,
                                nsBoundingMetrics& aBoundingMetrics,
                                PRInt32*           aFontID = nsnull);

#endif 

  void SetClipRectInPixels(const nsRect& aRect, nsClipCombine aCombine);

  xGC *GetGC() { mGC->AddRef(); return mGC; }
  void UpdateGC();
  
  
  void SetCurrentFont(nsFontXlib *cf) { mCurrentFont = cf; };
  nsFontXlib *GetCurrentFont() { return mCurrentFont; };

protected:
  
  nsresult CommonInit(void);

  nsCOMPtr<nsIDeviceContext>      mContext;
  nsCOMPtr<nsIDrawingSurfaceXlib> mOffscreenSurface;
  nsCOMPtr<nsIDrawingSurfaceXlib> mSurface;
  nsCOMPtr<nsIFontMetrics>        mFontMetrics;
  nsCOMPtr<nsIRegion>             mClipRegion;
  float                           mP2T;

 
  nsAutoVoidArray          mStateCache;

  xGC                     *mGC;
  int                      mFunction;
  int                      mLineStyle;
  char                    *mDashList;
  int                      mDashes;
  nscolor                  mCurrentColor;
  XlibRgbHandle           *mXlibRgbHandle; 
  Display                 *mDisplay;

  nsFontXlib              *mCurrentFont;
  nsLineStyle              mCurrentLineStyle;

  
  
  void ConditionRect(nscoord &x, nscoord &y, nscoord &w, nscoord &h) {
    if ( y < -32766 ) {
      y = -32766;
    }

    if ( y + h > 32766 ) {
      h  = 32766 - y;
    }

    if ( x < -32766 ) {
      x = -32766;
    }

    if ( x + w > 32766 ) {
      w  = 32766 - x;
    }
  }
};


nsresult CreateRenderingContextXlibContext(nsIDeviceContext *aDevice, nsRenderingContextXlibContext **aContext);
void DeleteRenderingContextXlibContext(nsRenderingContextXlibContext *aContext);

#endif 
