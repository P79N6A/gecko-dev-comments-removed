





#ifndef GFX_SHARED_MEMORYSURFACE_H
#define GFX_SHARED_MEMORYSURFACE_H

#include "mozilla/ipc/Shmem.h"
#include "mozilla/ipc/SharedMemory.h"

#include "gfxASurface.h"
#include "gfxImageSurface.h"
#include "cairo.h"

struct SharedImageInfo {
    int32_t width;
    int32_t height;
    int32_t format;
};

inline SharedImageInfo*
GetShmInfoPtr(const mozilla::ipc::Shmem& aShmem)
{
    return reinterpret_cast<SharedImageInfo*>
        (aShmem.get<char>() + aShmem.Size<char>() - sizeof(SharedImageInfo));
}

extern const cairo_user_data_key_t SHM_KEY;

template <typename Base, typename Sub>
class THEBES_API gfxBaseSharedMemorySurface : public Base {
    typedef mozilla::ipc::SharedMemory SharedMemory;
    typedef mozilla::ipc::Shmem Shmem;

public:
    virtual ~gfxBaseSharedMemorySurface()
    {
        MOZ_COUNT_DTOR(gfxBaseSharedMemorySurface);
    }

    








    template<class ShmemAllocator>
    static already_AddRefed<Sub>
    Create(ShmemAllocator* aAllocator,
           const gfxIntSize& aSize,
           gfxASurface::gfxImageFormat aFormat,
           SharedMemory::SharedMemoryType aShmType = SharedMemory::TYPE_BASIC)
    {
        return Create<ShmemAllocator, false>(aAllocator, aSize, aFormat, aShmType);
    }

    





    static already_AddRefed<Sub>
    Open(const Shmem& aShmem)
    {
        SharedImageInfo* shmInfo = GetShmInfoPtr(aShmem);
        gfxIntSize size(shmInfo->width, shmInfo->height);
        if (!gfxASurface::CheckSurfaceSize(size))
            return nullptr;
       
        gfxASurface::gfxImageFormat format = (gfxASurface::gfxImageFormat)shmInfo->format;
        long stride = gfxImageSurface::ComputeStride(size, format);

        nsRefPtr<Sub> s =
            new Sub(size,
                    stride,
                    format,
                    aShmem);
        
        return (s->CairoStatus() != 0) ? nullptr : s.forget();
    }

    template<class ShmemAllocator>
    static already_AddRefed<Sub>
    CreateUnsafe(ShmemAllocator* aAllocator,
                 const gfxIntSize& aSize,
                 gfxASurface::gfxImageFormat aFormat,
                 SharedMemory::SharedMemoryType aShmType = SharedMemory::TYPE_BASIC)
    {
        return Create<ShmemAllocator, true>(aAllocator, aSize, aFormat, aShmType);
    }

    Shmem& GetShmem() { return mShmem; }

    static bool IsSharedImage(gfxASurface *aSurface)
    {
        return (aSurface
                && aSurface->GetType() == gfxASurface::SurfaceTypeImage
                && aSurface->GetData(&SHM_KEY));
    }

protected:
    gfxBaseSharedMemorySurface(const gfxIntSize& aSize, long aStride, 
                               gfxASurface::gfxImageFormat aFormat, 
                               const Shmem& aShmem)
      : Base(aShmem.get<unsigned char>(), aSize, aStride, aFormat)
    {
        MOZ_COUNT_CTOR(gfxBaseSharedMemorySurface);

        mShmem = aShmem;
        this->SetData(&SHM_KEY, this, nullptr);
    }

private:
    void WriteShmemInfo()
    {
        SharedImageInfo* shmInfo = GetShmInfoPtr(mShmem);
        shmInfo->width = this->mSize.width;
        shmInfo->height = this->mSize.height;
        shmInfo->format = this->mFormat;
    }

    static size_t GetAlignedSize(const gfxIntSize& aSize, long aStride)
    {
        #define MOZ_ALIGN_WORD(x) (((x) + 3) & ~3)
        return MOZ_ALIGN_WORD(sizeof(SharedImageInfo) + aSize.height * aStride);
    }

    template<class ShmemAllocator, bool Unsafe>
    static already_AddRefed<Sub>
    Create(ShmemAllocator* aAllocator,
           const gfxIntSize& aSize,
           gfxASurface::gfxImageFormat aFormat,
           SharedMemory::SharedMemoryType aShmType)
    {
        if (!gfxASurface::CheckSurfaceSize(aSize))
            return nullptr;

        Shmem shmem;
        long stride = gfxImageSurface::ComputeStride(aSize, aFormat);
        size_t size = GetAlignedSize(aSize, stride);
        if (!Unsafe) {
            if (!aAllocator->AllocShmem(size, aShmType, &shmem))
                return nullptr;
        } else {
            if (!aAllocator->AllocUnsafeShmem(size, aShmType, &shmem))
                return nullptr;
        }

        nsRefPtr<Sub> s =
            new Sub(aSize, stride, aFormat, shmem);
        if (s->CairoStatus() != 0) {
            aAllocator->DeallocShmem(shmem);
            return nullptr;
        }
        s->WriteShmemInfo();
        return s.forget();
    }

    Shmem mShmem;

    
    gfxBaseSharedMemorySurface(const gfxBaseSharedMemorySurface&);
    gfxBaseSharedMemorySurface& operator=(const gfxBaseSharedMemorySurface&);
};

#endif 
