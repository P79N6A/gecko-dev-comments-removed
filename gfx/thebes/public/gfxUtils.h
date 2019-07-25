




































#ifndef GFX_UTILS_H
#define GFX_UTILS_H

class gfxImageSurface;

class gfxUtils {
public:
    static PRBool FuzzyEqual(gfxFloat aV1, gfxFloat aV2) {
        return fabs(aV2 - aV1) < 1e-6;
    }

    









    static void PremultiplyImageSurface(gfxImageSurface *aSourceSurface,
                                        gfxImageSurface *aDestSurface = nsnull);
    static void UnpremultiplyImageSurface(gfxImageSurface *aSurface,
                                          gfxImageSurface *aDestSurface = nsnull);
};

#endif
