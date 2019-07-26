




#ifndef SURFACE_FACTORY_H_
#define SURFACE_FACTORY_H_

#include <queue>
#include "SurfaceTypes.h"
#include "gfxPoint.h"

namespace mozilla {
namespace gfx {

class SharedSurface;

class SurfaceFactory
{
protected:
    SurfaceCaps mCaps;
    SharedSurfaceType mType;

    SurfaceFactory(SharedSurfaceType type, const SurfaceCaps& caps)
        : mCaps(caps)
        , mType(type)
    {}

public:
    virtual ~SurfaceFactory();

protected:
    virtual SharedSurface* CreateShared(const gfxIntSize& size) = 0;

    std::queue<SharedSurface*> mScraps;

public:
    SharedSurface* NewSharedSurface(const gfxIntSize& size);

    
    void Recycle(SharedSurface*& surf);

    const SurfaceCaps& Caps() const {
        return mCaps;
    }

    SharedSurfaceType Type() const {
        return mType;
    }
};

}   
}   

#endif  
