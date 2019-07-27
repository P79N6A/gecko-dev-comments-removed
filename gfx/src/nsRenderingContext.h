




#ifndef NSRENDERINGCONTEXT__H__
#define NSRENDERINGCONTEXT__H__

#include <stdint.h>                     
#include <sys/types.h>                  
#include "gfxContext.h"                 
#include "mozilla/Assertions.h"         
#include "mozilla/gfx/2D.h"
#include "nsAutoPtr.h"                  
#include "nsBoundingMetrics.h"          
#include "nsColor.h"                    
#include "nsCoord.h"                    
#include "nsDeviceContext.h"            
#include "nsFontMetrics.h"              
#include "nsISupports.h"                
#include "nsString.h"               
#include "nscore.h"                     

class nsIntRegion;
struct nsPoint;
struct nsRect;

typedef enum {
    nsLineStyle_kNone   = 0,
    nsLineStyle_kSolid  = 1,
    nsLineStyle_kDashed = 2,
    nsLineStyle_kDotted = 3
} nsLineStyle;

class nsRenderingContext MOZ_FINAL
{
    typedef mozilla::gfx::DrawTarget DrawTarget;

public:
    nsRenderingContext() : mP2A(0.) {}

    NS_INLINE_DECL_REFCOUNTING(nsRenderingContext)

    void Init(nsDeviceContext* aContext, gfxContext* aThebesContext);
    void Init(nsDeviceContext* aContext, DrawTarget* aDrawTarget);

    
    gfxContext *ThebesContext() { return mThebes; }
    DrawTarget *GetDrawTarget() { return mThebes->GetDrawTarget(); }
    nsDeviceContext *DeviceContext() { return mDeviceContext; }

    

    void PushState(void);
    void PopState(void);
    void IntersectClip(const nsRect& aRect);
    void SetClip(const nsIntRegion& aRegion);
    void SetLineStyle(nsLineStyle aLineStyle);
    void SetColor(nscolor aColor);

    

    void DrawLine(const nsPoint& aStartPt, const nsPoint& aEndPt);
    void DrawLine(nscoord aX0, nscoord aY0, nscoord aX1, nscoord aY1);
    void DrawRect(const nsRect& aRect);
    void DrawRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight);
    void DrawEllipse(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight);

    void FillRect(const nsRect& aRect);
    void FillRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight);
    void FillPolygon(const nsPoint aPoints[], int32_t aNumPoints);

    void FillEllipse(const nsRect& aRect);
    void FillEllipse(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight);

    

    void SetFont(nsFontMetrics *aFontMetrics);
    nsFontMetrics *FontMetrics() { return mFontMetrics; } 

    void SetTextRunRTL(bool aIsRTL);

    nscoord GetWidth(char aC);
    nscoord GetWidth(char16_t aC);
    nscoord GetWidth(const nsString& aString);
    nscoord GetWidth(const char* aString);
    nscoord GetWidth(const char* aString, uint32_t aLength);
    nscoord GetWidth(const char16_t *aString, uint32_t aLength);

    nsBoundingMetrics GetBoundingMetrics(const char16_t *aString,
                                         uint32_t aLength);

    void DrawString(const nsString& aString, nscoord aX, nscoord aY);
    void DrawString(const char *aString, uint32_t aLength,
                    nscoord aX, nscoord aY);
    void DrawString(const char16_t *aString, uint32_t aLength,
                    nscoord aX, nscoord aY);

private:
    
    ~nsRenderingContext()
    {
    }

    int32_t GetMaxChunkLength();

    nsRefPtr<gfxContext> mThebes;
    nsRefPtr<nsDeviceContext> mDeviceContext;
    nsRefPtr<nsFontMetrics> mFontMetrics;

    double mP2A; 
};

#endif  
