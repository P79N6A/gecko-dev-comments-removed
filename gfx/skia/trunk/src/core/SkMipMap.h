






#ifndef SkMipMap_DEFINED
#define SkMipMap_DEFINED

#include "SkRefCnt.h"
#include "SkScalar.h"

class SkBitmap;

class SkMipMap : public SkRefCnt {
public:
    static SkMipMap* Build(const SkBitmap& src);

    struct Level {
        void*       fPixels;
        uint32_t    fRowBytes;
        uint32_t    fWidth, fHeight;
        float       fScale; 
    };

    bool extractLevel(SkScalar scale, Level*) const;

    size_t getSize() const { return fSize; }

private:
    size_t  fSize;
    Level*  fLevels;
    int     fCount;

    
    SkMipMap(Level* levels, int count, size_t size);
    virtual ~SkMipMap();

    static Level* AllocLevels(int levelCount, size_t pixelSize);
};

#endif
