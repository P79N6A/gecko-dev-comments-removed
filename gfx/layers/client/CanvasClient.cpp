




#include "mozilla/layers/CanvasClient.h"
#include "mozilla/layers/TextureClient.h"
#include "ClientCanvasLayer.h"
#include "mozilla/layers/ShadowLayers.h"
#include "SharedTextureImage.h"
#include "nsXULAppAPI.h"
#include "GLContext.h"
#include "SurfaceStream.h"

using namespace mozilla::gl;

namespace mozilla {
namespace layers {

 TemporaryRef<CanvasClient>
CanvasClient::CreateCanvasClient(CompositableType aCompositableHostType,
                                 CompositableForwarder* aForwarder,
                                 TextureFlags aFlags)
{
  if (aCompositableHostType == BUFFER_IMAGE_SINGLE) {
    return new CanvasClient2D(aForwarder, aFlags);
  }
  if (aCompositableHostType == BUFFER_IMAGE_BUFFERED) {
    if (aForwarder->GetCompositorBackendType() == LAYERS_OPENGL) {
      return new CanvasClientWebGL(aForwarder, aFlags);
    }
    return new CanvasClient2D(aForwarder, aFlags);
  }
  return nullptr;
}

void
CanvasClient::Updated()
{
  mForwarder->UpdateTexture(this, 1, mTextureClient->GetDescriptor());
}


CanvasClient2D::CanvasClient2D(CompositableForwarder* aFwd,
                               TextureFlags aFlags)
: CanvasClient(aFwd, aFlags)
{
  mTextureInfo.mCompositableType = BUFFER_IMAGE_SINGLE;
}

void
CanvasClient2D::Update(gfx::IntSize aSize, ClientCanvasLayer* aLayer)
{
  if (!mTextureClient) {
    mTextureClient = CreateTextureClient(TEXTURE_SHMEM);
    MOZ_ASSERT(mTextureClient, "Failed to create texture client");
  }

  bool isOpaque = (aLayer->GetContentFlags() & Layer::CONTENT_OPAQUE);
  gfxASurface::gfxContentType contentType = isOpaque
                                              ? gfxASurface::CONTENT_COLOR
                                              : gfxASurface::CONTENT_COLOR_ALPHA;
  mTextureClient->EnsureAllocated(aSize, contentType);

  gfxASurface* surface = mTextureClient->LockSurface();
  aLayer->UpdateSurface(surface);
  mTextureClient->Unlock();
}

CanvasClientWebGL::CanvasClientWebGL(CompositableForwarder* aFwd,
                                     TextureFlags aFlags)
: CanvasClient(aFwd, aFlags)
{
  mTextureInfo.mCompositableType = BUFFER_IMAGE_BUFFERED;
}

void
CanvasClientWebGL::Update(gfx::IntSize aSize, ClientCanvasLayer* aLayer)
{
  if (!mTextureClient) {
    mTextureClient = CreateTextureClient(TEXTURE_STREAM_GL);
    MOZ_ASSERT(mTextureClient, "Failed to create texture client");
  }

  NS_ASSERTION(aLayer->mGLContext, "CanvasClientWebGL should only be used with GL canvases");

  
  mTextureClient->EnsureAllocated(aSize, gfxASurface::CONTENT_COLOR);

  GLScreenBuffer* screen = aLayer->mGLContext->Screen();
  SurfaceStreamHandle handle = screen->Stream()->GetShareHandle();

  mTextureClient->SetDescriptor(SurfaceStreamDescriptor(handle, false));

  aLayer->Painted();
}

}
}
