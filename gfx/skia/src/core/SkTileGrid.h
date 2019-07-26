







#ifndef SkTileGrid_DEFINED
#define SkTileGrid_DEFINED

#include "SkBBoxHierarchy.h"
#include "SkPictureStateTree.h"
#include "SkTileGridPicture.h" 











class SkTileGrid : public SkBBoxHierarchy {
public:
    typedef void* (*SkTileGridNextDatumFunctionPtr)(SkTDArray<void*>** tileData, SkTDArray<int>& tileIndices);

    SkTileGrid(int xTileCount, int yTileCount, const SkTileGridPicture::TileGridInfo& info,
        SkTileGridNextDatumFunctionPtr nextDatumFunction);

    virtual ~SkTileGrid();

    





    virtual void insert(void* data, const SkIRect& bounds, bool) SK_OVERRIDE;

    virtual void flushDeferredInserts() SK_OVERRIDE {};

    



    virtual void search(const SkIRect& query, SkTDArray<void*>* results) SK_OVERRIDE;

    virtual void clear() SK_OVERRIDE;

    


    virtual int getCount() const SK_OVERRIDE;

    virtual void rewindInserts() SK_OVERRIDE;

    
    enum {
        kTileFinished = -1,
    };
private:
    SkTDArray<void*>& tile(int x, int y);

    int fXTileCount, fYTileCount, fTileCount;
    SkTileGridPicture::TileGridInfo fInfo;
    SkTDArray<void*>* fTileData;
    int fInsertionCount;
    SkIRect fGridBounds;
    SkTileGridNextDatumFunctionPtr fNextDatumFunction;

    friend class TileGridTest;
    typedef SkBBoxHierarchy INHERITED;
};
















template <typename T>
void* SkTileGridNextDatum(SkTDArray<void*>** tileData, SkTDArray<int>& tileIndices) {
    T* minVal = NULL;
    int tileCount = tileIndices.count();
    
    for (int tile = 0; tile < tileCount; ++tile) {
        int pos = tileIndices[tile];
        if (pos != SkTileGrid::kTileFinished) {
            T* candidate = (T*)(*tileData[tile])[pos];
            if (NULL == minVal || (*candidate) < (*minVal)) {
                minVal = candidate;
            }
        }
    }
    
    if (minVal != NULL) {
        for (int tile = 0; tile < tileCount; ++tile) {
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
