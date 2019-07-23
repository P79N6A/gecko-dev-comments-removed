




































#ifndef nsRenderingContextMac_h___
#define nsRenderingContextMac_h___

#include <QDOffscreen.h>
#include <UnicodeConverter.h>

#include "nsRenderingContextImpl.h"
#include "nsDrawingSurfaceMac.h"
#include "nsUnicodeRenderingToolkit.h"

#include "nsVoidArray.h"

class nsIFontMetrics;
class nsIDeviceContext;
class nsIRegion;
class nsFont;
class nsTransform2D;

class nsGraphicState;
class nsUnicodeFallbackCache;




class nsRenderingContextMac : public nsRenderingContextImpl
{
public:
  nsRenderingContextMac();
  ~nsRenderingContextMac();

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
  NS_IMETHOD FlushRect(const nsRect& aRect);
  NS_IMETHOD FlushRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight);
  NS_IMETHOD DrawPolygon(const nsPoint aPoints[], PRInt32 aNumPoints);
  NS_IMETHOD FillPolygon(const nsPoint aPoints[], PRInt32 aNumPoints);
  NS_IMETHOD DrawEllipse(const nsRect& aRect);
  NS_IMETHOD DrawEllipse(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight);
  NS_IMETHOD FillEllipse(const nsRect& aRect);
  NS_IMETHOD FillEllipse(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight);
  NS_IMETHOD DrawArc(const nsRect& aRect,float aStartAngle, float aEndAngle);
  NS_IMETHOD DrawArc(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight,float aStartAngle, float aEndAngle);
  NS_IMETHOD FillArc(const nsRect& aRect,float aStartAngle, float aEndAngle);
  NS_IMETHOD FillArc(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight,float aStartAngle, float aEndAngle);

  NS_IMETHOD GetWidth(const nsString& aString, nscoord &aWidth,
                      PRInt32 *aFontID = nsnull)
  { return nsRenderingContextImpl::GetWidth(aString, aWidth, aFontID); }
  NS_IMETHOD GetWidth(const char* aString, nscoord& aWidth)
  { return nsRenderingContextImpl::GetWidth(aString, aWidth); }
  NS_IMETHOD GetWidth(const char* aString, PRUint32 aLength,
                      nscoord& aWidth)
  { return nsRenderingContextImpl::GetWidth(aString, aLength, aWidth); }
  NS_IMETHOD GetWidth(const PRUnichar *aString, PRUint32 aLength,
                      nscoord &aWidth, PRInt32 *aFontID = nsnull)
  { return nsRenderingContextImpl::GetWidth(aString, aLength, aWidth, aFontID); }
  NS_IMETHOD DrawString(const nsString& aString, nscoord aX, nscoord aY,
                        PRInt32 aFontID = -1,
                        const nscoord* aSpacing = nsnull)
  { return nsRenderingContextImpl::DrawString(aString, aX, aY, aFontID, aSpacing); }

  NS_IMETHOD GetWidth(char aC, nscoord &aWidth);
  NS_IMETHOD GetWidth(PRUnichar aC, nscoord &aWidth,
                      PRInt32 *aFontID);
  
  NS_IMETHOD GetWidthInternal(const char *aString, PRUint32 aLength, nscoord &aWidth);
  NS_IMETHOD GetWidthInternal(const PRUnichar *aString, PRUint32 aLength, nscoord &aWidth,
                              PRInt32 *aFontID);

  NS_IMETHOD DrawStringInternal(const char *aString, PRUint32 aLength,
                                nscoord aX, nscoord aY,
                                const nscoord* aSpacing);
  NS_IMETHOD DrawStringInternal(const PRUnichar *aString, PRUint32 aLength,
                                nscoord aX, nscoord aY,
                                PRInt32 aFontID,
                                const nscoord* aSpacing);

  NS_IMETHOD GetTextDimensionsInternal(const char* aString, PRUint32 aLength,
                                       nsTextDimensions& aDimensions);
  NS_IMETHOD GetTextDimensionsInternal(const PRUnichar *aString, PRUint32 aLength,
                                       nsTextDimensions& aDimensions,PRInt32 *aFontID);

#ifdef MOZ_MATHML
  


  NS_IMETHOD GetBoundingMetricsInternal(const char*        aString,
                                        PRUint32           aLength,
                                        nsBoundingMetrics& aBoundingMetrics);
  
  


  NS_IMETHOD GetBoundingMetricsInternal(const PRUnichar*   aString,
                                        PRUint32           aLength,
                                        nsBoundingMetrics& aBoundingMetrics,
                                        PRInt32*           aFontID = nsnull);

#endif 

  virtual PRInt32 GetMaxStringLength();
                                 
  NS_IMETHOD CopyOffScreenBits(nsIDrawingSurface* aSrcSurf, PRInt32 aSrcX, PRInt32 aSrcY,
                               const nsRect &aDestBounds, PRUint32 aCopyFlags);

  
  NS_IMETHOD ReleaseBackbuffer(void);
  NS_IMETHOD UseBackbuffer(PRBool* aUseBackbuffer);

  virtual void* GetNativeGraphicData(GraphicDataType aType);

  



  NS_IMETHOD SetRightToLeftText(PRBool aIsRTL);
  NS_IMETHOD GetRightToLeftText(PRBool* aIsRTL);

  NS_IMETHOD DrawImage(imgIContainer *aImage, const nsRect & aSrcRect, const nsRect & aDestRect);
  NS_IMETHOD DrawTile(imgIContainer *aImage,
                      nscoord aXImageStart, nscoord aYImageStart,
                      const nsRect * aTargetRect);

  
  nsresult   SetPortTextState();
  nsresult   Init(nsIDeviceContext* aContext, CGrafPtr aPort);

  
  static PRBool OnTigerOrLater();

protected:
  enum GraphicStateChanges {
		kFontChanged	= (1 << 0),
		kColorChanged	= (1 << 1),
		kClippingChanged = (1 << 2),
		
		kEverythingChanged = 0xFFFFFFFF
	};

	void 			SelectDrawingSurface(nsDrawingSurfaceMac* aSurface, PRUint32 aChanges = kEverythingChanged);
  void      SetupPortState();

protected:
    float                   mP2T;               
    nsIDeviceContext *      mContext;

    nsDrawingSurfaceMac*    mFrontSurface;
    nsDrawingSurfaceMac*    mCurrentSurface;    

    CGrafPtr                mPort;              
    nsGraphicState *        mGS;                
    nsUnicodeRenderingToolkit mUnicodeRenderingToolkit;
    nsAutoVoidArray         mGSStack;           
    PRUint32                mChanges;           
    PRBool                  mRightToLeftText;
};

#endif 
