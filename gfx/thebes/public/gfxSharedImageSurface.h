





































#ifndef GFX_SHARED_IMAGESURFACE_H
#define GFX_SHARED_IMAGESURFACE_H

#include "gfxASurface.h"
#include "gfxImageSurface.h"

#include "mozilla/ipc/SharedMemory.h"
#include "mozilla/ipc/Shmem.h"

class THEBES_API gfxSharedImageSurface : public gfxImageSurface {
public:
    


    gfxSharedImageSurface();

    



    gfxSharedImageSurface(const mozilla::ipc::Shmem &aShmem);

    ~gfxSharedImageSurface();

    






    template<class ShmemAllocator>
    bool Init(ShmemAllocator *aAllocator,
              const gfxIntSize& aSize,
              gfxImageFormat aFormat,
              mozilla::ipc::SharedMemory::SharedMemoryType aShmType = mozilla::ipc::SharedMemory::TYPE_BASIC)
    {
        mSize = aSize;
        mFormat = aFormat;
        mStride = ComputeStride();
        if (!aAllocator->AllocShmem(GetAlignedSize(),
                                    aShmType, &mShmem))
            return false;

        return InitSurface(PR_TRUE);
    }

    
    mozilla::ipc::Shmem& GetShmem() { return mShmem; }

    
    static PRBool IsSharedImage(gfxASurface *aSurface);

private:
    size_t GetAlignedSize();
    bool InitSurface(PRBool aUpdateShmemInfo);

    mozilla::ipc::Shmem mShmem;
};

#endif 
