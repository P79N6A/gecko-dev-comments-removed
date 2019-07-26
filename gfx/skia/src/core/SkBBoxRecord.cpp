







#include "SkBBoxRecord.h"

void SkBBoxRecord::drawRect(const SkRect& rect, const SkPaint& paint) {
    if (this->transformBounds(rect, &paint)) {
        INHERITED::drawRect(rect, paint);
    }
}

void SkBBoxRecord::drawPath(const SkPath& path, const SkPaint& paint) {
    if (this->transformBounds(path.getBounds(), &paint)) {
        INHERITED::drawPath(path, paint);
    }
}

void SkBBoxRecord::drawPoints(PointMode mode, size_t count, const SkPoint pts[],
                              const SkPaint& paint) {
    SkRect bbox;
    bbox.set(pts, count);
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
    SkRect bbox = {0, 0, size.width(), size.height()};
    this->handleBBox(bbox);
    INHERITED::clear(color);
}

void SkBBoxRecord::drawText(const void* text, size_t byteLength, SkScalar x, SkScalar y,
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
        INHERITED::drawText(text, byteLength, x, y, paint);
    }
}

void SkBBoxRecord::drawBitmap(const SkBitmap& bitmap, SkScalar left, SkScalar top,
                              const SkPaint* paint) {
    SkRect bbox = {left, top, left + bitmap.width(), top + bitmap.height()};
    if (this->transformBounds(bbox, paint)) {
        INHERITED::drawBitmap(bitmap, left, top, paint);
    }
}

void SkBBoxRecord::drawBitmapRect(const SkBitmap& bitmap, const SkIRect* src,
                                  const SkRect& dst, const SkPaint* paint) {
    if (this->transformBounds(dst, paint)) {
        INHERITED::drawBitmapRect(bitmap, src, dst, paint);
    }
}

void SkBBoxRecord::drawBitmapMatrix(const SkBitmap& bitmap, const SkMatrix& mat,
                                    const SkPaint* paint) {
    SkMatrix m = mat;
    SkRect bbox = {0, 0, bitmap.width(), bitmap.height()};
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

void SkBBoxRecord::drawPosText(const void* text, size_t byteLength,
                               const SkPoint pos[], const SkPaint& paint) {
    SkRect bbox;
    bbox.set(pos, paint.countText(text, byteLength));
    SkPaint::FontMetrics metrics;
    paint.getFontMetrics(&metrics);
    bbox.fTop += metrics.fTop;
    bbox.fBottom += metrics.fBottom;

    
    SkScalar pad = (metrics.fTop - metrics.fBottom) / 2;
    bbox.fLeft += pad;
    bbox.fRight -= pad;

    if (this->transformBounds(bbox, &paint)) {
        INHERITED::drawPosText(text, byteLength, pos, paint);
    }
}

void SkBBoxRecord::drawPosTextH(const void* text, size_t byteLength, const SkScalar xpos[],
                                SkScalar constY, const SkPaint& paint) {
    SkRect bbox;
    size_t numChars = paint.countText(text, byteLength);
    if (numChars > 0) {
        bbox.fLeft  = xpos[0];
        bbox.fRight = xpos[numChars - 1];
        
        for (size_t i = 1; i < numChars; ++i) {
            if (xpos[i] < bbox.fLeft) {
                bbox.fLeft = xpos[i];
            }
            if (xpos[i] > bbox.fRight) {
                bbox.fRight = xpos[i];
            }
        }
        SkPaint::FontMetrics metrics;
        paint.getFontMetrics(&metrics);

        
        SkScalar pad = (metrics.fTop - metrics.fBottom);
        bbox.fLeft  += pad;
        bbox.fRight -= pad;

        bbox.fTop    = metrics.fTop + constY;
        bbox.fBottom = metrics.fBottom + constY;
        if (!this->transformBounds(bbox, &paint)) {
            return;
        }
    }
    INHERITED::drawPosTextH(text, byteLength, xpos, constY, paint);
}

void SkBBoxRecord::drawSprite(const SkBitmap& bitmap, int left, int top,
                              const SkPaint* paint) {
    SkRect bbox = {left, top, left + bitmap.width(), top + bitmap.height()};
    this->handleBBox(bbox); 
    INHERITED::drawBitmap(bitmap, left, top, paint);
}

void SkBBoxRecord::drawTextOnPath(const void* text, size_t byteLength,
                                  const SkPath& path, const SkMatrix* matrix,
                                  const SkPaint& paint) {
    SkRect bbox = path.getBounds();
    SkPaint::FontMetrics metrics;
    paint.getFontMetrics(&metrics);

    
    SkScalar pad = metrics.fTop;
    bbox.fLeft += pad;
    bbox.fRight -= pad;
    bbox.fTop += pad;
    bbox.fBottom -= pad;

    if (this->transformBounds(bbox, &paint)) {
        INHERITED::drawTextOnPath(text, byteLength, path, matrix, paint);
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

void SkBBoxRecord::drawPicture(SkPicture& picture) {
    if (picture.width() > 0 && picture.height() > 0 &&
        this->transformBounds(SkRect::MakeWH(picture.width(), picture.height()), NULL)) {
        INHERITED::drawPicture(picture);
    }
}

bool SkBBoxRecord::transformBounds(const SkRect& bounds, const SkPaint* paint) {
    SkRect outBounds = bounds;

    if (paint) {
        
        if (paint->canComputeFastBounds()) {
            SkRect temp;
            outBounds = paint->computeFastBounds(bounds, &temp);
        } else {
            
            if (!this->getClipBounds(&outBounds)) {
                
                return false;
            }
        }
    }

    SkRect clip;

    if (this->getClipBounds(&clip) && outBounds.intersect(clip)) {
        this->getTotalMatrix().mapRect(&outBounds);
        this->handleBBox(outBounds);
        return true;
    }

    return false;
}

