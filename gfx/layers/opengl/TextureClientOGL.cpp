




#include "GLContext.h"                  
#include "SurfaceStream.h"
#include "mozilla/Assertions.h"         
#include "mozilla/layers/ISurfaceAllocator.h"
#include "mozilla/layers/TextureClientOGL.h"
#include "nsSize.h"                     

using namespace mozilla::gl;

namespace mozilla {
namespace layers {

class CompositableForwarder;




EGLImageTextureClient::EGLImageTextureClient(TextureFlags aFlags,
                                             EGLImage aImage,
                                             gfx::IntSize aSize,
                                             bool aInverted)
  : TextureClient(aFlags)
  , mImage(aImage)
  , mSize(aSize)
  , mIsLocked(false)
{
  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Default,
             "Can't pass an `EGLImage` between processes.");
  
  
  AddFlags(TextureFlags::DEALLOCATE_CLIENT);
  
  if (aInverted) {
    AddFlags(TextureFlags::NEEDS_Y_FLIP);
  }
}
  
EGLImageTextureClient::~EGLImageTextureClient()
{
  
}

bool
EGLImageTextureClient::ToSurfaceDescriptor(SurfaceDescriptor& aOutDescriptor)
{
  MOZ_ASSERT(IsValid());
  MOZ_ASSERT(IsAllocated());

  aOutDescriptor = EGLImageDescriptor((uintptr_t)mImage, mSize);
  return true;
}

bool
EGLImageTextureClient::Lock(OpenMode mode)
  {
    MOZ_ASSERT(!mIsLocked);
    if (!IsValid() || !IsAllocated()) {
      return false;
    }
    mIsLocked = true;
    return true;
  }
  
void
EGLImageTextureClient::Unlock()
{
  MOZ_ASSERT(mIsLocked);
  mIsLocked = false;
}
  



#ifdef MOZ_WIDGET_ANDROID

SurfaceTextureClient::SurfaceTextureClient(TextureFlags aFlags,
                                           nsSurfaceTexture* aSurfTex,
                                           gfx::IntSize aSize,
                                           bool aInverted)
  : TextureClient(aFlags)
  , mSurfTex(aSurfTex)
  , mSize(aSize)
  , mIsLocked(false)
{
  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Default,
             "Can't pass pointers between processes.");

  
  AddFlags(TextureFlags::DEALLOCATE_CLIENT);

  if (aInverted) {
    AddFlags(TextureFlags::NEEDS_Y_FLIP);
  }
}
  
SurfaceTextureClient::~SurfaceTextureClient()
{
  
}

bool
SurfaceTextureClient::ToSurfaceDescriptor(SurfaceDescriptor& aOutDescriptor)
{
  MOZ_ASSERT(IsValid());
  MOZ_ASSERT(IsAllocated());

  aOutDescriptor = SurfaceTextureDescriptor((uintptr_t)mSurfTex.get(),
                                            mSize);
  return true;
}

bool
SurfaceTextureClient::Lock(OpenMode mode)
{
  MOZ_ASSERT(!mIsLocked);
  if (!IsValid() || !IsAllocated()) {
    return false;
  }
  mIsLocked = true;
  return true;
}

void
SurfaceTextureClient::Unlock()
{
  MOZ_ASSERT(mIsLocked);
  mIsLocked = false;
}

#endif 

} 
} 
