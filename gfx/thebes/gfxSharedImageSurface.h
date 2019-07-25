





































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
                 SharedMemory::SharedMemoryType aShmType = SharedMemory::TYPE_BASIC,
                 bool aUsePoTSharedSurface = false)
    {
        return Create<ShmemAllocator, true>(aAllocator, aSize, aFormat,
                                            aShmType, aUsePoTSharedSurface);
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
           SharedMemory::SharedMemoryType aShmType,
           bool aUsePoTSharedSurface = false)
    {
        if (!CheckSurfaceSize(aSize))
            return nsnull;

        Shmem shmem;
        long stride;
        size_t size;
        if (aUsePoTSharedSurface) {
            printf_stderr("Buffer PoT\n");
            int potW = 1;
            while( potW < aSize.width ) potW <<= 1;
            int potH = 1;
            while( potH < aSize.height ) potH <<= 1;

            stride = ComputeStride(gfxIntSize(potW, potH), aFormat);
            size = GetAlignedSize(gfxIntSize(potW, potH), stride);
        } else {
            printf_stderr("Buffer NOT PoT\n");
            stride = ComputeStride(aSize, aFormat);
            size = GetAlignedSize(aSize, stride);
        }

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
