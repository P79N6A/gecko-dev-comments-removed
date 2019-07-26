






#ifndef SkImagePriv_DEFINED
#define SkImagePriv_DEFINED

#include "SkBitmap.h"
#include "SkImage.h"

class SkPicture;

extern SkBitmap::Config SkImageInfoToBitmapConfig(const SkImageInfo&);


extern SkImage* SkNewImageFromPixelRef(const SkImageInfo&, SkPixelRef*,
                                       size_t rowBytes);











extern SkImage* SkNewImageFromBitmap(const SkBitmap&, bool canSharePixelRef);

extern void SkImagePrivDrawPicture(SkCanvas*, SkPicture*,
                                   SkScalar x, SkScalar y, const SkPaint*);

extern void SkImagePrivDrawPicture(SkCanvas*, SkPicture*,
                                   const SkRect*, const SkRect&, const SkPaint*);





extern SkImage* SkNewImageFromPicture(const SkPicture*);

static inline size_t SkImageMinRowBytes(const SkImageInfo& info) {
    return SkAlign4(info.minRowBytes());
}




extern SkPixelRef* SkBitmapImageGetPixelRef(SkImage* rasterImage);


extern SkPicture* SkPictureImageGetPicture(SkImage* pictureImage);




extern GrTexture* SkTextureImageGetTexture(SkImage* textureImage);




extern void SkTextureImageSetTexture(SkImage* image, GrTexture* texture);

#endif
