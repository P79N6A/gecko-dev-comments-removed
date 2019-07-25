




































#ifndef GFX_UTILS_H
#define GFX_UTILS_H

#include "gfxTypes.h"

class gfxImageSurface;

class THEBES_API gfxUtils {
public:
    









    static void PremultiplyImageSurface(gfxImageSurface *aSourceSurface,
                                        gfxImageSurface *aDestSurface = nsnull);
    static void UnpremultiplyImageSurface(gfxImageSurface *aSurface,
                                          gfxImageSurface *aDestSurface = nsnull);
};

#endif
