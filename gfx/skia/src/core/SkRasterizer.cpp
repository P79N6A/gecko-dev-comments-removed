








#include "SkRasterizer.h"
#include "SkDraw.h"
#include "SkMaskFilter.h"
#include "SkPath.h"


SkRasterizer::SkRasterizer(SkFlattenableReadBuffer&) {}

bool SkRasterizer::rasterize(const SkPath& fillPath, const SkMatrix& matrix,
                             const SkIRect* clipBounds, SkMaskFilter* filter,
                             SkMask* mask, SkMask::CreateMode mode) {
    SkIRect storage;
    
    if (clipBounds && filter && SkMask::kJustRenderImage_CreateMode != mode) {        
        SkIPoint    margin;
        SkMask      srcM, dstM;
        
        srcM.fFormat = SkMask::kA8_Format;
        srcM.fBounds.set(0, 0, 1, 1);
        srcM.fImage = NULL;
        if (!filter->filterMask(&dstM, srcM, matrix, &margin)) {
            return false;
        }
        storage = *clipBounds;
        storage.inset(-margin.fX, -margin.fY);
        clipBounds = &storage;
    }
    
    return this->onRasterize(fillPath, matrix, clipBounds, mask, mode);
}



bool SkRasterizer::onRasterize(const SkPath& fillPath, const SkMatrix& matrix,
                             const SkIRect* clipBounds,
                             SkMask* mask, SkMask::CreateMode mode) {
    SkPath  devPath;
    
    fillPath.transform(matrix, &devPath);
    return SkDraw::DrawToMask(devPath, clipBounds, NULL, NULL, mask, mode);
}

