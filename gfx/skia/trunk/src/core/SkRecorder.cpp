






#include "SkRecorder.h"
#include "SkPicture.h"


SkRecorder::SkRecorder(SkRecord* record, int width, int height)
    : SkCanvas(width, height), fRecord(record) {}

void SkRecorder::forgetRecord() {
    fRecord = NULL;
}


#define APPEND(T, ...) \
        SkNEW_PLACEMENT_ARGS(fRecord->append<SkRecords::T>(), SkRecords::T, (__VA_ARGS__))


#define INHERITED(method, ...) this->SkCanvas::method(__VA_ARGS__)










template <typename T>
class Reference {
public:
    Reference(const T& x) : fX(x) {}
    operator const T&() const { return fX; }
private:
    const T& fX;
};

template <typename T>
static Reference<T> delay_copy(const T& x) { return Reference<T>(x); }



template <typename T>
T* SkRecorder::copy(const T* src) {
    if (NULL == src) {
        return NULL;
    }
    return SkNEW_PLACEMENT_ARGS(fRecord->alloc<T>(), T, (*src));
}



template <typename T>
T* SkRecorder::copy(const T src[], unsigned count) {
    if (NULL == src) {
        return NULL;
    }
    T* dst = fRecord->alloc<T>(count);
    for (unsigned i = 0; i < count; i++) {
        SkNEW_PLACEMENT_ARGS(dst + i, T, (src[i]));
    }
    return dst;
}




template <>
char* SkRecorder::copy(const char src[], unsigned count) {
    if (NULL == src) {
        return NULL;
    }
    char* dst = fRecord->alloc<char>(count);
    memcpy(dst, src, count);
    return dst;
}

void SkRecorder::clear(SkColor color) {
    APPEND(Clear, color);
}

void SkRecorder::drawPaint(const SkPaint& paint) {
    APPEND(DrawPaint, delay_copy(paint));
}

void SkRecorder::drawPoints(PointMode mode,
                            size_t count,
                            const SkPoint pts[],
                            const SkPaint& paint) {
    APPEND(DrawPoints, delay_copy(paint), mode, count, this->copy(pts, count));
}

void SkRecorder::drawRect(const SkRect& rect, const SkPaint& paint) {
    APPEND(DrawRect, delay_copy(paint), rect);
}

void SkRecorder::drawOval(const SkRect& oval, const SkPaint& paint) {
    APPEND(DrawOval, delay_copy(paint), oval);
}

void SkRecorder::drawRRect(const SkRRect& rrect, const SkPaint& paint) {
    APPEND(DrawRRect, delay_copy(paint), rrect);
}

void SkRecorder::onDrawDRRect(const SkRRect& outer, const SkRRect& inner, const SkPaint& paint) {
    APPEND(DrawDRRect, delay_copy(paint), outer, inner);
}

void SkRecorder::drawPath(const SkPath& path, const SkPaint& paint) {
    APPEND(DrawPath, delay_copy(paint), delay_copy(path));
}

void SkRecorder::drawBitmap(const SkBitmap& bitmap,
                            SkScalar left,
                            SkScalar top,
                            const SkPaint* paint) {
    APPEND(DrawBitmap, this->copy(paint), delay_copy(bitmap), left, top);
}

void SkRecorder::drawBitmapRectToRect(const SkBitmap& bitmap,
                                      const SkRect* src,
                                      const SkRect& dst,
                                      const SkPaint* paint,
                                      DrawBitmapRectFlags flags) {
    APPEND(DrawBitmapRectToRect,
           this->copy(paint), delay_copy(bitmap), this->copy(src), dst, flags);
}

void SkRecorder::drawBitmapMatrix(const SkBitmap& bitmap,
                                  const SkMatrix& matrix,
                                  const SkPaint* paint) {
    APPEND(DrawBitmapMatrix, this->copy(paint), delay_copy(bitmap), matrix);
}

void SkRecorder::drawBitmapNine(const SkBitmap& bitmap,
                                const SkIRect& center,
                                const SkRect& dst,
                                const SkPaint* paint) {
    APPEND(DrawBitmapNine, this->copy(paint), delay_copy(bitmap), center, dst);
}

void SkRecorder::drawSprite(const SkBitmap& bitmap, int left, int top, const SkPaint* paint) {
    APPEND(DrawSprite, this->copy(paint), delay_copy(bitmap), left, top);
}

