




#include "mozilla/layers/TextureClientOGL.h"
#include "mozilla/layers/CompositableClient.h"
#include "mozilla/layers/CompositableForwarder.h"
#include "GLContext.h"
#include "gfxipc/ShadowLayerUtils.h"

using namespace mozilla::gl;

namespace mozilla {
namespace layers {

SharedTextureClientOGL::SharedTextureClientOGL()
: mHandle(0), mIsCrossProcess(false), mInverted(false)
{
}

SharedTextureClientOGL::~SharedTextureClientOGL()
{
  
}


bool
SharedTextureClientOGL::ToSurfaceDescriptor(SurfaceDescriptor& aOutDescriptor)
{
  if (!IsAllocated()) {
    return false;
  }
  nsIntSize nsSize(mSize.width, mSize.height);
  aOutDescriptor = SharedTextureDescriptor(mIsCrossProcess ? gl::GLContext::CrossProcess
                                                           : gl::GLContext::SameProcess,
                                           mHandle, nsSize, mInverted);
  return true;
}

void
SharedTextureClientOGL::InitWith(gl::SharedTextureHandle aHandle,
                                 gfx::IntSize aSize,
                                 bool aIsCrossProcess,
                                 bool aInverted)
{
  MOZ_ASSERT(!IsAllocated());
  mHandle = aHandle;
  mSize = aSize;
  mIsCrossProcess = aIsCrossProcess;
  mInverted = aInverted;
}

bool
SharedTextureClientOGL::IsAllocated() const
{
  return mHandle != 0;
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

void
DeprecatedTextureClientSharedOGL::EnsureAllocated(gfx::IntSize aSize,
                                        gfxASurface::gfxContentType aContentType)
{
  mSize = aSize;
}


} 
} 
