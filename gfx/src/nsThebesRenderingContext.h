





































#ifndef NSTHEBESRENDERINGCONTEXT__H__
#define NSTHEBESRENDERINGCONTEXT__H__

#include "nsCOMPtr.h"
#include "nsTArray.h"
#include "nsIRenderingContext.h"
#include "nsIDeviceContext.h"
#include "nsIFontMetrics.h"
#include "nsIWidget.h"
#include "nsPoint.h"
#include "nsSize.h"
#include "nsColor.h"
#include "nsRect.h"
#include "nsIRegion.h"
#include "nsTransform2D.h"
#include "nsIThebesFontMetrics.h"
#include "gfxContext.h"

class nsThebesRenderingContext : public nsIRenderingContext
{
public:
    nsThebesRenderingContext();
    virtual ~nsThebesRenderingContext();

    NS_DECL_ISUPPORTS


    



    virtual PRInt32 GetMaxStringLength();

    
    
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
    NS_IMETHOD GetWidth(char aC, nscoord &aWidth);
    NS_IMETHOD GetWidth(PRUnichar aC, nscoord &aWidth,
                        PRInt32 *aFontID);

    NS_IMETHOD GetTextDimensions(const char* aString, PRUint32 aLength,
                                 nsTextDimensions& aDimensions);
    NS_IMETHOD GetTextDimensions(const PRUnichar* aString, PRUint32 aLength,
                                 nsTextDimensions& aDimensions, PRInt32* aFontID = nsnull);

#if defined(_WIN32) || defined(XP_OS2) || defined(MOZ_X11)
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
    NS_IMETHOD GetBoundingMetrics(const char*        aString,
                                  PRUint32           aLength,
                                  nsBoundingMetrics& aBoundingMetrics);
    NS_IMETHOD GetBoundingMetrics(const PRUnichar*   aString,
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

    NS_IMETHOD Init(nsIDeviceContext* aContext, gfxASurface* aThebesSurface);
    NS_IMETHOD Init(nsIDeviceContext* aContext, gfxContext* aThebesContext);
    NS_IMETHOD Init(nsIDeviceContext* aContext, nsIWidget *aWidget);
    NS_IMETHOD CommonInit(void);
    virtual already_AddRefed<nsIDeviceContext> GetDeviceContext();
    NS_IMETHOD PushState(void);
    NS_IMETHOD PopState(void);
    NS_IMETHOD SetClipRect(const nsRect& aRect, nsClipCombine aCombine);
    NS_IMETHOD SetLineStyle(nsLineStyle aLineStyle);
    NS_IMETHOD SetClipRegion(const nsIntRegion& aRegion, nsClipCombine aCombine);
    NS_IMETHOD SetColor(nscolor aColor);
    NS_IMETHOD GetColor(nscolor &aColor) const;
    NS_IMETHOD SetFont(const nsFont& aFont, nsIAtom* aLanguage,
                       gfxUserFontSet *aUserFontSet);
    NS_IMETHOD SetFont(const nsFont& aFont,
                       gfxUserFontSet *aUserFontSet);
    NS_IMETHOD SetFont(nsIFontMetrics *aFontMetrics);
    virtual already_AddRefed<nsIFontMetrics> GetFontMetrics();
    NS_IMETHOD Translate(const nsPoint& aPt);
    NS_IMETHOD Scale(float aSx, float aSy);
    virtual nsTransform2D* GetCurrentTransform();

    NS_IMETHOD DrawLine(const nsPoint& aStartPt, const nsPoint& aEndPt);
    NS_IMETHOD DrawLine(nscoord aX0, nscoord aY0, nscoord aX1, nscoord aY1);
    NS_IMETHOD DrawRect(const nsRect& aRect);
    NS_IMETHOD DrawRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight);
    NS_IMETHOD FillRect(const nsRect& aRect);
    NS_IMETHOD FillRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight);
    NS_IMETHOD InvertRect(const nsRect& aRect);
    NS_IMETHOD InvertRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight);
    NS_IMETHOD FillPolygon(const nsPoint aPoints[], PRInt32 aNumPoints);
    NS_IMETHOD DrawEllipse(const nsRect& aRect);
    NS_IMETHOD DrawEllipse(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight);
    NS_IMETHOD FillEllipse(const nsRect& aRect);
    NS_IMETHOD FillEllipse(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight);

    NS_IMETHOD PushFilter(const nsRect& aRect, PRBool aAreaIsOpaque, float aOpacity);
    NS_IMETHOD PopFilter();

    virtual void* GetNativeGraphicData(GraphicDataType aType);

    NS_IMETHOD PushTranslation(PushedTranslation* aState);
    NS_IMETHOD PopTranslation(PushedTranslation* aState);
    NS_IMETHOD SetTranslation(const nsPoint& aPoint);

    



    NS_IMETHOD SetRightToLeftText(PRBool aIsRTL);
    NS_IMETHOD GetRightToLeftText(PRBool* aIsRTL);
    virtual void SetTextRunRTL(PRBool aIsRTL);

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

    NS_IMETHOD RenderEPS(const nsRect& aRect, FILE *aDataFile);

    

    gfxContext *ThebesContext() { return mThebes; }

    nsTransform2D& CurrentTransform();

    void TransformCoord (nscoord *aX, nscoord *aY);

