







#ifndef SkRTreeCanvas_DEFINED
#define SkRTreeCanvas_DEFINED

#include "SkBBoxHierarchy.h"
#include "SkBBoxRecord.h"





class SkBBoxHierarchyRecord : public SkBBoxRecord, public SkBBoxHierarchyClient {
public:
    
    SkBBoxHierarchyRecord(const SkISize& size, uint32_t recordFlags, SkBBoxHierarchy* h);
    virtual ~SkBBoxHierarchyRecord() { };

    virtual void handleBBox(const SkRect& bounds) SK_OVERRIDE;

    
    virtual bool shouldRewind(void* data) SK_OVERRIDE;

protected:
    virtual void willSave() SK_OVERRIDE;
    virtual SaveLayerStrategy willSaveLayer(const SkRect*, const SkPaint*, SaveFlags) SK_OVERRIDE;
    virtual void willRestore() SK_OVERRIDE;

    virtual void didConcat(const SkMatrix&) SK_OVERRIDE;
    virtual void didSetMatrix(const SkMatrix&) SK_OVERRIDE;

    virtual void onClipRect(const SkRect&, SkRegion::Op, ClipEdgeStyle) SK_OVERRIDE;
    virtual void onClipRRect(const SkRRect&, SkRegion::Op, ClipEdgeStyle) SK_OVERRIDE;
    virtual void onClipPath(const SkPath&, SkRegion::Op, ClipEdgeStyle) SK_OVERRIDE;
    virtual void onClipRegion(const SkRegion&, SkRegion::Op) SK_OVERRIDE;

private:
    typedef SkBBoxRecord INHERITED;
};

#endif
