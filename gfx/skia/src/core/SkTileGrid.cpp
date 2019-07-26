







#include "SkTileGrid.h"

SkTileGrid::SkTileGrid(int xTileCount, int yTileCount, const SkTileGridPicture::TileGridInfo& info,
    SkTileGridNextDatumFunctionPtr nextDatumFunction)
{
    fXTileCount = xTileCount;
    fYTileCount = yTileCount;
    fInfo = info;
    
    
    fInfo.fMargin.fHeight++;
    fInfo.fMargin.fWidth++;
    fTileCount = fXTileCount * fYTileCount;
    fInsertionCount = 0;
    fGridBounds = SkIRect::MakeXYWH(0, 0, fInfo.fTileInterval.width() * fXTileCount,
        fInfo.fTileInterval.height() * fYTileCount);
    fNextDatumFunction = nextDatumFunction;
    fTileData = SkNEW_ARRAY(SkTDArray<void *>, fTileCount);
}

SkTileGrid::~SkTileGrid() {
    SkDELETE_ARRAY(fTileData);
}

SkTDArray<void *>& SkTileGrid::tile(int x, int y) {
    return fTileData[y * fXTileCount + x];
}

void SkTileGrid::insert(void* data, const SkIRect& bounds, bool) {
    SkASSERT(!bounds.isEmpty());
    SkIRect dilatedBounds = bounds;
    dilatedBounds.outset(fInfo.fMargin.width(), fInfo.fMargin.height());
    dilatedBounds.offset(fInfo.fOffset);
    if (!SkIRect::Intersects(dilatedBounds, fGridBounds)) {
        return;
    }

    
    
    int minTileX = SkMax32(SkMin32(dilatedBounds.left() / fInfo.fTileInterval.width(),
        fXTileCount - 1), 0);
    int maxTileX = SkMax32(SkMin32((dilatedBounds.right() - 1) / fInfo.fTileInterval.width(),
        fXTileCount - 1), 0);
    int minTileY = SkMax32(SkMin32(dilatedBounds.top() / fInfo.fTileInterval.height(),
        fYTileCount -1), 0);
    int maxTileY = SkMax32(SkMin32((dilatedBounds.bottom() -1) / fInfo.fTileInterval.height(),
        fYTileCount -1), 0);

    for (int x = minTileX; x <= maxTileX; x++) {
        for (int y = minTileY; y <= maxTileY; y++) {
            this->tile(x, y).push(data);
        }
    }
    fInsertionCount++;
}

void SkTileGrid::search(const SkIRect& query, SkTDArray<void*>* results) {
    SkIRect adjustedQuery = query;
    adjustedQuery.inset(fInfo.fMargin.width(), fInfo.fMargin.height());
    adjustedQuery.offset(fInfo.fOffset);
    
    
    
    int tileStartX = adjustedQuery.left() / fInfo.fTileInterval.width();
    int tileEndX = (adjustedQuery.right() + fInfo.fTileInterval.width() - 1) /
        fInfo.fTileInterval.width();
    int tileStartY = adjustedQuery.top() / fInfo.fTileInterval.height();
    int tileEndY = (adjustedQuery.bottom() + fInfo.fTileInterval.height() - 1) /
        fInfo.fTileInterval.height();
    if (tileStartX >= fXTileCount || tileStartY >= fYTileCount || tileEndX <= 0 || tileEndY <= 0) {
        return; 
    }
    
    if (tileStartX < 0) tileStartX = 0;
    if (tileStartY < 0) tileStartY = 0;
    if (tileEndX > fXTileCount) tileEndX = fXTileCount;
    if (tileEndY > fYTileCount) tileEndY = fYTileCount;

    int queryTileCount = (tileEndX - tileStartX) * (tileEndY - tileStartY);
    if (queryTileCount == 1) {
        *results = this->tile(tileStartX, tileStartY);
    } else {
        results->reset();
        SkTDArray<int> curPositions;
        curPositions.setCount(queryTileCount);
        
        
        
        
        SkAutoSTArray<1024, SkTDArray<void *>*> storage(queryTileCount);
        SkTDArray<void *>** tileRange = storage.get();
        int tile = 0;
        for (int x = tileStartX; x < tileEndX; ++x) {
            for (int y = tileStartY; y < tileEndY; ++y) {
                tileRange[tile] = &this->tile(x, y);
                curPositions[tile] = tileRange[tile]->count() ? 0 : kTileFinished;
                ++tile;
            }
        }
        void *nextElement;
        while(NULL != (nextElement = fNextDatumFunction(tileRange, curPositions))) {
            results->push(nextElement);
        }
    }
}

void SkTileGrid::clear() {
    for (int i = 0; i < fTileCount; i++) {
        fTileData[i].reset();
    }
}

int SkTileGrid::getCount() const {
    return fInsertionCount;
}

void SkTileGrid::rewindInserts() {
    SkASSERT(fClient);
    for (int i = 0; i < fTileCount; ++i) {
        while (!fTileData[i].isEmpty() && fClient->shouldRewind(fTileData[i].top())) {
            fTileData[i].pop();
        }
    }
}
