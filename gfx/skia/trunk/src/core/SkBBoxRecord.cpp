







#include "SkBBoxRecord.h"

SkBBoxRecord::~SkBBoxRecord() {
    fSaveStack.deleteAll();
}

void SkBBoxRecord::drawOval(const SkRect& rect, const SkPaint& paint) {
    if (this->transformBounds(rect, &paint)) {
        INHERITED::drawOval(rect, paint);
    }
}

void SkBBoxRecord::drawRRect(const SkRRect& rrect, const SkPaint& paint) {
    if (this->transformBounds(rrect.rect(), &paint)) {
        INHERITED::drawRRect(rrect, paint);
    }
}

void SkBBoxRecord::drawRect(const SkRect& rect, const SkPaint& paint) {
    if (this->transformBounds(rect, &paint)) {
        INHERITED::drawRect(rect, paint);
    }
}

void SkBBoxRecord::onDrawDRRect(const SkRRect& outer, const SkRRect& inner,
                                const SkPaint& paint) {
    if (this->transformBounds(outer.rect(), &paint)) {
        this->INHERITED::onDrawDRRect(outer, inner, paint);
    }
}

void SkBBoxRecord::drawPath(const SkPath& path, const SkPaint& paint) {
    if (path.isInverseFillType()) {
        
        
        SkIRect clipBounds;
        if (this->getClipDeviceBounds(&clipBounds)) {
            this->handleBBox(SkRect::Make(clipBounds));
            INHERITED::drawPath(path, paint);
        }
    } else if (this->transformBounds(path.getBounds(), &paint)) {
        INHERITED::drawPath(path, paint);
    }
}

void SkBBoxRecord::drawPoints(PointMode mode, size_t count, const SkPoint pts[],
                              const SkPaint& paint) {
    SkRect bbox;
    bbox.set(pts, SkToInt(count));
    
    
    
    
    
    
    
    
    
    static const SkScalar kMinWidth = 0.01f;
    SkScalar halfStrokeWidth = SkMaxScalar(paint.getStrokeWidth(), kMinWidth) / 2;
    bbox.outset(halfStrokeWidth, halfStrokeWidth);
    if (this->transformBounds(bbox, &paint)) {
        INHERITED::drawPoints(mode, count, pts, paint);
    }
}

void SkBBoxRecord::drawPaint(const SkPaint& paint) {
    SkRect bbox;
    if (this->getClipBounds(&bbox)) {
        if (this->transformBounds(bbox, &paint)) {
            INHERITED::drawPaint(paint);
        }
    }
}

void SkBBoxRecord::clear(SkColor color) {
    SkISize size = this->getDeviceSize();
    SkRect bbox = {0, 0, SkIntToScalar(size.width()), SkIntToScalar(size.height())};
    this->handleBBox(bbox);
    INHERITED::clear(color);
}

void SkBBoxRecord::onDrawText(const void* text, size_t byteLength, SkScalar x, SkScalar y,
                              const SkPaint& paint) {
    SkRect bbox;
    paint.measureText(text, byteLength, &bbox);
    SkPaint::FontMetrics metrics;
    paint.getFontMetrics(&metrics);

    
    if (paint.isVerticalText()) {
        SkScalar h = bbox.fBottom - bbox.fTop;
        if (paint.getTextAlign() == SkPaint::kCenter_Align) {
            bbox.fTop    -= h / 2;
            bbox.fBottom -= h / 2;
        }
        
        bbox.fBottom += metrics.fBottom;
        bbox.fTop += metrics.fTop;
    } else {
        SkScalar w = bbox.fRight - bbox.fLeft;
        if (paint.getTextAlign() == SkPaint::kCenter_Align) {
            bbox.fLeft  -= w / 2;
            bbox.fRight -= w / 2;
        } else if (paint.getTextAlign() == SkPaint::kRight_Align) {
            bbox.fLeft  -= w;
            bbox.fRight -= w;
        }
        
        bbox.fTop = metrics.fTop;
        bbox.fBottom = metrics.fBottom;
    }

    
    
    
    
    SkScalar pad = (metrics.fBottom - metrics.fTop) / 2;
    bbox.fLeft  -= pad;
    bbox.fRight += pad;

    bbox.fLeft += x;
    bbox.fRight += x;
    bbox.fTop += y;
    bbox.fBottom += y;
    if (this->transformBounds(bbox, &paint)) {
        INHERITED::onDrawText(text, byteLength, x, y, paint);
    }
}

