




#include "mozilla/layers/TextureClientOGL.h"
#include "SurfaceStream.h"
#include "GLContext.h"                  
#include "mozilla/Assertions.h"         
#include "mozilla/layers/ISurfaceAllocator.h"
#include "nsSize.h"                     

using namespace mozilla::gl;

namespace mozilla {
namespace layers {

class CompositableForwarder;

SharedTextureClientOGL::SharedTextureClientOGL(TextureFlags aFlags)
  : TextureClient(aFlags)
  , mHandle(0)
  , mInverted(false)
{
  
  mFlags |= TEXTURE_DEALLOCATE_CLIENT;
}

SharedTextureClientOGL::~SharedTextureClientOGL()
{
  
}


bool
SharedTextureClientOGL::ToSurfaceDescriptor(SurfaceDescriptor& aOutDescriptor)
{
  MOZ_ASSERT(IsValid());
  if (!IsAllocated()) {
    return false;
  }
  nsIntSize nsSize(mSize.width, mSize.height);
  aOutDescriptor = SharedTextureDescriptor(mShareType, mHandle, nsSize, mInverted);
  return true;
}

void
SharedTextureClientOGL::InitWith(gl::SharedTextureHandle aHandle,
                                 gfx::IntSize aSize,
                                 gl::SharedTextureShareType aShareType,
                                 bool aInverted)
{
  MOZ_ASSERT(IsValid());
  MOZ_ASSERT(!IsAllocated());
  mHandle = aHandle;
  mSize = aSize;
  mShareType = aShareType;
  mInverted = aInverted;
  if (mInverted) {
    AddFlags(TEXTURE_NEEDS_Y_FLIP);
  }
}

bool
SharedTextureClientOGL::IsAllocated() const
{
  return mHandle != 0;
}

StreamTextureClientOGL::StreamTextureClientOGL(TextureFlags aFlags)
  : TextureClient(aFlags)
  , mStream(0)
{
}

StreamTextureClientOGL::~StreamTextureClientOGL()
{
  
}

bool
StreamTextureClientOGL::ToSurfaceDescriptor(SurfaceDescriptor& aOutDescriptor)
{
  if (!IsAllocated()) {
    return false;
  }

  gfx::SurfaceStreamHandle handle = mStream->GetShareHandle();
  aOutDescriptor = SurfaceStreamDescriptor(handle, false);
  return true;
}

void
StreamTextureClientOGL::InitWith(gfx::SurfaceStream* aStream)
{
  MOZ_ASSERT(!IsAllocated());
  mStream = aStream;
}

bool
StreamTextureClientOGL::IsAllocated() const
{
  return mStream != 0;
}

DeprecatedTextureClientSharedOGL::DeprecatedTextureClientSharedOGL(CompositableForwarder* aForwarder,
                                               const TextureInfo& aTextureInfo)
  : DeprecatedTextureClient(aForwarder, aTextureInfo)
  , mGL(nullptr)
{
}

void
DeprecatedTextureClientSharedOGL::ReleaseResources()
{
  if (!IsSurfaceDescriptorValid(mDescriptor)) {
    return;
  }
  MOZ_ASSERT(mDescriptor.type() == SurfaceDescriptor::TSharedTextureDescriptor);
  mDescriptor = SurfaceDescriptor();
  
  
}

bool
DeprecatedTextureClientSharedOGL::EnsureAllocated(gfx::IntSize aSize,
                                        gfxContentType aContentType)
{
  mSize = aSize;
  return true;
}


} 
} 
