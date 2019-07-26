









#ifndef SkGr_DEFINED
#define SkGr_DEFINED

#include <stddef.h>


#include "GrTypes.h"
#include "GrContext.h"
#include "GrFontScaler.h"


#include "SkBitmap.h"
#include "SkPath.h"
#include "SkPoint.h"
#include "SkRegion.h"
#include "SkClipStack.h"




GR_STATIC_ASSERT((int)kZero_GrBlendCoeff == (int)SkXfermode::kZero_Coeff);
GR_STATIC_ASSERT((int)kOne_GrBlendCoeff  == (int)SkXfermode::kOne_Coeff);
GR_STATIC_ASSERT((int)kSC_GrBlendCoeff   == (int)SkXfermode::kSC_Coeff);
GR_STATIC_ASSERT((int)kISC_GrBlendCoeff  == (int)SkXfermode::kISC_Coeff);
GR_STATIC_ASSERT((int)kDC_GrBlendCoeff   == (int)SkXfermode::kDC_Coeff);
GR_STATIC_ASSERT((int)kIDC_GrBlendCoeff  == (int)SkXfermode::kIDC_Coeff);
GR_STATIC_ASSERT((int)kSA_GrBlendCoeff   == (int)SkXfermode::kSA_Coeff);
GR_STATIC_ASSERT((int)kISA_GrBlendCoeff  == (int)SkXfermode::kISA_Coeff);
GR_STATIC_ASSERT((int)kDA_GrBlendCoeff   == (int)SkXfermode::kDA_Coeff);
GR_STATIC_ASSERT((int)kIDA_GrBlendCoeff  == (int)SkXfermode::kIDA_Coeff);

#define sk_blend_to_grblend(X) ((GrBlendCoeff)(X))



#include "SkColorPriv.h"





GrPixelConfig SkBitmapConfig2GrPixelConfig(SkBitmap::Config);
GrPixelConfig SkImageInfo2GrPixelConfig(SkColorType, SkAlphaType);
bool GrPixelConfig2ColorType(GrPixelConfig, SkColorType*);

static inline GrColor SkColor2GrColor(SkColor c) {
    SkPMColor pm = SkPreMultiplyColor(c);
    unsigned r = SkGetPackedR32(pm);
    unsigned g = SkGetPackedG32(pm);
    unsigned b = SkGetPackedB32(pm);
    unsigned a = SkGetPackedA32(pm);
    return GrColorPackRGBA(r, g, b, a);
}



bool GrIsBitmapInCache(const GrContext*, const SkBitmap&, const GrTextureParams*);

GrTexture* GrLockAndRefCachedBitmapTexture(GrContext*, const SkBitmap&, const GrTextureParams*);

void GrUnlockAndUnrefCachedBitmapTexture(GrTexture*);




class SkGlyphCache;

class SkGrFontScaler : public GrFontScaler {
public:
    explicit SkGrFontScaler(SkGlyphCache* strike);
    virtual ~SkGrFontScaler();

    
    virtual const GrKey* getKey();
    virtual GrMaskFormat getMaskFormat();
    virtual bool getPackedGlyphBounds(GrGlyph::PackedID, SkIRect* bounds);
    virtual bool getPackedGlyphImage(GrGlyph::PackedID, int width, int height,
                                     int rowBytes, void* image);
    virtual bool getGlyphPath(uint16_t glyphID, SkPath*);

private:
    SkGlyphCache* fStrike;
    GrKey*  fKey;

};



#endif
