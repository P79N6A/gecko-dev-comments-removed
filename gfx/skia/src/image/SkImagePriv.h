






#ifndef SkImagePriv_DEFINED
#define SkImagePriv_DEFINED

#include "SkBitmap.h"
#include "SkImage.h"

class SkPicture;

extern SkBitmap::Config SkImageInfoToBitmapConfig(const SkImage::Info&,
                                                  bool* isOpaque);

extern int SkImageBytesPerPixel(SkImage::ColorType);

extern bool SkBitmapToImageInfo(const SkBitmap&, SkImage::Info*);


extern SkImage* SkNewImageFromPixelRef(const SkImage::Info&, SkPixelRef*,
                                       size_t rowBytes);











extern SkImage* SkNewImageFromBitmap(const SkBitmap&, bool canSharePixelRef);

extern void SkImagePrivDrawPicture(SkCanvas*, SkPicture*,
                                   SkScalar x, SkScalar y, const SkPaint*);





extern SkImage* SkNewImageFromPicture(const SkPicture*);

static inline size_t SkImageMinRowBytes(const SkImage::Info& info) {
    size_t rb = info.fWidth * SkImageBytesPerPixel(info.fColorType);
    return SkAlign4(rb);
}




extern SkPixelRef* SkBitmapImageGetPixelRef(SkImage* rasterImage);

#endif