void SkRecorder::onDrawText(const void* text, size_t byteLength,
                            SkScalar x, SkScalar y, const SkPaint& paint) {
    APPEND(DrawText,
           delay_copy(paint), this->copy((const char*)text, byteLength), byteLength, x, y);
}

void SkRecorder::onDrawPosText(const void* text, size_t byteLength,
                               const SkPoint pos[], const SkPaint& paint) {
    const unsigned points = paint.countText(text, byteLength);
    APPEND(DrawPosText,
           delay_copy(paint),
           this->copy((const char*)text, byteLength),
           byteLength,
           this->copy(pos, points));
}

void SkRecorder::onDrawPosTextH(const void* text, size_t byteLength,
                                const SkScalar xpos[], SkScalar constY, const SkPaint& paint) {
    const unsigned points = paint.countText(text, byteLength);
    APPEND(DrawPosTextH,
           delay_copy(paint),
           this->copy((const char*)text, byteLength),
           byteLength,
           this->copy(xpos, points),
           constY);
}

void SkRecorder::onDrawTextOnPath(const void* text, size_t byteLength, const SkPath& path,
                                  const SkMatrix* matrix, const SkPaint& paint) {
    APPEND(DrawTextOnPath,
           delay_copy(paint),
           this->copy((const char*)text, byteLength),
           byteLength,
           delay_copy(path),
           this->copy(matrix));
}

void SkRecorder::onDrawPicture(const SkPicture* picture) {
    picture->draw(this);
}

void SkRecorder::drawVertices(VertexMode vmode,
                              int vertexCount, const SkPoint vertices[],
                              const SkPoint texs[], const SkColor colors[],
                              SkXfermode* xmode,
                              const uint16_t indices[], int indexCount, const SkPaint& paint) {
    APPEND(DrawVertices, delay_copy(paint),
                         vmode,
                         vertexCount,
                         this->copy(vertices, vertexCount),
                         texs ? this->copy(texs, vertexCount) : NULL,
                         colors ? this->copy(colors, vertexCount) : NULL,
                         xmode,
                         this->copy(indices, indexCount),
                         indexCount);
}

void SkRecorder::willSave() {
    APPEND(Save);
    INHERITED(willSave);
}

SkCanvas::SaveLayerStrategy SkRecorder::willSaveLayer(const SkRect* bounds,
                                                      const SkPaint* paint,
                                                      SkCanvas::SaveFlags flags) {
    APPEND(SaveLayer, this->copy(bounds), this->copy(paint), flags);
    INHERITED(willSaveLayer, bounds, paint, flags);
    return SkCanvas::kNoLayer_SaveLayerStrategy;
}

void SkRecorder::willRestore() {
    APPEND(Restore);
    INHERITED(willRestore);
}

void SkRecorder::onPushCull(const SkRect& rect) {
    APPEND(PushCull, rect);
}

void SkRecorder::onPopCull() {
    APPEND(PopCull);
}

void SkRecorder::didConcat(const SkMatrix& matrix) {
    APPEND(Concat, matrix);
    INHERITED(didConcat, matrix);
}

void SkRecorder::didSetMatrix(const SkMatrix& matrix) {
    APPEND(SetMatrix, matrix);
    INHERITED(didSetMatrix, matrix);
}

void SkRecorder::onClipRect(const SkRect& rect, SkRegion::Op op, ClipEdgeStyle edgeStyle) {
    APPEND(ClipRect, rect, op, edgeStyle == kSoft_ClipEdgeStyle);
    INHERITED(onClipRect, rect, op, edgeStyle);
}

void SkRecorder::onClipRRect(const SkRRect& rrect, SkRegion::Op op, ClipEdgeStyle edgeStyle) {
    APPEND(ClipRRect, rrect, op, edgeStyle == kSoft_ClipEdgeStyle);
    INHERITED(updateClipConservativelyUsingBounds, rrect.getBounds(), op, false);
}

void SkRecorder::onClipPath(const SkPath& path, SkRegion::Op op, ClipEdgeStyle edgeStyle) {
    APPEND(ClipPath, delay_copy(path), op, edgeStyle == kSoft_ClipEdgeStyle);
    INHERITED(updateClipConservativelyUsingBounds, path.getBounds(), op, path.isInverseFillType());
}

void SkRecorder::onClipRegion(const SkRegion& deviceRgn, SkRegion::Op op) {
    APPEND(ClipRegion, delay_copy(deviceRgn), op);
    INHERITED(onClipRegion, deviceRgn, op);
}
