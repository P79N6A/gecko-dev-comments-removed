




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
  aOutDescriptor = SharedTextureDescriptor(mShareType, mHandle, mSize, mInverted);
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
SharedTextureClientOGL::Lock(OpenMode mode)
{
  MOZ_ASSERT(!mIsLocked);
  if (!IsValid() || !IsAllocated()) {
    return false;
  }
  mIsLocked = true;
  return true;
}

void
SharedTextureClientOGL::Unlock()
{
  MOZ_ASSERT(mIsLocked);
  mIsLocked = false;
}

bool
SharedTextureClientOGL::IsAllocated() const
{
  return mHandle != 0;
}

StreamTextureClientOGL::StreamTextureClientOGL(TextureFlags aFlags)
  : TextureClient(aFlags)
  , mIsLocked(false)
{
}

StreamTextureClientOGL::~StreamTextureClientOGL()
{
  
}

bool
StreamTextureClientOGL::Lock(OpenMode mode)
{
  MOZ_ASSERT(!mIsLocked);
  if (!IsValid() || !IsAllocated()) {
    return false;
  }
  mIsLocked = true;
  return true;
}

void
StreamTextureClientOGL::Unlock()
{
  MOZ_ASSERT(mIsLocked);
  mIsLocked = false;
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
  mGL = mStream->GLContext();
}

bool
StreamTextureClientOGL::IsAllocated() const
{
  return mStream != 0;
}


} 
} 
