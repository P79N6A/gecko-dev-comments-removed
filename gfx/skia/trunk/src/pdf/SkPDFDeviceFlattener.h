






#ifndef SkPDFDeviceFlattener_DEFINED
#define SkPDFDeviceFlattener_DEFINED

#include "SkPDFDevice.h"









class SkPDFDeviceFlattener : public SkPDFDevice {
private:
    typedef SkPDFDevice INHERITED;

    SK_API SkPDFDeviceFlattener(const SkSize& pageSize, const SkRect* trimBox = NULL);

public:
    SK_API virtual ~SkPDFDeviceFlattener();

    virtual void drawPoints(const SkDraw&, SkCanvas::PointMode mode,
                            size_t count, const SkPoint[],
                            const SkPaint& paint) SK_OVERRIDE;
    virtual void drawRect(const SkDraw&, const SkRect& r, const SkPaint& paint);
    virtual void drawPath(const SkDraw&, const SkPath& origpath,
                          const SkPaint& paint, const SkMatrix* prePathMatrix,
                          bool pathIsMutable) SK_OVERRIDE;
    virtual void drawText(const SkDraw&, const void* text, size_t len,
                          SkScalar x, SkScalar y, const SkPaint&) SK_OVERRIDE;
    virtual void drawPosText(const SkDraw&, const void* text, size_t len,
                             const SkScalar pos[], SkScalar constY,
                             int scalarsPerPos, const SkPaint&) SK_OVERRIDE;
    virtual void drawTextOnPath(const SkDraw&, const void* text, size_t len,
                                const SkPath& path, const SkMatrix* matrix,
                                const SkPaint& paint) SK_OVERRIDE;
private:

    bool mustFlatten(const SkDraw& d) const;
    bool mustPathText(const SkDraw& d, const SkPaint& paint);

    friend class SkDocument_PDF;
};

#endif  
