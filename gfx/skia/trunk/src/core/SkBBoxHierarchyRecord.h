







#ifndef SkRTreeCanvas_DEFINED
#define SkRTreeCanvas_DEFINED

#include "SkBBoxHierarchy.h"
#include "SkBBoxRecord.h"





class SkBBoxHierarchyRecord : public SkBBoxRecord, public SkBBoxHierarchyClient {
public:
    
    SkBBoxHierarchyRecord(uint32_t recordFlags, SkBBoxHierarchy* h,
                          SkBaseDevice*);

    virtual void handleBBox(const SkRect& bounds) SK_OVERRIDE;

    virtual int save(SaveFlags flags = kMatrixClip_SaveFlag) SK_OVERRIDE;
    virtual int saveLayer(const SkRect* bounds, const SkPaint* paint,
                          SaveFlags flags = kARGB_ClipLayer_SaveFlag) SK_OVERRIDE;
    virtual void restore() SK_OVERRIDE;

    virtual bool translate(SkScalar dx, SkScalar dy) SK_OVERRIDE;
    virtual bool scale(SkScalar sx, SkScalar sy) SK_OVERRIDE;
    virtual bool rotate(SkScalar degrees) SK_OVERRIDE;
    virtual bool skew(SkScalar sx, SkScalar sy) SK_OVERRIDE;
    virtual bool concat(const SkMatrix& matrix) SK_OVERRIDE;
    virtual void setMatrix(const SkMatrix& matrix) SK_OVERRIDE;

    virtual bool clipRect(const SkRect& rect,
                          SkRegion::Op op = SkRegion::kIntersect_Op,
                          bool doAntiAlias = false) SK_OVERRIDE;
    virtual bool clipRegion(const SkRegion& region,
                            SkRegion::Op op = SkRegion::kIntersect_Op) SK_OVERRIDE;
    virtual bool clipPath(const SkPath& path,
                          SkRegion::Op op = SkRegion::kIntersect_Op,
                          bool doAntiAlias = false) SK_OVERRIDE;
    virtual bool clipRRect(const SkRRect& rrect,
                           SkRegion::Op op = SkRegion::kIntersect_Op,
                           bool doAntiAlias = false) SK_OVERRIDE;

    
    virtual bool shouldRewind(void* data) SK_OVERRIDE;

private:
    typedef SkBBoxRecord INHERITED;
};

#endif
