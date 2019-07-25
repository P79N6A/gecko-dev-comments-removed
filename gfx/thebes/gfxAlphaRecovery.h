




































#ifndef _GFXALPHARECOVERY_H_
#define _GFXALPHARECOVERY_H_

#include "gfxContext.h"
#include "gfxImageSurface.h"

class THEBES_API gfxAlphaRecovery {
public:
    struct Analysis {
        PRBool uniformColor;
        PRBool uniformAlpha;
        gfxFloat alpha;
        gfxFloat r, g, b;
    };

    





    static PRBool RecoverAlpha (gfxImageSurface *blackSurface,
                                const gfxImageSurface *whiteSurface,
                                Analysis *analysis = nsnull);

    


    static PRBool RecoverAlphaSSE2 (gfxImageSurface *blackSurface,
                                    const gfxImageSurface *whiteSurface);
};

#endif 
