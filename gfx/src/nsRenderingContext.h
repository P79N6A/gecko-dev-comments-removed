





































#ifndef NSRENDERINGCONTEXT__H__
#define NSRENDERINGCONTEXT__H__

#include "nsCOMPtr.h"
#include "nsTArray.h"
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

typedef enum {
    nsClipCombine_kIntersect = 0,
    nsClipCombine_kUnion = 1,
    nsClipCombine_kSubtract = 2,
    nsClipCombine_kReplace = 3
} nsClipCombine;

typedef enum {
    nsLineStyle_kNone   = 0,
    nsLineStyle_kSolid  = 1,
    nsLineStyle_kDashed = 2,
    nsLineStyle_kDotted = 3
} nsLineStyle;





struct nsTextDimensions {
    
    nscoord ascent;

    
    nscoord descent;

    
    nscoord width;


    nsTextDimensions()
    {
        Clear();
    }

    
    void
    Clear() {
        ascent = descent = width = 0;
    }

    
    void
    Combine(const nsTextDimensions& aOther) {
        if (ascent < aOther.ascent) ascent = aOther.ascent;
        if (descent < aOther.descent) descent = aOther.descent;
        width += aOther.width;
    }
};

#ifdef MOZ_MATHML



struct nsBoundingMetrics {

    
    

    
    
    
    
    
    
    

    
    
    
    

    
    

    nscoord leftBearing;
    


    nscoord rightBearing;
    




    nscoord ascent;
    


    nscoord descent;
    




    
    

    nscoord width;
    




    nsBoundingMetrics() {
        Clear();
    }

    
    

    
    void
    Clear() {
        leftBearing = rightBearing = 0;
        ascent = descent = width = 0;
    }

    
    void
    operator += (const nsBoundingMetrics& bm) {
        if (ascent + descent == 0 && rightBearing - leftBearing == 0) {
            ascent = bm.ascent;
            descent = bm.descent;
            leftBearing = width + bm.leftBearing;
            rightBearing = width + bm.rightBearing;
        }
        else {
            if (ascent < bm.ascent) ascent = bm.ascent;
            if (descent < bm.descent) descent = bm.descent;
            leftBearing = PR_MIN(leftBearing, width + bm.leftBearing);
            rightBearing = PR_MAX(rightBearing, width + bm.rightBearing);
        }
        width += bm.width;
    }
};
#endif 


class nsRenderingContext
{
public:
    nsRenderingContext()
        : mP2A(0.), mLineStyle(nsLineStyle_kNone), mColor(NS_RGB(0,0,0))
    {}
    

    NS_INLINE_DECL_REFCOUNTING(nsRenderingContext)

    



    PRInt32 GetMaxStringLength();

    
    
    nsresult GetWidth(const nsString& aString, nscoord &aWidth,
                      PRInt32 *aFontID = nsnull);
    nsresult GetWidth(const char* aString, nscoord& aWidth);
    nsresult DrawString(const nsString& aString, nscoord aX, nscoord aY,
                        PRInt32 aFontID = -1,
                        const nscoord* aSpacing = nsnull);

    
    nsresult GetWidth(const char* aString, PRUint32 aLength,
                      nscoord& aWidth);
    nsresult GetWidth(const PRUnichar *aString, PRUint32 aLength,
                      nscoord &aWidth, PRInt32 *aFontID = nsnull);
    nsresult GetWidth(char aC, nscoord &aWidth);
    nsresult GetWidth(PRUnichar aC, nscoord &aWidth,
                      PRInt32 *aFontID);

    nsresult GetTextDimensions(const char* aString, PRUint32 aLength,
                               nsTextDimensions& aDimensions);
    nsresult GetTextDimensions(const PRUnichar* aString, PRUint32 aLength,
                               nsTextDimensions& aDimensions,
                               PRInt32* aFontID = nsnull);

#if defined(_WIN32) || defined(XP_OS2) || defined(MOZ_X11)
    nsresult GetTextDimensions(const char*       aString,
                               PRInt32           aLength,
                               PRInt32           aAvailWidth,
                               PRInt32*          aBreaks,
                               PRInt32           aNumBreaks,
                               nsTextDimensions& aDimensions,
                               PRInt32&          aNumCharsFit,
                               nsTextDimensions& aLastWordDimensions,
                               PRInt32*          aFontID = nsnull);

