





































#ifndef AndroidBackingSurface_h__
#define AndroidBackingSurface_h__

#include "gfxImageSurface.h"
#include "gfxPoint.h"

typedef void* EGLClientBuffer;
typedef void* EGLImage;
typedef void* EGLSurface;

namespace mozilla {

void CreateAndroidBackingSurface(const gfxIntSize& aSize, nsRefPtr<gfxASurface>& aBackingSurface,
                                 EGLSurface& aSurface, EGLClientBuffer& aBuffer);
already_AddRefed<gfxImageSurface> LockAndroidBackingBuffer(EGLClientBuffer aBuffer,
                                                           const gfxIntSize& aSize);
void UnlockAndroidBackingBuffer(EGLClientBuffer aBuffer);

}   

#endif

