






#include "SkTileGridPicture.h"

#include "SkPictureStateTree.h"
#include "SkTileGrid.h"

SkTileGridPicture::SkTileGridPicture(int width, int height, const TileGridInfo& info) {
    SkASSERT(info.fMargin.width() >= 0);
    SkASSERT(info.fMargin.height() >= 0);
    fInfo = info;
    
    
    
    
    fXTileCount = (width + info.fTileInterval.width() - 1) / info.fTileInterval.width();
    fYTileCount = (height + info.fTileInterval.height() - 1) / info.fTileInterval.height();
}

SkBBoxHierarchy* SkTileGridPicture::createBBoxHierarchy() const {
    return SkNEW_ARGS(SkTileGrid, (fXTileCount, fYTileCount, fInfo,
         SkTileGridNextDatum<SkPictureStateTree::Draw>));
}