void SkBBoxRecord::drawBitmap(const SkBitmap& bitmap, SkScalar left, SkScalar top,
                              const SkPaint* paint) {
    SkRect bbox = {left, top, left + bitmap.width(), top + bitmap.height()};
    if (this->transformBounds(bbox, paint)) {
        INHERITED::drawBitmap(bitmap, left, top, paint);
    }
}

void SkBBoxRecord::drawBitmapRectToRect(const SkBitmap& bitmap, const SkRect* src,
                                        const SkRect& dst, const SkPaint* paint,
                                        DrawBitmapRectFlags flags) {
    if (this->transformBounds(dst, paint)) {
        INHERITED::drawBitmapRectToRect(bitmap, src, dst, paint, flags);
    }
}

void SkBBoxRecord::drawBitmapMatrix(const SkBitmap& bitmap, const SkMatrix& mat,
                                    const SkPaint* paint) {
    SkMatrix m = mat;
    SkRect bbox = {0, 0, SkIntToScalar(bitmap.width()), SkIntToScalar(bitmap.height())};
    m.mapRect(&bbox);
    if (this->transformBounds(bbox, paint)) {
        INHERITED::drawBitmapMatrix(bitmap, mat, paint);
    }
}

void SkBBoxRecord::drawBitmapNine(const SkBitmap& bitmap, const SkIRect& center,
                                  const SkRect& dst, const SkPaint* paint) {
    if (this->transformBounds(dst, paint)) {
        INHERITED::drawBitmapNine(bitmap, center, dst, paint);
    }
}









static SkScalar hack_373785_amend_pad(SkScalar pad) {
    return pad * 4;
}

void SkBBoxRecord::onDrawPosText(const void* text, size_t byteLength, const SkPoint pos[],
                                 const SkPaint& paint) {
    SkRect bbox;
    bbox.set(pos, paint.countText(text, byteLength));
    SkPaint::FontMetrics metrics;
    paint.getFontMetrics(&metrics);
    bbox.fTop += metrics.fTop;
    bbox.fBottom += metrics.fBottom;

    
    SkScalar pad = (metrics.fTop - metrics.fBottom) / 2;
    pad = hack_373785_amend_pad(pad);
    bbox.fLeft += pad;
    bbox.fRight -= pad;

    if (this->transformBounds(bbox, &paint)) {
        INHERITED::onDrawPosText(text, byteLength, pos, paint);
    }
}

void SkBBoxRecord::onDrawPosTextH(const void* text, size_t byteLength, const SkScalar xpos[],
                                  SkScalar constY, const SkPaint& paint) {
    size_t numChars = paint.countText(text, byteLength);
    if (numChars == 0) {
        return;
    }

    const SkFlatData* flatPaintData = this->getFlatPaintData(paint);
    WriteTopBot(paint, *flatPaintData);

    SkScalar top = flatPaintData->topBot()[0];
    SkScalar bottom = flatPaintData->topBot()[1];
    SkScalar pad = top - bottom;

    SkRect bbox;
    bbox.fLeft = SK_ScalarMax;
    bbox.fRight = SK_ScalarMin;

    for (size_t i = 0; i < numChars; ++i) {
        if (xpos[i] < bbox.fLeft) {
            bbox.fLeft = xpos[i];
        }
        if (xpos[i] > bbox.fRight) {
            bbox.fRight = xpos[i];
        }
    }

    
    pad = hack_373785_amend_pad(pad);
    bbox.fLeft  += pad;
    bbox.fRight -= pad;

    bbox.fTop    = top + constY;
    bbox.fBottom = bottom + constY;

    if (!this->transformBounds(bbox, &paint)) {
        return;
    }
    
    
    
    drawPosTextHImpl(text, byteLength, xpos, constY, paint, flatPaintData);
}

