







#ifndef SkTileGrid_DEFINED
#define SkTileGrid_DEFINED

#include "SkBBHFactory.h"
#include "SkBBoxHierarchy.h"
#include "SkPictureStateTree.h"











class SkTileGrid : public SkBBoxHierarchy {
public:
    enum {
        
        
        
        
        kStackAllocationTileCount = 1024
    };

    typedef void* (*SkTileGridNextDatumFunctionPtr)(SkTDArray<void*>** tileData, SkAutoSTArray<kStackAllocationTileCount, int>& tileIndices);

    SkTileGrid(int xTileCount, int yTileCount, const SkTileGridFactory::TileGridInfo& info,
        SkTileGridNextDatumFunctionPtr nextDatumFunction);

    virtual ~SkTileGrid();

    





    virtual void insert(void* data, const SkIRect& bounds, bool) SK_OVERRIDE;

    virtual void flushDeferredInserts() SK_OVERRIDE {};

    



    virtual void search(const SkIRect& query, SkTDArray<void*>* results) SK_OVERRIDE;

    virtual void clear() SK_OVERRIDE;

    


    virtual int getCount() const SK_OVERRIDE;

    virtual int getDepth() const SK_OVERRIDE { return -1; }

    virtual void rewindInserts() SK_OVERRIDE;

    
    enum {
        kTileFinished = -1,
    };

    int tileCount(int x, int y);  

private:
    SkTDArray<void*>& tile(int x, int y);

    int fXTileCount, fYTileCount, fTileCount;
    SkTileGridFactory::TileGridInfo fInfo;
    SkTDArray<void*>* fTileData;
    int fInsertionCount;
    SkIRect fGridBounds;
    SkTileGridNextDatumFunctionPtr fNextDatumFunction;

    typedef SkBBoxHierarchy INHERITED;
};
















template <typename T>
void* SkTileGridNextDatum(SkTDArray<void*>** tileData, SkAutoSTArray<SkTileGrid::kStackAllocationTileCount, int>& tileIndices) {
    T* minVal = NULL;
    int tileCount = tileIndices.count();
    int minIndex = tileCount;
    int maxIndex = 0;
    
    for (int tile = 0; tile < tileCount; ++tile) {
        int pos = tileIndices[tile];
        if (pos != SkTileGrid::kTileFinished) {
            T* candidate = (T*)(*tileData[tile])[pos];
            if (NULL == minVal || (*candidate) < (*minVal)) {
                minVal = candidate;
                minIndex = tile;
                maxIndex = tile;
            } else if (!((*minVal) < (*candidate))) {
                
                
                maxIndex = tile;
            }
        }
    }
    
    if (minVal != NULL) {
        for (int tile = minIndex; tile <= maxIndex; ++tile) {
            int pos = tileIndices[tile];
            if (pos != SkTileGrid::kTileFinished && (*tileData[tile])[pos] == minVal) {
                if (++(tileIndices[tile]) >= tileData[tile]->count()) {
                    tileIndices[tile] = SkTileGrid::kTileFinished;
                }
            }
        }
        return minVal;
    }
    return NULL;
}

#endif
