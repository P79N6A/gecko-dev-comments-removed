






#include "SkBitmapSource.h"

SkBitmapSource::SkBitmapSource(const SkBitmap& bitmap) : fBitmap(bitmap) {
}

SkBitmapSource::SkBitmapSource(SkFlattenableReadBuffer& buffer)
  : INHERITED(buffer) {
    fBitmap.unflatten(buffer);
}

void SkBitmapSource::flatten(SkFlattenableWriteBuffer& buffer) const {
    this->INHERITED::flatten(buffer);
    fBitmap.flatten(buffer);
}

bool SkBitmapSource::onFilterImage(Proxy*, const SkBitmap&, const SkMatrix&,
                                   SkBitmap* result, SkIPoint* offset) {
    *result = fBitmap;
    return true;
}

SK_DEFINE_FLATTENABLE_REGISTRAR(SkBitmapSource)
