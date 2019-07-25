





































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
        return Init<ShmemAllocator, false>(aAllocator, aSize, aFormat, aShmType);
    }

    template<class ShmemAllocator>
    bool InitUnsafe(ShmemAllocator *aAllocator,
                    const gfxIntSize& aSize,
                    gfxImageFormat aFormat,
                    SharedMemory::SharedMemoryType aShmType = SharedMemory::TYPE_BASIC)
    {
        return Init<ShmemAllocator, true>(aAllocator, aSize, aFormat, aShmType);
    }

    
    Shmem& GetShmem() { return mShmem; }

    
    static PRBool IsSharedImage(gfxASurface *aSurface);

private:
    template<class ShmemAllocator, bool Unsafe>
    bool Init(ShmemAllocator *aAllocator,
              const gfxIntSize& aSize,
              gfxImageFormat aFormat,
              SharedMemory::SharedMemoryType aShmType)
    {
        mSize = aSize;
        mFormat = aFormat;
        mStride = ComputeStride();
        if (!Unsafe) {
            if (!aAllocator->AllocShmem(GetAlignedSize(),
                                        aShmType, &mShmem))
                return false;
        } else {
            if (!aAllocator->AllocUnsafeShmem(GetAlignedSize(),
                                              aShmType, &mShmem))
                return false;
        }

        return InitSurface(PR_TRUE);
    }

    size_t GetAlignedSize();
    bool InitSurface(PRBool aUpdateShmemInfo);

    Shmem mShmem;
};

#endif 
