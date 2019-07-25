








#ifndef SkRasterizer_DEFINED
#define SkRasterizer_DEFINED

#include "SkFlattenable.h"
#include "SkMask.h"

class SkMaskFilter;
class SkMatrix;
class SkPath;
struct SkIRect;

class SkRasterizer : public SkFlattenable {
public:
    SkRasterizer() {}

    

    bool rasterize(const SkPath& path, const SkMatrix& matrix,
                   const SkIRect* clipBounds, SkMaskFilter* filter,
                   SkMask* mask, SkMask::CreateMode mode);

    virtual void flatten(SkFlattenableWriteBuffer& ) SK_OVERRIDE {}
protected:
    SkRasterizer(SkFlattenableReadBuffer&);

    virtual bool onRasterize(const SkPath& path, const SkMatrix& matrix,
                             const SkIRect* clipBounds,
                             SkMask* mask, SkMask::CreateMode mode);

private:
    typedef SkFlattenable INHERITED;
};

#endif
