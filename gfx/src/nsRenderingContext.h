




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
#include "nsFontMetrics.h"              
#include "nsISupports.h"                
#include "nsString.h"               
#include "nscore.h"                     

class nsIntRegion;
struct nsPoint;
struct nsRect;

class nsRenderingContext MOZ_FINAL
{
    typedef mozilla::gfx::DrawTarget DrawTarget;

public:
    nsRenderingContext() {}

    NS_INLINE_DECL_REFCOUNTING(nsRenderingContext)

    void Init(gfxContext* aThebesContext);
    void Init(DrawTarget* aDrawTarget);

    
    gfxContext *ThebesContext() { return mThebes; }
    DrawTarget *GetDrawTarget() { return mThebes->GetDrawTarget(); }

    

    void SetFont(nsFontMetrics *aFontMetrics);
    nsFontMetrics *FontMetrics() { return mFontMetrics; } 

    void SetTextRunRTL(bool aIsRTL);

    nscoord GetWidth(char16_t aC);
    nscoord GetWidth(const nsString& aString);
    nscoord GetWidth(const char16_t *aString, uint32_t aLength);

    nsBoundingMetrics GetBoundingMetrics(const char16_t *aString,
                                         uint32_t aLength);

    void DrawString(const char16_t *aString, uint32_t aLength,
                    nscoord aX, nscoord aY);

private:
    
    ~nsRenderingContext()
    {
    }

    int32_t GetMaxChunkLength();

    nsRefPtr<gfxContext> mThebes;
    nsRefPtr<nsFontMetrics> mFontMetrics;
};

#endif  
