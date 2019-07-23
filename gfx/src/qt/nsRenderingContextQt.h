







































#ifndef nsRenderingContextQt_h___
#define nsRenderingContextQt_h___

#include "nsRenderingContextImpl.h"
#include "nsUnitConversion.h"
#include "nsFont.h"
#include "nsIFontMetrics.h"
#include "nsPoint.h"
#include "nsString.h"
#include "nsCRT.h"
#include "nsTransform2D.h"
#include "nsIWidget.h"
#include "nsRect.h"
#include "nsIDeviceContext.h"
#include "nsVoidArray.h"
#include "nsCOMPtr.h"

#include "nsRegionQt.h"
#include "nsDrawingSurfaceQt.h"
#include "prlog.h"

class nsFontQt;
class nsDrawingSurface;

class nsRenderingContextQt : public nsRenderingContextImpl
{
public:
    nsRenderingContextQt();
    virtual ~nsRenderingContextQt();

    NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

    NS_DECL_ISUPPORTS

    NS_IMETHOD Init(nsIDeviceContext* aContext, nsIWidget *aWindow);
    NS_IMETHOD Init(nsIDeviceContext* aContext, nsIDrawingSurface* aSurface);

    NS_IMETHOD Reset(void);

    NS_IMETHOD GetDeviceContext(nsIDeviceContext *&aContext);

    NS_IMETHOD LockDrawingSurface(PRInt32 aX,PRInt32 aY,PRUint32 aWidth,
                                  PRUint32 aHeight,void **aBits,
                                  PRInt32 *aStride,PRInt32 *aWidthBytes,
                                  PRUint32 aFlags);
    NS_IMETHOD UnlockDrawingSurface(void);

    NS_IMETHOD SelectOffScreenDrawingSurface(nsIDrawingSurface* aSurface);
    NS_IMETHOD GetDrawingSurface(nsIDrawingSurface* *aSurface);
    NS_IMETHOD GetHints(PRUint32& aResult);

    NS_IMETHOD PushState(void);
    NS_IMETHOD PopState(void);

    NS_IMETHOD IsVisibleRect(const nsRect& aRect, PRBool &aVisible);

    NS_IMETHOD SetClipRect(const nsRect& aRect, nsClipCombine aCombine);
    NS_IMETHOD CopyClipRegion(nsIRegion &aRegion);
    NS_IMETHOD GetClipRect(nsRect &aRect, PRBool &aClipValid);
    NS_IMETHOD SetClipRegion(const nsIRegion& aRegion, nsClipCombine aCombine);
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

    NS_IMETHOD CreateDrawingSurface(const nsRect& aBounds,PRUint32 aSurfFlags,
                                    nsIDrawingSurface* &aSurface);
    NS_IMETHOD DestroyDrawingSurface(nsIDrawingSurface* aDS);

    NS_IMETHOD DrawLine(nscoord aX0, nscoord aY0, nscoord aX1, nscoord aY1);
    NS_IMETHOD DrawStdLine(nscoord aX0,nscoord aY0,nscoord aX1,nscoord aY1);
    NS_IMETHOD DrawPolyline(const nsPoint aPoints[], PRInt32 aNumPoints);

    NS_IMETHOD DrawRect(const nsRect& aRect);
    NS_IMETHOD DrawRect(nscoord aX,nscoord aY,nscoord aWidth,nscoord aHeight);
    NS_IMETHOD FillRect(const nsRect& aRect);
    NS_IMETHOD FillRect(nscoord aX,nscoord aY,nscoord aWidth,nscoord aHeight);
    NS_IMETHOD InvertRect(const nsRect& aRect);
    NS_IMETHOD InvertRect(nscoord aX,nscoord aY,nscoord aWidth,nscoord aHeight);

    NS_IMETHOD DrawPolygon(const nsPoint aPoints[], PRInt32 aNumPoints);
    NS_IMETHOD FillPolygon(const nsPoint aPoints[], PRInt32 aNumPoints);

    NS_IMETHOD DrawEllipse(const nsRect& aRect);
    NS_IMETHOD DrawEllipse(nscoord aX,nscoord aY,nscoord aWidth,
                           nscoord aHeight);
    NS_IMETHOD FillEllipse(const nsRect& aRect);
    NS_IMETHOD FillEllipse(nscoord aX,nscoord aY,nscoord aWidth,
                           nscoord aHeight);

    NS_IMETHOD DrawArc(const nsRect& aRect,float aStartAngle,float aEndAngle);
    NS_IMETHOD DrawArc(nscoord aX,nscoord aY,nscoord aWidth,nscoord aHeight,
                       float aStartAngle,float aEndAngle);
    NS_IMETHOD FillArc(const nsRect& aRect,float aStartAngle,float aEndAngle);
    NS_IMETHOD FillArc(nscoord aX,nscoord aY,nscoord aWidth,nscoord aHeight,
                       float aStartAngle,float aEndAngle);

