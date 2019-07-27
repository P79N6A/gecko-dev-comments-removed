






#include "SkBBHFactory.h"
#include "SkPictureStateTree.h"
#include "SkQuadTree.h"
#include "SkRTree.h"
#include "SkTileGrid.h"


SkBBoxHierarchy* SkQuadTreeFactory::operator()(int width, int height) const {
    return SkNEW_ARGS(SkQuadTree, (SkIRect::MakeWH(width, height)));
}

SkBBoxHierarchy* SkRTreeFactory::operator()(int width, int height) const {
    
    
    static const int kRTreeMinChildren = 6;
    static const int kRTreeMaxChildren = 11;

    SkScalar aspectRatio = SkScalarDiv(SkIntToScalar(width),
                                       SkIntToScalar(height));
    bool sortDraws = false;  

    return SkRTree::Create(kRTreeMinChildren, kRTreeMaxChildren,
                           aspectRatio, sortDraws);
}

SkBBoxHierarchy* SkTileGridFactory::operator()(int width, int height) const {
    SkASSERT(fInfo.fMargin.width() >= 0);
    SkASSERT(fInfo.fMargin.height() >= 0);
    
    
    
    
    int xTileCount = (width + fInfo.fTileInterval.width() - 1) / fInfo.fTileInterval.width();
    int yTileCount = (height + fInfo.fTileInterval.height() - 1) / fInfo.fTileInterval.height();
    return SkNEW_ARGS(SkTileGrid, (xTileCount, yTileCount, fInfo,
                                    SkTileGridNextDatum<SkPictureStateTree::Draw>));
}
