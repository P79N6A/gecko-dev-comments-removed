





































#ifndef NSRENDERINGCONTEXT__H__
#define NSRENDERINGCONTEXT__H__

#include "nsCOMPtr.h"
#include "nsIDeviceContext.h"
#include "nsIThebesFontMetrics.h"
#include "nsIRegion.h"
#include "nsPoint.h"
#include "nsSize.h"
#include "nsColor.h"
#include "nsRect.h"
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

#ifdef MOZ_MATHML



struct nsBoundingMetrics {

    
    

    
    
    
    
    
    
    

    
    
    
    

    
    

    nscoord leftBearing;
    


    nscoord rightBearing;
    




    nscoord ascent;
    


    nscoord descent;
    




    nscoord width;
    




    nsBoundingMetrics() : leftBearing(0), rightBearing(0),
                          ascent(0), descent(0), width(0)
    {}

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
    nsRenderingContext() : mP2A(0.) {}
    

    NS_INLINE_DECL_REFCOUNTING(nsRenderingContext)

    nsresult Init(nsIDeviceContext* aContext, gfxASurface* aThebesSurface);
    nsresult Init(nsIDeviceContext* aContext, gfxContext* aThebesContext);

    already_AddRefed<nsIDeviceContext> GetDeviceContext();
    gfxContext *ThebesContext() { return mThebes; }

    

    nsresult PushState(void);
    nsresult PopState(void);
    nsresult SetClipRect(const nsRect& aRect, nsClipCombine aCombine);
    nsresult SetClipRegion(const nsIntRegion& aRegion, nsClipCombine aCombine);
    nsresult SetLineStyle(nsLineStyle aLineStyle);
    nsresult SetColor(nscolor aColor);
    nsresult Translate(const nsPoint& aPt);
    nsresult Scale(float aSx, float aSy);

    class AutoPushTranslation {
        nsRenderingContext* mCtx;
    public:
        AutoPushTranslation(nsRenderingContext* aCtx, const nsPoint& aPt)
            : mCtx(aCtx) {
            mCtx->PushState();
            mCtx->Translate(aPt);
        }
        ~AutoPushTranslation() {
            mCtx->PopState();
        }
    };

    

    nsresult DrawLine(const nsPoint& aStartPt, const nsPoint& aEndPt);
    nsresult DrawLine(nscoord aX0, nscoord aY0, nscoord aX1, nscoord aY1);
    nsresult DrawRect(const nsRect& aRect);
    nsresult DrawRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight);
    nsresult DrawEllipse(nscoord aX, nscoord aY,
                         nscoord aWidth, nscoord aHeight);
    nsresult DrawEllipse(const nsRect& aRect);

    nsresult FillRect(const nsRect& aRect);
    nsresult FillRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight);
    nsresult FillPolygon(const nsPoint aPoints[], PRInt32 aNumPoints);

    nsresult FillEllipse(const nsRect& aRect);
    nsresult FillEllipse(nscoord aX, nscoord aY,
                         nscoord aWidth, nscoord aHeight);

    nsresult InvertRect(const nsRect& aRect);
    nsresult InvertRect(nscoord aX, nscoord aY,
                        nscoord aWidth, nscoord aHeight);

    

    nsresult SetFont(const nsFont& aFont, nsIAtom* aLanguage,
                     gfxUserFontSet *aUserFontSet);
    nsresult SetFont(const nsFont& aFont, gfxUserFontSet *aUserFontSet);
    nsresult SetFont(nsIFontMetrics *aFontMetrics);
    already_AddRefed<nsIFontMetrics> GetFontMetrics();
    nsresult SetRightToLeftText(PRBool aIsRTL);
    void SetTextRunRTL(PRBool aIsRTL);

    nsresult GetWidth(const nsString& aString, nscoord &aWidth,
                      PRInt32 *aFontID = nsnull);
    nsresult GetWidth(const char* aString, nscoord& aWidth);
    nsresult GetWidth(const char* aString, PRUint32 aLength,
                      nscoord& aWidth);
    nsresult GetWidth(const PRUnichar *aString, PRUint32 aLength,
                      nscoord &aWidth, PRInt32 *aFontID = nsnull);
    nsresult GetWidth(char aC, nscoord &aWidth);
    nsresult GetWidth(PRUnichar aC, nscoord &aWidth, PRInt32 *aFontID = nsnull);

#ifdef MOZ_MATHML
    nsresult GetBoundingMetrics(const PRUnichar*   aString,
                                PRUint32           aLength,
                                nsBoundingMetrics& aBoundingMetrics,
                                PRInt32*           aFontID = nsnull);
#endif

    nsresult DrawString(const nsString& aString, nscoord aX, nscoord aY,
                        PRInt32 aFontID = -1,
                        const nscoord* aSpacing = nsnull);
    nsresult DrawString(const char *aString, PRUint32 aLength,
                        nscoord aX, nscoord aY,
                        const nscoord* aSpacing = nsnull);
    nsresult DrawString(const PRUnichar *aString, PRUint32 aLength,
                        nscoord aX, nscoord aY,
                        PRInt32 aFontID = -1,
                        const nscoord* aSpacing = nsnull);

protected:
    PRInt32 GetMaxChunkLength();

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

#ifdef MOZ_MATHML
    


    nsresult GetBoundingMetricsInternal(const PRUnichar*   aString,
                                        PRUint32           aLength,
                                        nsBoundingMetrics& aBoundingMetrics,
                                        PRInt32*           aFontID = nsnull);
#endif 

    nsRefPtr<gfxContext> mThebes;
    nsCOMPtr<nsIDeviceContext> mDeviceContext;
    nsCOMPtr<nsIThebesFontMetrics> mFontMetrics;

    double mP2A; 
};

#endif  
