





#ifndef GFX_SHARED_IMAGESURFACE_H
#define GFX_SHARED_IMAGESURFACE_H

#include "mozilla/ipc/Shmem.h"
#include "mozilla/ipc/SharedMemory.h"

#include "gfxASurface.h"
#include "gfxImageSurface.h"

struct SharedImageInfo {
    PRInt32 width;
    PRInt32 height;
    PRInt32 format;
};

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

    static SharedImageInfo* GetShmInfoPtr(const Shmem& aShmem);

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
            return nullptr;

        Shmem shmem;
        long stride = ComputeStride(aSize, aFormat);
        size_t size = GetAlignedSize(aSize, stride);
        if (!Unsafe) {
            if (!aAllocator->AllocShmem(size, aShmType, &shmem))
                return nullptr;
        } else {
            if (!aAllocator->AllocUnsafeShmem(size, aShmType, &shmem))
                return nullptr;
        }

        nsRefPtr<gfxSharedImageSurface> s =
            new gfxSharedImageSurface(aSize, aFormat, shmem);
        if (s->CairoStatus() != 0) {
            aAllocator->DeallocShmem(shmem);
            return nullptr;
        }
        s->WriteShmemInfo();
        return s.forget();
    }

    Shmem mShmem;

    
    gfxSharedImageSurface(const gfxSharedImageSurface&);
    gfxSharedImageSurface& operator=(const gfxSharedImageSurface&);
};

#endif 
