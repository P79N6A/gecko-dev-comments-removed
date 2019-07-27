






#ifndef SkBBHFactory_DEFINED
#define SkBBHFactory_DEFINED

#include "SkSize.h"
#include "SkPoint.h"

class SkBBoxHierarchy;

class SK_API SkBBHFactory {
public:
    


    virtual SkBBoxHierarchy* operator()(int width, int height) const = 0;
    virtual ~SkBBHFactory() {};
};

class SK_API SkQuadTreeFactory : public SkBBHFactory {
public:
    virtual SkBBoxHierarchy* operator()(int width, int height) const SK_OVERRIDE;
private:
    typedef SkBBHFactory INHERITED;
};


class SK_API SkRTreeFactory : public SkBBHFactory {
public:
    virtual SkBBoxHierarchy* operator()(int width, int height) const SK_OVERRIDE;
private:
    typedef SkBBHFactory INHERITED;
};

class SK_API SkTileGridFactory : public SkBBHFactory {
public:
    struct TileGridInfo {
        
        SkISize  fTileInterval;

        
        SkISize  fMargin;

        






        SkIPoint fOffset;
    };

    SkTileGridFactory(const TileGridInfo& info) : fInfo(info) { }

    virtual SkBBoxHierarchy* operator()(int width, int height) const SK_OVERRIDE;

private:
    TileGridInfo fInfo;

    typedef SkBBHFactory INHERITED;
};

#endif
