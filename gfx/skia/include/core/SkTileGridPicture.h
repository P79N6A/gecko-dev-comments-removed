






#ifndef SkTileGridPicture_DEFINED
#define SkTileGridPicture_DEFINED

#include "SkPicture.h"
#include "SkPoint.h"
#include "SkSize.h"









class SK_API SkTileGridPicture : public SkPicture {
public:
    struct TileGridInfo {
        
        SkISize  fTileInterval;

        
        SkISize  fMargin;

        






        SkIPoint fOffset;
    };
    





    SkTileGridPicture(int width, int height, const TileGridInfo& info);

    virtual SkBBoxHierarchy* createBBoxHierarchy() const SK_OVERRIDE;

private:
    int fXTileCount, fYTileCount;
    TileGridInfo fInfo;
};

#endif