    nsresult GetTextDimensions(const PRUnichar*  aString,
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
    nsresult GetBoundingMetrics(const char*        aString,
                                PRUint32           aLength,
                                nsBoundingMetrics& aBoundingMetrics);
    nsresult GetBoundingMetrics(const PRUnichar*   aString,
                                PRUint32           aLength,
                                nsBoundingMetrics& aBoundingMetrics,
                                PRInt32*           aFontID = nsnull);
#endif
    nsresult DrawString(const char *aString, PRUint32 aLength,
                        nscoord aX, nscoord aY,
                        const nscoord* aSpacing = nsnull);
    nsresult DrawString(const PRUnichar *aString, PRUint32 aLength,
                        nscoord aX, nscoord aY,
                        PRInt32 aFontID = -1,
                        const nscoord* aSpacing = nsnull);

    nsresult Init(nsIDeviceContext* aContext, gfxASurface* aThebesSurface);
    nsresult Init(nsIDeviceContext* aContext, gfxContext* aThebesContext);
    nsresult Init(nsIDeviceContext* aContext, nsIWidget *aWidget);
    nsresult CommonInit(void);
    already_AddRefed<nsIDeviceContext> GetDeviceContext();
    nsresult PushState(void);
    nsresult PopState(void);
    nsresult SetClipRect(const nsRect& aRect, nsClipCombine aCombine);
    nsresult SetLineStyle(nsLineStyle aLineStyle);
    nsresult SetClipRegion(const nsIntRegion& aRegion, nsClipCombine aCombine);
    nsresult SetColor(nscolor aColor);
    nsresult GetColor(nscolor &aColor) const;
    nsresult SetFont(const nsFont& aFont, nsIAtom* aLanguage,
                     gfxUserFontSet *aUserFontSet);
    nsresult SetFont(const nsFont& aFont,
                     gfxUserFontSet *aUserFontSet);
    nsresult SetFont(nsIFontMetrics *aFontMetrics);
    already_AddRefed<nsIFontMetrics> GetFontMetrics();
    nsresult Translate(const nsPoint& aPt);
    nsresult Scale(float aSx, float aSy);
    nsTransform2D* GetCurrentTransform();

    nsresult DrawLine(const nsPoint& aStartPt, const nsPoint& aEndPt);
    nsresult DrawLine(nscoord aX0, nscoord aY0, nscoord aX1, nscoord aY1);
    nsresult DrawRect(const nsRect& aRect);
    nsresult DrawRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight);
    nsresult FillRect(const nsRect& aRect);
    nsresult FillRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight);
    nsresult InvertRect(const nsRect& aRect);
    nsresult InvertRect(nscoord aX, nscoord aY,
                        nscoord aWidth, nscoord aHeight);
    nsresult FillPolygon(const nsPoint aPoints[], PRInt32 aNumPoints);
    nsresult DrawEllipse(const nsRect& aRect);
    nsresult DrawEllipse(nscoord aX, nscoord aY,
                         nscoord aWidth, nscoord aHeight);
    nsresult FillEllipse(const nsRect& aRect);
    nsresult FillEllipse(nscoord aX, nscoord aY,
                         nscoord aWidth, nscoord aHeight);

    nsresult PushFilter(const nsRect& aRect,
                        PRBool aAreaIsOpaque, float aOpacity);
    nsresult PopFilter();

    enum GraphicDataType {
        NATIVE_CAIRO_CONTEXT = 1,
        NATIVE_GDK_DRAWABLE = 2,
        NATIVE_WINDOWS_DC = 3,
        NATIVE_MAC_THING = 4,
        NATIVE_THEBES_CONTEXT = 5,
        NATIVE_OS2_PS = 6
    };
    void* GetNativeGraphicData(GraphicDataType aType);

    struct PushedTranslation {
        float mSavedX, mSavedY;
    };
    class AutoPushTranslation {
        nsRenderingContext* mCtx;
        PushedTranslation mPushed;
    public:
        AutoPushTranslation(nsRenderingContext* aCtx, const nsPoint& aPt)
            : mCtx(aCtx) {
            mCtx->PushTranslation(&mPushed);
            mCtx->Translate(aPt);
        }
        ~AutoPushTranslation() {
            mCtx->PopTranslation(&mPushed);
        }
    };

    nsresult PushTranslation(PushedTranslation* aState);
    nsresult PopTranslation(PushedTranslation* aState);
    nsresult SetTranslation(const nsPoint& aPoint);

    



    nsresult SetRightToLeftText(PRBool aIsRTL);
    nsresult GetRightToLeftText(PRBool* aIsRTL);
    void SetTextRunRTL(PRBool aIsRTL);

    PRInt32 GetPosition(const PRUnichar *aText,
                        PRUint32 aLength,
                        nsPoint aPt);
    nsresult GetRangeWidth(const PRUnichar *aText,
                           PRUint32 aLength,
                           PRUint32 aStart,
                           PRUint32 aEnd,
                           PRUint32 &aWidth);
    nsresult GetRangeWidth(const char *aText,
                           PRUint32 aLength,
                           PRUint32 aStart,
                           PRUint32 aEnd,
                           PRUint32 &aWidth);

    nsresult RenderEPS(const nsRect& aRect, FILE *aDataFile);

    

    gfxContext *ThebesContext() { return mThebes; }

    nsTransform2D& CurrentTransform();

    void TransformCoord (nscoord *aX, nscoord *aY);

protected:
    nsresult GetWidthInternal(const char *aString, PRUint32 aLength,
                              nscoord &aWidth);
    nsresult GetWidthInternal(const PRUnichar *aString, PRUint32 aLength,
                              nscoord &aWidth, PRInt32 *aFontID = nsnull);

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

    void UpdateTempTransformMatrix();

#ifdef MOZ_MATHML
    


    nsresult GetBoundingMetricsInternal(const char*        aString,
                                        PRUint32           aLength,
                                        nsBoundingMetrics& aBoundingMetrics);

    


    nsresult GetBoundingMetricsInternal(const PRUnichar*   aString,
                                        PRUint32           aLength,
                                        nsBoundingMetrics& aBoundingMetrics,
                                        PRInt32*           aFontID = nsnull);

#endif 

    nsRefPtr<gfxContext> mThebes;
    nsCOMPtr<nsIDeviceContext> mDeviceContext;
    nsCOMPtr<nsIWidget> mWidget;
    nsCOMPtr<nsIThebesFontMetrics> mFontMetrics;

    double mP2A; 
    nsLineStyle mLineStyle;
    nscolor mColor;

    
    nsTransform2D mTempTransform;

    
    nsTArray<float> mOpacityArray;
};

#endif  