protected:
    nsresult GetWidthInternal(const char *aString, PRUint32 aLength, nscoord &aWidth);
    nsresult GetWidthInternal(const PRUnichar *aString, PRUint32 aLength, nscoord &aWidth,
                              PRInt32 *aFontID = nsnull);

    nsresult DrawStringInternal(const char *aString, PRUint32 aLength,
                                nscoord aX, nscoord aY,
                                const nscoord* aSpacing = nsnull);
    nsresult DrawStringInternal(const PRUnichar *aString, PRUint32 aLength,
                                nscoord aX, nscoord aY,
                                PRInt32 aFontID = -1,
                                const nscoord* aSpacing = nsnull);

    nsresult GetTextDimensionsInternal(const char*       aString,
                                       PRUint32          aLength,
                                       nsTextDimensions& aDimensions);
    nsresult GetTextDimensionsInternal(const PRUnichar*  aString,
                                       PRUint32          aLength,
                                       nsTextDimensions& aDimensions,
                                       PRInt32*          aFontID = nsnull);
    nsresult GetTextDimensionsInternal(const char*       aString,
                                       PRInt32           aLength,
                                       PRInt32           aAvailWidth,
                                       PRInt32*          aBreaks,
                                       PRInt32           aNumBreaks,
                                       nsTextDimensions& aDimensions,
                                       PRInt32&          aNumCharsFit,
                                       nsTextDimensions& aLastWordDimensions,
                                       PRInt32*          aFontID = nsnull);
    nsresult GetTextDimensionsInternal(const PRUnichar*  aString,
                                       PRInt32           aLength,
                                       PRInt32           aAvailWidth,
                                       PRInt32*          aBreaks,
                                       PRInt32           aNumBreaks,
                                       nsTextDimensions& aDimensions,
                                       PRInt32&          aNumCharsFit,
                                       nsTextDimensions& aLastWordDimensions,
                                       PRInt32*          aFontID = nsnull);

#ifdef MOZ_MATHML
    


    nsresult GetBoundingMetricsInternal(const char*        aString,
                                        PRUint32           aLength,
                                        nsBoundingMetrics& aBoundingMetrics);

    


    nsresult GetBoundingMetricsInternal(const PRUnichar*   aString,
                                        PRUint32           aLength,
                                        nsBoundingMetrics& aBoundingMetrics,
                                        PRInt32*           aFontID = nsnull);

#endif 

    nsCOMPtr<nsIDeviceContext> mDeviceContext;
    
    double mP2A;

    nsCOMPtr<nsIWidget> mWidget;

    
    
    nsCOMPtr<nsIThebesFontMetrics> mFontMetrics;

    nsLineStyle mLineStyle;
    nscolor mColor;

    
    nsRefPtr<gfxContext> mThebes;

    
    void UpdateTempTransformMatrix();
    nsTransform2D mTempTransform;

    
    nsTArray<float> mOpacityArray;
};

#endif  
