




































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

    







    static PRUint32 GoodAlignmentLog2() { return 4;  }

    





    static bool RecoverAlpha (gfxImageSurface *blackSurface,
                                const gfxImageSurface *whiteSurface,
                                Analysis *analysis = nsnull);

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

    
    





















    static inline PRUint32
    RecoverPixel(PRUint32 black, PRUint32 white)
    {
        const PRUint32 GREEN_MASK = 0x0000FF00;
        const PRUint32 ALPHA_MASK = 0xFF000000;

        







        PRUint32 diff = (white & GREEN_MASK) - (black & GREEN_MASK);
        

        PRUint32 limit = diff & ALPHA_MASK;
        
        PRUint32 alpha = (ALPHA_MASK - (diff << 16)) | limit;

        return alpha | (black & ~ALPHA_MASK);
    }
};

#endif 
