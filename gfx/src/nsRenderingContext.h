





































#ifndef NSRENDERINGCONTEXT__H__
#define NSRENDERINGCONTEXT__H__

#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsIDeviceContext.h"
#include "nsFontMetrics.h"
#include "nsColor.h"
#include "nsCoord.h"
#include "gfxContext.h"

class nsIntRegion;

typedef enum {
    nsLineStyle_kNone   = 0,
    nsLineStyle_kSolid  = 1,
    nsLineStyle_kDashed = 2,
    nsLineStyle_kDotted = 3
} nsLineStyle;

class nsRenderingContext
{
public:
    nsRenderingContext() : mP2A(0.) {}
    

    NS_INLINE_DECL_REFCOUNTING(nsRenderingContext)

    void Init(nsIDeviceContext* aContext, gfxASurface* aThebesSurface);
    void Init(nsIDeviceContext* aContext, gfxContext* aThebesContext);

    
    gfxContext *ThebesContext() { return mThebes; }
    nsIDeviceContext *DeviceContext() { return mDeviceContext; }
    PRInt32 AppUnitsPerDevPixel() { return mP2A; }

    

    void PushState(void);
    void PopState(void);
    void IntersectClip(const nsRect& aRect);
    void SetClip(const nsIntRegion& aRegion);
    void SetLineStyle(nsLineStyle aLineStyle);
    void SetColor(nscolor aColor);
    void Translate(const nsPoint& aPt);
    void Scale(float aSx, float aSy);

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

    

    void DrawLine(const nsPoint& aStartPt, const nsPoint& aEndPt);
    void DrawLine(nscoord aX0, nscoord aY0, nscoord aX1, nscoord aY1);
    void DrawRect(const nsRect& aRect);
    void DrawRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight);
    void DrawEllipse(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight);
    void DrawEllipse(const nsRect& aRect);

    void FillRect(const nsRect& aRect);
    void FillRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight);
    void FillPolygon(const nsPoint aPoints[], PRInt32 aNumPoints);

    void FillEllipse(const nsRect& aRect);
    void FillEllipse(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight);

    void InvertRect(const nsRect& aRect);
    void InvertRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight);

    

    void SetFont(const nsFont& aFont, nsIAtom* aLanguage,
                 gfxUserFontSet *aUserFontSet);
    void SetFont(const nsFont& aFont, gfxUserFontSet *aUserFontSet);
    void SetFont(nsFontMetrics *aFontMetrics);
    nsFontMetrics *FontMetrics() { return mFontMetrics; } 

    void SetRightToLeftText(PRBool aIsRTL);
    void SetTextRunRTL(PRBool aIsRTL);

    nscoord GetWidth(char aC);
    nscoord GetWidth(PRUnichar aC);
    nscoord GetWidth(const nsString& aString);
    nscoord GetWidth(const char* aString);
    nscoord GetWidth(const char* aString, PRUint32 aLength);
    nscoord GetWidth(const PRUnichar *aString, PRUint32 aLength);

#ifdef MOZ_MATHML
    nsBoundingMetrics GetBoundingMetrics(const PRUnichar *aString,
                                         PRUint32 aLength);
#endif

    void DrawString(const nsString& aString, nscoord aX, nscoord aY);
    void DrawString(const char *aString, PRUint32 aLength,
                    nscoord aX, nscoord aY);
    void DrawString(const PRUnichar *aString, PRUint32 aLength,
                    nscoord aX, nscoord aY);

protected:
    PRInt32 GetMaxChunkLength();
    nscoord GetWidthInternal(const char *aString, PRUint32 aLength);
    nscoord GetWidthInternal(const PRUnichar *aString, PRUint32 aLength);

    nsRefPtr<gfxContext> mThebes;
    nsCOMPtr<nsIDeviceContext> mDeviceContext;
    nsRefPtr<nsFontMetrics> mFontMetrics;

    double mP2A; 
};

#endif  
