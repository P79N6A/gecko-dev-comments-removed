





































#ifndef GFX_SHARED_IMAGESURFACE_H
#define GFX_SHARED_IMAGESURFACE_H

#include "mozilla/ipc/Shmem.h"
#include "mozilla/ipc/SharedMemory.h"

#include "gfxASurface.h"
#include "gfxImageSurface.h"

class THEBES_API gfxSharedImageSurface : public gfxImageSurface {
    typedef mozilla::ipc::SharedMemory SharedMemory;
    typedef mozilla::ipc::Shmem Shmem;

public:
    


    gfxSharedImageSurface();

    



    gfxSharedImageSurface(const Shmem &aShmem);

    ~gfxSharedImageSurface();

    






    template<class ShmemAllocator>
    bool Init(ShmemAllocator *aAllocator,
              const gfxIntSize& aSize,
              gfxImageFormat aFormat,
              SharedMemory::SharedMemoryType aShmType = SharedMemory::TYPE_BASIC)
    {
        mSize = aSize;
        mFormat = aFormat;
        mStride = ComputeStride();
        if (!aAllocator->AllocShmem(GetAlignedSize(),
                                    aShmType, &mShmem))
            return false;

        return InitSurface(PR_TRUE);
    }

    
    Shmem& GetShmem() { return mShmem; }

    
    static PRBool IsSharedImage(gfxASurface *aSurface);

private:
    size_t GetAlignedSize();
    bool InitSurface(PRBool aUpdateShmemInfo);

    Shmem mShmem;
};

#endif 
