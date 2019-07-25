




#ifndef NSRENDERINGCONTEXT__H__
#define NSRENDERINGCONTEXT__H__

#include "nsAutoPtr.h"
#include "nsDeviceContext.h"
#include "nsFontMetrics.h"
#include "nsColor.h"
#include "nsCoord.h"
#include "gfxContext.h"
#include "mozilla/gfx/UserData.h"

struct nsPoint;
class nsIntRegion;

typedef enum {
    nsLineStyle_kNone   = 0,
    nsLineStyle_kSolid  = 1,
    nsLineStyle_kDashed = 2,
    nsLineStyle_kDotted = 3
} nsLineStyle;

class nsRenderingContext
{
    typedef mozilla::gfx::UserData UserData;
    typedef mozilla::gfx::UserDataKey UserDataKey;

public:
    nsRenderingContext() : mP2A(0.) {}
    

    NS_INLINE_DECL_REFCOUNTING(nsRenderingContext)

    void Init(nsDeviceContext* aContext, gfxASurface* aThebesSurface);
    void Init(nsDeviceContext* aContext, gfxContext* aThebesContext);

    
    gfxContext *ThebesContext() { return mThebes; }
    nsDeviceContext *DeviceContext() { return mDeviceContext; }
    PRUint32 AppUnitsPerDevPixel() { return NSToIntRound(mP2A); }

    

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

    void FillRect(const nsRect& aRect);
    void FillRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight);
    void FillPolygon(const nsPoint aPoints[], PRInt32 aNumPoints);

    void FillEllipse(const nsRect& aRect);
    void FillEllipse(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight);

    void InvertRect(const nsRect& aRect);

    

    void SetFont(nsFontMetrics *aFontMetrics);
    nsFontMetrics *FontMetrics() { return mFontMetrics; } 

    void SetTextRunRTL(bool aIsRTL);

    nscoord GetWidth(char aC);
    nscoord GetWidth(PRUnichar aC);
    nscoord GetWidth(const nsString& aString);
    nscoord GetWidth(const char* aString);
    nscoord GetWidth(const char* aString, PRUint32 aLength);
    nscoord GetWidth(const PRUnichar *aString, PRUint32 aLength);

    nsBoundingMetrics GetBoundingMetrics(const PRUnichar *aString,
                                         PRUint32 aLength);

    void DrawString(const nsString& aString, nscoord aX, nscoord aY);
    void DrawString(const char *aString, PRUint32 aLength,
                    nscoord aX, nscoord aY);
    void DrawString(const PRUnichar *aString, PRUint32 aLength,
                    nscoord aX, nscoord aY);

    void AddUserData(UserDataKey *key, void *userData, void (*destroy)(void*)) {
      mUserData.Add(key, userData, destroy);
    }
    void *GetUserData(UserDataKey *key) {
      return mUserData.Get(key);
    }
    void *RemoveUserData(UserDataKey *key) {
      return mUserData.Remove(key);
    }

protected:
    PRInt32 GetMaxChunkLength();

    nsRefPtr<gfxContext> mThebes;
    nsRefPtr<nsDeviceContext> mDeviceContext;
    nsRefPtr<nsFontMetrics> mFontMetrics;

    double mP2A; 

    UserData mUserData;
};

#endif  
