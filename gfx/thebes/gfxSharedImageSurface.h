





































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
    virtual ~gfxSharedImageSurface();

    








    template<class ShmemAllocator>
    static already_AddRefed<gfxSharedImageSurface>
    Create(ShmemAllocator* aAllocator,
           const gfxIntSize& aSize,
           gfxImageFormat aFormat,
           SharedMemory::SharedMemoryType aShmType = SharedMemory::TYPE_BASIC)
    {
        return Create<ShmemAllocator, false>(aAllocator, aSize, aFormat, aShmType);
    }

    





    static already_AddRefed<gfxSharedImageSurface>
    Open(const Shmem& aShmem);

    template<class ShmemAllocator>
    static already_AddRefed<gfxSharedImageSurface>
    CreateUnsafe(ShmemAllocator* aAllocator,
                 const gfxIntSize& aSize,
                 gfxImageFormat aFormat,
                 SharedMemory::SharedMemoryType aShmType = SharedMemory::TYPE_BASIC)
    {
        return Create<ShmemAllocator, true>(aAllocator, aSize, aFormat, aShmType);
    }

    Shmem& GetShmem() { return mShmem; }

    static bool IsSharedImage(gfxASurface *aSurface);

private:
    gfxSharedImageSurface(const gfxIntSize&, gfxImageFormat, const Shmem&);

    void WriteShmemInfo();

    static size_t GetAlignedSize(const gfxIntSize&, long aStride);

    template<class ShmemAllocator, bool Unsafe>
    static already_AddRefed<gfxSharedImageSurface>
    Create(ShmemAllocator* aAllocator,
           const gfxIntSize& aSize,
           gfxImageFormat aFormat,
           SharedMemory::SharedMemoryType aShmType)
    {
        if (!CheckSurfaceSize(aSize))
            return nsnull;

        Shmem shmem;
        long stride = ComputeStride(aSize, aFormat);
        size_t size = GetAlignedSize(aSize, stride);
        if (!Unsafe) {
            if (!aAllocator->AllocShmem(size, aShmType, &shmem))
                return nsnull;
        } else {
            if (!aAllocator->AllocUnsafeShmem(size, aShmType, &shmem))
                return nsnull;
        }

        nsRefPtr<gfxSharedImageSurface> s =
            new gfxSharedImageSurface(aSize, aFormat, shmem);
        if (s->CairoStatus() != 0) {
            aAllocator->DeallocShmem(shmem);
            return nsnull;
        }
        s->WriteShmemInfo();
        return s.forget();
    }

    Shmem mShmem;

    
    gfxSharedImageSurface(const gfxSharedImageSurface&);
    gfxSharedImageSurface& operator=(const gfxSharedImageSurface&);
};

#endif 
