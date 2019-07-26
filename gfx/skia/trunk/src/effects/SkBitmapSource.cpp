






#include "SkBitmapSource.h"
#include "SkDevice.h"
#include "SkCanvas.h"
#include "SkReadBuffer.h"
#include "SkWriteBuffer.h"
#include "SkValidationUtils.h"

SkBitmapSource::SkBitmapSource(const SkBitmap& bitmap)
  : INHERITED(0, 0),
    fBitmap(bitmap),
    fSrcRect(SkRect::MakeWH(SkIntToScalar(bitmap.width()),
                            SkIntToScalar(bitmap.height()))),
    fDstRect(fSrcRect) {
}

SkBitmapSource::SkBitmapSource(const SkBitmap& bitmap, const SkRect& srcRect, const SkRect& dstRect)
  : INHERITED(0, 0),
    fBitmap(bitmap),
    fSrcRect(srcRect),
    fDstRect(dstRect) {
}

SkBitmapSource::SkBitmapSource(SkReadBuffer& buffer)
  : INHERITED(0, buffer) {
    fBitmap.unflatten(buffer);
    buffer.readRect(&fSrcRect);
    buffer.readRect(&fDstRect);
    buffer.validate(buffer.isValid() && SkIsValidRect(fSrcRect) && SkIsValidRect(fDstRect));
}

void SkBitmapSource::flatten(SkWriteBuffer& buffer) const {
    this->INHERITED::flatten(buffer);
    fBitmap.flatten(buffer);
    buffer.writeRect(fSrcRect);
    buffer.writeRect(fDstRect);
}

bool SkBitmapSource::onFilterImage(Proxy* proxy, const SkBitmap&, const SkMatrix& matrix,
                                   SkBitmap* result, SkIPoint* offset) const {
    SkRect bounds, dstRect;
    fBitmap.getBounds(&bounds);
    matrix.mapRect(&dstRect, fDstRect);
    if (fSrcRect == bounds && dstRect == bounds) {
        
        *result = fBitmap;
        offset->fX = offset->fY = 0;
        return true;
    }
    SkIRect dstIRect;
    dstRect.roundOut(&dstIRect);

    SkAutoTUnref<SkBaseDevice> device(proxy->createDevice(dstIRect.width(), dstIRect.height()));
    if (NULL == device.get()) {
        return false;
    }

    SkCanvas canvas(device.get());
    SkPaint paint;

    
    dstRect.offset(-SkIntToScalar(dstIRect.fLeft), -SkIntToScalar(dstIRect.fTop));
    paint.setXfermodeMode(SkXfermode::kSrc_Mode);
    
    
    paint.setFilterLevel(
        fSrcRect.width() == dstRect.width() && fSrcRect.height() == dstRect.height() ?
        SkPaint::kNone_FilterLevel : SkPaint::kMedium_FilterLevel);
    canvas.drawBitmapRectToRect(fBitmap, &fSrcRect, dstRect, &paint);

    *result = device.get()->accessBitmap(false);
    offset->fX = dstIRect.fLeft;
    offset->fY = dstIRect.fTop;
    return true;
}

void SkBitmapSource::computeFastBounds(const SkRect&, SkRect* dst) const {
    *dst = fDstRect;
}

bool SkBitmapSource::onFilterBounds(const SkIRect& src, const SkMatrix& ctm,
                                    SkIRect* dst) const {
    *dst = src;
    return true;
}
