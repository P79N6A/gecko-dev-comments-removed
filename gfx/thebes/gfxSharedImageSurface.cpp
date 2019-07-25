





































#include "base/basictypes.h"
#include "gfxSharedImageSurface.h"
#include "cairo.h"

#define MOZ_ALIGN_WORD(x) (((x) + 3) & ~3)

using mozilla::ipc::SharedMemory;

static const cairo_user_data_key_t SHM_KEY;

typedef struct _SharedImageInfo
{
    PRInt32 width;
    PRInt32 height;
    PRInt32 format;
} SharedImageInfo;

static SharedImageInfo*
GetShmInfoPtr(const mozilla::ipc::Shmem &aShmem)
{
    return reinterpret_cast<SharedImageInfo*>
        (aShmem.get<char>() + aShmem.Size<char>() - sizeof(SharedImageInfo));
}

size_t
gfxSharedImageSurface::GetAlignedSize()
{
   return MOZ_ALIGN_WORD(sizeof(SharedImageInfo) + mSize.height * mStride);
}

bool
gfxSharedImageSurface::InitSurface(PRBool aUpdateShmemInfo)
{
    if (!CheckSurfaceSize(mSize))
        return false;

    cairo_surface_t *surface =
        cairo_image_surface_create_for_data(mShmem.get<unsigned char>(),
                                            (cairo_format_t)mFormat,
                                            mSize.width,
                                            mSize.height,
                                            mStride);

    if (!surface)
        return false;

    cairo_surface_set_user_data(surface,
                                &SHM_KEY,
                                this, NULL);

    if (aUpdateShmemInfo) {
        SharedImageInfo *shmInfo = GetShmInfoPtr(mShmem);
        shmInfo->width = mSize.width;
        shmInfo->height = mSize.height;
        shmInfo->format = mFormat;
    }

    InitFromSurface(surface);
    return true;
}

gfxSharedImageSurface::~gfxSharedImageSurface()
{
}

gfxSharedImageSurface::gfxSharedImageSurface()
{
}

gfxSharedImageSurface::gfxSharedImageSurface(const mozilla::ipc::Shmem &aShmem)
{
    mShmem = aShmem;
    SharedImageInfo *shmInfo = GetShmInfoPtr(aShmem);
    mSize.width = shmInfo->width;
    mSize.height = shmInfo->height;
    mFormat = (gfxImageFormat)shmInfo->format;
    mStride = ComputeStride();

    if (!InitSurface(PR_FALSE))
        NS_RUNTIMEABORT("Shared memory is bad");
}

PRBool
gfxSharedImageSurface::IsSharedImage(gfxASurface *aSurface)
{
    return (aSurface
            && aSurface->GetType() == gfxASurface::SurfaceTypeImage
            && aSurface->GetData(&SHM_KEY));
}
