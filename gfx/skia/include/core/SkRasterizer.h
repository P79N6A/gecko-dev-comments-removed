








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
    SK_DECLARE_INST_COUNT(SkRasterizer)

    SkRasterizer() {}

    

    bool rasterize(const SkPath& path, const SkMatrix& matrix,
                   const SkIRect* clipBounds, SkMaskFilter* filter,
                   SkMask* mask, SkMask::CreateMode mode);

protected:
    SkRasterizer(SkFlattenableReadBuffer& buffer) : INHERITED(buffer) {}

    virtual bool onRasterize(const SkPath& path, const SkMatrix& matrix,
                             const SkIRect* clipBounds,
                             SkMask* mask, SkMask::CreateMode mode);

private:
    typedef SkFlattenable INHERITED;
};

#endif
