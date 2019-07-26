




#include "SurfaceFactory.h"

#include "SharedSurface.h"

namespace mozilla {
namespace gfx {

SurfaceFactory::~SurfaceFactory()
{
    while (!mScraps.empty()) {
        SharedSurface* cur = mScraps.front();
        mScraps.pop();

        delete cur;
    }
}

SharedSurface*
SurfaceFactory::NewSharedSurface(const gfxIntSize& size)
{
    
    while (!mScraps.empty()) {
        SharedSurface* cur = mScraps.front();
        mScraps.pop();
        if (cur->Size() == size)
            return cur;

        
        delete cur;
    }

    SharedSurface* ret = CreateShared(size);

    return ret;
}


void
SurfaceFactory::Recycle(SharedSurface*& surf)
{
    if (!surf)
        return;

    if (surf->Type() == mType) {
        mScraps.push(surf);
    } else {
        delete surf;
    }

    surf = nullptr;
}

} 
} 