void SkBBoxRecord::drawSprite(const SkBitmap& bitmap, int left, int top,
                              const SkPaint* paint) {
    SkRect bbox;
    bbox.set(SkIRect::MakeXYWH(left, top, bitmap.width(), bitmap.height()));
    this->handleBBox(bbox); 
    INHERITED::drawSprite(bitmap, left, top, paint);
}

void SkBBoxRecord::onDrawTextOnPath(const void* text, size_t byteLength, const SkPath& path,
                                    const SkMatrix* matrix, const SkPaint& paint) {
    SkRect bbox = path.getBounds();
    SkPaint::FontMetrics metrics;
    paint.getFontMetrics(&metrics);

    
    SkScalar pad = metrics.fTop;
    bbox.fLeft += pad;
    bbox.fRight -= pad;
    bbox.fTop += pad;
    bbox.fBottom -= pad;

    if (this->transformBounds(bbox, &paint)) {
        INHERITED::onDrawTextOnPath(text, byteLength, path, matrix, paint);
    }
}

void SkBBoxRecord::drawVertices(VertexMode mode, int vertexCount,
                                const SkPoint vertices[], const SkPoint texs[],
                                const SkColor colors[], SkXfermode* xfer,
                                const uint16_t indices[], int indexCount,
                                const SkPaint& paint) {
    SkRect bbox;
    bbox.set(vertices, vertexCount);
    if (this->transformBounds(bbox, &paint)) {
        INHERITED::drawVertices(mode, vertexCount, vertices, texs,
                                colors, xfer, indices, indexCount, paint);
    }
}

void SkBBoxRecord::onDrawPicture(const SkPicture* picture) {
    if (picture->width() > 0 && picture->height() > 0 &&
        this->transformBounds(SkRect::MakeWH(picture->width(), picture->height()), NULL)) {
        this->INHERITED::onDrawPicture(picture);
    }
}

void SkBBoxRecord::willSave() {
    fSaveStack.push(NULL);
    this->INHERITED::willSave();
}

SkCanvas::SaveLayerStrategy SkBBoxRecord::willSaveLayer(const SkRect* bounds,
                                                        const SkPaint* paint,
                                                        SaveFlags flags) {
    
    
    fSaveStack.push(paint && paint->getImageFilter() ? new SkPaint(*paint) : NULL);
    return this->INHERITED::willSaveLayer(bounds, paint, flags);
}

void SkBBoxRecord::willRestore() {
    delete fSaveStack.top();
    fSaveStack.pop();
    this->INHERITED::willRestore();
}

bool SkBBoxRecord::transformBounds(const SkRect& bounds, const SkPaint* paint) {
    SkRect outBounds = bounds;
    outBounds.sort();

    if (paint) {
        
        if (paint->canComputeFastBounds()) {
            SkRect temp;
            outBounds = paint->computeFastBounds(outBounds, &temp);
        } else {
            
            if (!this->getClipBounds(&outBounds)) {
                
                return false;
            }
        }
    }

    for (int i = fSaveStack.count() - 1; i >= 0; --i) {
        const SkPaint* paint = fSaveStack.getAt(i);
        if (paint && paint->canComputeFastBounds()) {
            SkRect temp;
            outBounds = paint->computeFastBounds(outBounds, &temp);
        }
    }

    if (!outBounds.isEmpty() && !this->quickReject(outBounds)) {
        this->getTotalMatrix().mapRect(&outBounds);
        this->handleBBox(outBounds);
        return true;
    }

    return false;
}
