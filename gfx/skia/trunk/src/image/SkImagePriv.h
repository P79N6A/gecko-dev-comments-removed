






#ifndef SkImagePriv_DEFINED
#define SkImagePriv_DEFINED

#include "SkBitmap.h"
#include "SkImage.h"


extern SkImage* SkNewImageFromPixelRef(const SkImageInfo&, SkPixelRef*,
                                       size_t rowBytes);











extern SkImage* SkNewImageFromBitmap(const SkBitmap&, bool canSharePixelRef);

static inline size_t SkImageMinRowBytes(const SkImageInfo& info) {
    return SkAlign4(info.minRowBytes());
}




extern SkPixelRef* SkBitmapImageGetPixelRef(SkImage* rasterImage);




extern GrTexture* SkTextureImageGetTexture(SkImage* textureImage);




extern void SkTextureImageSetTexture(SkImage* image, GrTexture* texture);

#endif
