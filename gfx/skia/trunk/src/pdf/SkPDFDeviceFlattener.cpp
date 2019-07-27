






#include "SkPDFDeviceFlattener.h"
#include "SkDraw.h"

static SkISize SkSizeToISize(const SkSize& size) {
    return SkISize::Make(SkScalarRoundToInt(size.width()), SkScalarRoundToInt(size.height()));
}

SkPDFDeviceFlattener::SkPDFDeviceFlattener(const SkSize& pageSize, const SkRect* trimBox)
            : SkPDFDevice(SkSizeToISize(pageSize),
                          SkSizeToISize(pageSize),
                          SkMatrix::I()) {
    
}

SkPDFDeviceFlattener::~SkPDFDeviceFlattener() {
}

static void flattenPaint(const SkDraw& d, SkPaint* paint) {
    if (paint->getShader()) {
        SkAutoTUnref<SkShader> lms(SkShader::CreateLocalMatrixShader(paint->getShader(),
                                                                     *d.fMatrix));
        paint->setShader(lms);
    }
}

void SkPDFDeviceFlattener::drawPoints(const SkDraw& d, SkCanvas::PointMode mode,
                                      size_t count, const SkPoint points[],
                                      const SkPaint& paint) {
    if (!mustFlatten(d)) {
        INHERITED::drawPoints(d, mode, count, points, paint);
        return;
    }

    SkPaint paintFlatten(paint);
    flattenPaint(d, &paintFlatten);

    SkPoint* flattenedPoints = SkNEW_ARRAY(SkPoint, count);
    d.fMatrix->mapPoints(flattenedPoints, points, SkToS32(count));
    SkDraw draw(d);
    SkMatrix identity = SkMatrix::I();
    draw.fMatrix = &identity;
    INHERITED::drawPoints(draw, mode, count, flattenedPoints, paintFlatten);
    SkDELETE_ARRAY(flattenedPoints);
}

void SkPDFDeviceFlattener::drawRect(const SkDraw& d, const SkRect& r, const SkPaint& paint) {
    if (!mustFlatten(d)) {
        INHERITED::drawRect(d, r, paint);
        return;
    }

    SkPath path;
    path.addRect(r);
    path.transform(*d.fMatrix);
    SkDraw draw(d);
    SkMatrix matrix = SkMatrix::I();
    draw.fMatrix = &matrix;

    SkPaint paintFlatten(paint);
    flattenPaint(d, &paintFlatten);

    INHERITED::drawPath(draw, path, paintFlatten, NULL, true);
}

void SkPDFDeviceFlattener::drawPath(const SkDraw& d, const SkPath& origPath,
                                    const SkPaint& paint, const SkMatrix* prePathMatrix,
                                    bool pathIsMutable) {
    if (!mustFlatten(d) && !(prePathMatrix && prePathMatrix->hasPerspective())) {
        INHERITED::drawPath(d, origPath, paint, prePathMatrix, pathIsMutable);
        return;
    }

    SkPath* pathPtr = (SkPath*)&origPath;
    SkPath tmpPath;

    if (!pathIsMutable) {
        tmpPath = origPath;
        pathPtr = &tmpPath;
    }

    if (prePathMatrix) {
        pathPtr->transform(*prePathMatrix);
    }

    SkPaint paintFlatten(paint);
    flattenPaint(d, &paintFlatten);

    bool fill = paintFlatten.getFillPath(*pathPtr, &tmpPath);
    SkDEBUGCODE(pathPtr = (SkPath*)0x12345678);  

    paintFlatten.setPathEffect(NULL);
    if (fill) {
        paintFlatten.setStyle(SkPaint::kFill_Style);
    } else {
        paintFlatten.setStyle(SkPaint::kStroke_Style);
        paintFlatten.setStrokeWidth(0);
    }

    tmpPath.transform(*d.fMatrix);

    SkDraw draw(d);
    SkMatrix matrix = SkMatrix::I();
    draw.fMatrix = &matrix;

    INHERITED::drawPath(draw, tmpPath, paintFlatten, NULL, true);
}

void SkPDFDeviceFlattener::drawText(const SkDraw& d, const void* text, size_t len,
                                    SkScalar x, SkScalar y, const SkPaint& paint) {
    if (mustPathText(d, paint)) {
        d.drawText_asPaths((const char*)text, len, x, y, paint);
        return;
    }

    INHERITED::drawText(d, text, len, x, y, paint);
}

void SkPDFDeviceFlattener::drawPosText(const SkDraw& d, const void* text, size_t len,
                                       const SkScalar pos[], SkScalar constY,
                                       int scalarsPerPos, const SkPaint& paint) {
    if (mustPathText(d, paint)) {
        d.drawPosText_asPaths((const char*)text, len, pos, constY, scalarsPerPos, paint);
        return;
    }
    INHERITED::drawPosText(d, text, len, pos, constY,scalarsPerPos, paint);
}

void SkPDFDeviceFlattener::drawTextOnPath(const SkDraw& d, const void* text, size_t len,
                                          const SkPath& path, const SkMatrix* matrix,
                                          const SkPaint& paint) {
    if (mustPathText(d, paint) || (matrix && matrix->hasPerspective())) {
        d.drawTextOnPath((const char*)text, len, path, matrix, paint);
        return;
    }
    INHERITED::drawTextOnPath(d, text, len, path, matrix, paint);
}

bool SkPDFDeviceFlattener::mustFlatten(const SkDraw& d) const {
    
    return d.fMatrix->hasPerspective();
}

bool SkPDFDeviceFlattener::mustPathText(const SkDraw& d, const SkPaint&) {
    
    
    return d.fMatrix->hasPerspective();
}
