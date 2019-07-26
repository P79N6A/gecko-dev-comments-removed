




#include "mozilla/layers/CanvasClient.h"
#include "ClientCanvasLayer.h"          
#include "GLContext.h"                  
#include "GLScreenBuffer.h"             
#include "SurfaceStream.h"              
#include "SurfaceTypes.h"               
#include "gfx2DGlue.h"                  
#include "gfxASurface.h"                
#include "gfxPlatform.h"                
#include "mozilla/gfx/BaseSize.h"       
#include "mozilla/layers/CompositableForwarder.h"
#include "mozilla/layers/GrallocTextureClient.h"
#include "mozilla/layers/LayersTypes.h"
#include "mozilla/layers/TextureClient.h"  
#include "mozilla/layers/TextureClientOGL.h"
#include "nsAutoPtr.h"                  
#include "nsDebug.h"                    
#include "nsXULAppAPI.h"                
#ifdef MOZ_WIDGET_GONK
#include "SharedSurfaceGralloc.h"
#endif

using namespace mozilla::gfx;
using namespace mozilla::gl;

namespace mozilla {
namespace layers {

 TemporaryRef<CanvasClient>
CanvasClient::CreateCanvasClient(CanvasClientType aType,
                                 CompositableForwarder* aForwarder,
                                 TextureFlags aFlags)
{
  if (aType == CanvasClientGLContext &&
      aForwarder->GetCompositorBackendType() == LayersBackend::LAYERS_OPENGL) {
    aFlags |= TEXTURE_DEALLOCATE_CLIENT;
    return new CanvasClientSurfaceStream(aForwarder, aFlags);
  }
  if (gfxPlatform::GetPlatform()->UseDeprecatedTextures()) {
    aFlags |= TEXTURE_DEALLOCATE_CLIENT;
    return new DeprecatedCanvasClient2D(aForwarder, aFlags);
  }
  return new CanvasClient2D(aForwarder, aFlags);
}

void
CanvasClient2D::Update(gfx::IntSize aSize, ClientCanvasLayer* aLayer)
{
  if (mBuffer &&
      (mBuffer->IsImmutable() || mBuffer->GetSize() != aSize)) {
    GetForwarder()->RemoveTextureFromCompositable(this, mBuffer);
    mBuffer = nullptr;
  }

  bool bufferCreated = false;
  if (!mBuffer) {
    bool isOpaque = (aLayer->GetContentFlags() & Layer::CONTENT_OPAQUE);
    gfxContentType contentType = isOpaque
                                                ? gfxContentType::COLOR
                                                : gfxContentType::COLOR_ALPHA;
    gfxImageFormat format
      = gfxPlatform::GetPlatform()->OptimalFormatForContent(contentType);
    mBuffer = CreateBufferTextureClient(gfx::ImageFormatToSurfaceFormat(format),
                                        TEXTURE_FLAGS_DEFAULT,
                                        gfxPlatform::GetPlatform()->GetPreferredCanvasBackend());
    MOZ_ASSERT(mBuffer->AsTextureClientSurface());
    mBuffer->AsTextureClientSurface()->AllocateForSurface(aSize);

    bufferCreated = true;
  }

  if (!mBuffer->Lock(OPEN_WRITE_ONLY)) {
    return;
  }

  bool updated = false;
  {
    
    RefPtr<DrawTarget> drawTarget =
      mBuffer->AsTextureClientDrawTarget()->GetAsDrawTarget();
    if (drawTarget) {
      aLayer->UpdateTarget(drawTarget);
      updated = true;
    }
  }
  mBuffer->Unlock();

  if (bufferCreated && !AddTextureClient(mBuffer)) {
    mBuffer = nullptr;
    return;
  }

  if (updated) {
    GetForwarder()->UpdatedTexture(this, mBuffer, nullptr);
    GetForwarder()->UseTexture(this, mBuffer);
  }
}

TemporaryRef<BufferTextureClient>
CanvasClient2D::CreateBufferTextureClient(gfx::SurfaceFormat aFormat, TextureFlags aFlags,
                                          gfx::BackendType aMoz2DBackend)
{
  return CompositableClient::CreateBufferTextureClient(aFormat,
                                                       mTextureInfo.mTextureFlags | aFlags,
                                                       aMoz2DBackend);
}

CanvasClientSurfaceStream::CanvasClientSurfaceStream(CompositableForwarder* aLayerForwarder,
                                                     TextureFlags aFlags)
  : CanvasClient(aLayerForwarder, aFlags)
{
}

void
CanvasClientSurfaceStream::Update(gfx::IntSize aSize, ClientCanvasLayer* aLayer)
{
  GLScreenBuffer* screen = aLayer->mGLContext->Screen();
  SurfaceStream* stream = screen->Stream();

  bool isCrossProcess = !(XRE_GetProcessType() == GeckoProcessType_Default);
  bool bufferCreated = false;
  if (isCrossProcess) {
#ifdef MOZ_WIDGET_GONK
    SharedSurface* surf = stream->SwapConsumer();
    if (!surf) {
      printf_stderr("surf is null post-SwapConsumer!\n");
      return;
    }

    if (surf->Type() != SharedSurfaceType::Gralloc) {
      printf_stderr("Unexpected non-Gralloc SharedSurface in IPC path!");
      MOZ_ASSERT(false);
      return;
    }

    SharedSurface_Gralloc* grallocSurf = SharedSurface_Gralloc::Cast(surf);

    RefPtr<GrallocTextureClientOGL> grallocTextureClient =
      static_cast<GrallocTextureClientOGL*>(grallocSurf->GetTextureClient());

    
    if (!grallocTextureClient->GetIPDLActor()) {
      grallocTextureClient->SetTextureFlags(mTextureInfo.mTextureFlags);
      AddTextureClient(grallocTextureClient);
    }

    if (grallocTextureClient->GetIPDLActor()) {
      GetForwarder()->UseTexture(this, grallocTextureClient);
    }
#else
    printf_stderr("isCrossProcess, but not MOZ_WIDGET_GONK! Someone needs to write some code!");
    MOZ_ASSERT(false);
#endif
  } else {
    if (!mBuffer) {
      StreamTextureClientOGL* textureClient =
        new StreamTextureClientOGL(mTextureInfo.mTextureFlags);
      textureClient->InitWith(stream);
      mBuffer = textureClient;
      bufferCreated = true;
    }

    if (bufferCreated && !AddTextureClient(mBuffer)) {
      mBuffer = nullptr;
    }

    if (mBuffer) {
      GetForwarder()->UseTexture(this, mBuffer);
    }
  }

  aLayer->Painted();
}

void
DeprecatedCanvasClient2D::Updated()
{
  mForwarder->UpdateTexture(this, 1, mDeprecatedTextureClient->LockSurfaceDescriptor());
}


DeprecatedCanvasClient2D::DeprecatedCanvasClient2D(CompositableForwarder* aFwd,
                                                   TextureFlags aFlags)
: CanvasClient(aFwd, aFlags)
{
  mTextureInfo.mCompositableType = BUFFER_IMAGE_SINGLE;
}

void
DeprecatedCanvasClient2D::Update(gfx::IntSize aSize, ClientCanvasLayer* aLayer)
{
  bool isOpaque = (aLayer->GetContentFlags() & Layer::CONTENT_OPAQUE);
  gfxContentType contentType = isOpaque
                                              ? gfxContentType::COLOR
                                              : gfxContentType::COLOR_ALPHA;

  if (!mDeprecatedTextureClient) {
    mDeprecatedTextureClient = CreateDeprecatedTextureClient(TEXTURE_CONTENT, contentType);
    if (!mDeprecatedTextureClient) {
      mDeprecatedTextureClient = CreateDeprecatedTextureClient(TEXTURE_FALLBACK, contentType);
      if (!mDeprecatedTextureClient) {
        NS_WARNING("Could not create texture client");
        return;
      }
    }
  }

  if (!mDeprecatedTextureClient->EnsureAllocated(aSize, contentType)) {
    
    
    
    
    mDeprecatedTextureClient = CreateDeprecatedTextureClient(TEXTURE_FALLBACK, contentType);
    MOZ_ASSERT(mDeprecatedTextureClient, "Failed to create texture client");
    if (!mDeprecatedTextureClient->EnsureAllocated(aSize, contentType)) {
      NS_WARNING("Could not allocate texture client");
      return;
    }
  }

  gfxASurface* surface = mDeprecatedTextureClient->LockSurface();
  aLayer->DeprecatedUpdateSurface(surface);
  mDeprecatedTextureClient->Unlock();
}

void
DeprecatedCanvasClientSurfaceStream::Updated()
{
  mForwarder->UpdateTextureNoSwap(this, 1, mDeprecatedTextureClient->LockSurfaceDescriptor());
}


DeprecatedCanvasClientSurfaceStream::DeprecatedCanvasClientSurfaceStream(CompositableForwarder* aFwd,
                                                                         TextureFlags aFlags)
: CanvasClient(aFwd, aFlags)
{
  mTextureInfo.mCompositableType = BUFFER_IMAGE_SINGLE;
}

void
DeprecatedCanvasClientSurfaceStream::Update(gfx::IntSize aSize, ClientCanvasLayer* aLayer)
{
  if (!mDeprecatedTextureClient) {
    mDeprecatedTextureClient = CreateDeprecatedTextureClient(TEXTURE_STREAM_GL,
                                                             aLayer->GetSurfaceMode() == SurfaceMode::SURFACE_OPAQUE
                                                               ? gfxContentType::COLOR
                                                               : gfxContentType::COLOR_ALPHA);
    MOZ_ASSERT(mDeprecatedTextureClient, "Failed to create texture client");
  }

  NS_ASSERTION(aLayer->mGLContext, "CanvasClientSurfaceStream should only be used with GL canvases");

  
  mDeprecatedTextureClient->EnsureAllocated(aSize, gfxContentType::COLOR);

  GLScreenBuffer* screen = aLayer->mGLContext->Screen();
  SurfaceStream* stream = screen->Stream();

  bool isCrossProcess = !(XRE_GetProcessType() == GeckoProcessType_Default);
  if (isCrossProcess) {
    
    SharedSurface* surf = stream->SwapConsumer();
    if (!surf) {
      printf_stderr("surf is null post-SwapConsumer!\n");
      return;
    }

#ifdef MOZ_WIDGET_GONK
    if (surf->Type() != SharedSurfaceType::Gralloc) {
      printf_stderr("Unexpected non-Gralloc SharedSurface in IPC path!");
      return;
    }

    SharedSurface_Gralloc* grallocSurf = SharedSurface_Gralloc::Cast(surf);
    
    
#else
    printf_stderr("isCrossProcess, but not MOZ_WIDGET_GONK! Someone needs to write some code!");
    MOZ_ASSERT(false);
#endif
  } else {
    SurfaceStreamHandle handle = stream->GetShareHandle();
    mDeprecatedTextureClient->SetDescriptor(SurfaceStreamDescriptor(handle, false));

    
    
    
    
    aLayer->mGLContext->AddRef();
  }

  aLayer->Painted();
}

}
}