    NS_IMETHOD GetWidth(char aC, nscoord &aWidth);
    NS_IMETHOD GetWidth(PRUnichar aC, nscoord &aWidth, PRInt32 *aFontID);
    NS_IMETHOD GetWidth(const nsString& aString,nscoord &aWidth,
                        PRInt32 *aFontID);
    NS_IMETHOD GetWidth(const char *aString, nscoord &aWidth);
    NS_IMETHOD GetWidth(const char *aString,PRUint32 aLength,nscoord &aWidth);
    NS_IMETHOD GetWidth(const PRUnichar *aString,PRUint32 aLength,
                        nscoord &aWidth,PRInt32 *aFontID);

    NS_IMETHOD GetTextDimensions(const char* aString,
                                 PRUint32 aLength,
                                 nsTextDimensions& aDimensions);

    NS_IMETHOD GetTextDimensions(const PRUnichar *aString,
                                 PRUint32 aLength,
                                 nsTextDimensions& aDimensions,
                                 PRInt32 *aFontID);

    NS_IMETHOD GetTextDimensions(const char*       aString,
                                 PRInt32           aLength,
                                 PRInt32           aAvailWidth,
                                 PRInt32*          aBreaks,
                                 PRInt32           aNumBreaks,
                                 nsTextDimensions& aDimensions,
                                 PRInt32&          aNumCharsFit,
                                 nsTextDimensions& aLastWordDimensions,
                                 PRInt32*          aFontID);

    NS_IMETHOD GetTextDimensions(const PRUnichar*  aString,
                                 PRInt32           aLength,
                                 PRInt32           aAvailWidth,
                                 PRInt32*          aBreaks,
                                 PRInt32           aNumBreaks,
                                 nsTextDimensions& aDimensions,
                                 PRInt32&          aNumCharsFit,
                                 nsTextDimensions& aLastWordDimensions,
                                 PRInt32*          aFontID);

#ifdef MOZ_MATHML
    
    NS_IMETHOD GetBoundingMetrics(const char *aString,PRUint32 aLength,
                                  nsBoundingMetrics& aBoundingMetrics);

    
    NS_IMETHOD GetBoundingMetrics(const PRUnichar *aString,PRUint32 aLength,
                                  nsBoundingMetrics& aBoundingMetrics,
                                  PRInt32 *aFontID = nsnull);
#endif 

    NS_IMETHOD DrawString(const char *aString,PRUint32 aLength,
                          nscoord aX,nscoord aY,const nscoord* aSpacing);
    NS_IMETHOD DrawString(const PRUnichar *aString,PRUint32 aLength,
                          nscoord aX,nscoord aY,PRInt32 aFontID,
                          const nscoord* aSpacing);
    NS_IMETHOD DrawString(const nsString& aString,nscoord aX,nscoord aY,
                          PRInt32 aFontID,const nscoord* aSpacing);

    NS_IMETHOD CopyOffScreenBits(nsIDrawingSurface* aSrcSurf,
                                 PRInt32 aSrcX, PRInt32 aSrcY,
                                 const nsRect &aDestBounds,
                                 PRUint32 aCopyFlags);

    
    NS_IMETHOD CommonInit();
    void       UpdateGC();
    void       ConditionRect(nscoord &x, nscoord &y, nscoord &w, nscoord &h);
    void       CreateClipRegion();

    QPainter *painter() const { return mSurface->GetGC(); }
    QPaintDevice *paintDevice() const { return mSurface->GetPaintDevice(); }
    nsTransform2D *matrix() const { return mTranMatrix; }

    QRect qRect(const nsRect &aRect) {
        int x = aRect.x;
        int y = aRect.y;
        int w = aRect.width;
        int h = aRect.height;
        mTranMatrix->TransformCoord(&x,&y,&w,&h);
        return QRect (x, y, w, h);
    }

protected:
    nsDrawingSurfaceQt   *mOffscreenSurface;
    nsDrawingSurfaceQt   *mSurface;
    nsIDeviceContext     *mContext;
    nsIFontMetrics       *mFontMetrics;
    nsCOMPtr<nsIRegion>  mClipRegion;
    float                mP2T;

    
    nsVoidArray          mStateCache;

    nscolor              mCurrentColor;
    nsLineStyle          mCurrentLineStyle;

    nsFontQt             *mCurrentFont;
    Qt::PenStyle	 mQLineStyle;
    Qt::RasterOp	 mFunction;
};

#endif 
