




#ifndef _GFXALPHARECOVERY_H_
#define _GFXALPHARECOVERY_H_

#include "gfxContext.h"
#include "gfxImageSurface.h"
#include "mozilla/SSE.h"
#include "nsRect.h"

class THEBES_API gfxAlphaRecovery {
public:
    struct Analysis {
        bool uniformColor;
        bool uniformAlpha;
        gfxFloat alpha;
        gfxFloat r, g, b;
    };

    







    static uint32_t GoodAlignmentLog2() { return 4;  }

    





    static bool RecoverAlpha (gfxImageSurface *blackSurface,
                                const gfxImageSurface *whiteSurface,
                                Analysis *analysis = nullptr);

#ifdef MOZILLA_MAY_SUPPORT_SSE2
    



    static bool RecoverAlphaSSE2 (gfxImageSurface *blackSurface,
                                    const gfxImageSurface *whiteSurface);

    










    static nsIntRect AlignRectForSubimageRecovery(const nsIntRect& aRect,
                                                  gfxImageSurface* aSurface);
#else
    static nsIntRect AlignRectForSubimageRecovery(const nsIntRect& aRect,
                                                  gfxImageSurface*)
    { return aRect; }
#endif

    
    





















    static inline uint32_t
    RecoverPixel(uint32_t black, uint32_t white)
    {
        const uint32_t GREEN_MASK = 0x0000FF00;
        const uint32_t ALPHA_MASK = 0xFF000000;

        







        uint32_t diff = (white & GREEN_MASK) - (black & GREEN_MASK);
        

        uint32_t limit = diff & ALPHA_MASK;
        
        uint32_t alpha = (ALPHA_MASK - (diff << 16)) | limit;

        return alpha | (black & ~ALPHA_MASK);
    }
};

#endif 
