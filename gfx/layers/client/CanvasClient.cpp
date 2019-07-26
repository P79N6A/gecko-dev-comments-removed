




#include "mozilla/layers/CanvasClient.h"
#include "ClientCanvasLayer.h"          
#include "GLContext.h"                  
#include "GLScreenBuffer.h"             
#include "SurfaceStream.h"              
#include "SurfaceTypes.h"               
#include "gfx2DGlue.h"                  
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
#ifndef MOZ_WIDGET_GONK
  if (XRE_GetProcessType() != GeckoProcessType_Default) {
    NS_WARNING("Most platforms still need an optimized way to share GL cross process.");
    return new CanvasClient2D(aForwarder, aFlags);
  }
#endif
  if (aType == CanvasClientGLContext &&
      aForwarder->GetCompositorBackendType() == LayersBackend::LAYERS_OPENGL) {
    aFlags |= TextureFlags::DEALLOCATE_CLIENT;
    return new CanvasClientSurfaceStream(aForwarder, aFlags);
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
    TextureFlags flags = TextureFlags::DEFAULT;
    if (mTextureFlags & TextureFlags::NEEDS_Y_FLIP) {
      flags |= TextureFlags::NEEDS_Y_FLIP;
    }
    mBuffer = CreateTextureClientForDrawing(gfx::ImageFormatToSurfaceFormat(format),
                                            flags,
                                            gfxPlatform::GetPlatform()->GetPreferredCanvasBackend(),
                                            aSize);
    MOZ_ASSERT(mBuffer->CanExposeDrawTarget());
    mBuffer->AllocateForSurface(aSize);

    bufferCreated = true;
  }

  if (!mBuffer->Lock(OpenMode::OPEN_WRITE_ONLY)) {
    mBuffer = nullptr;
    return;
  }

  bool updated = false;
  {
    
    RefPtr<DrawTarget> target =
      mBuffer->GetAsDrawTarget();
    if (target) {
      aLayer->UpdateTarget(target);
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

CanvasClientSurfaceStream::CanvasClientSurfaceStream(CompositableForwarder* aLayerForwarder,
                                                     TextureFlags aFlags)
  : CanvasClient(aLayerForwarder, aFlags)
{
}

void
CanvasClientSurfaceStream::Update(gfx::IntSize aSize, ClientCanvasLayer* aLayer)
{
  aLayer->mGLContext->MakeCurrent();
  GLScreenBuffer* screen = aLayer->mGLContext->Screen();
  SurfaceStream* stream = nullptr;

  if (aLayer->mStream) {
    stream = aLayer->mStream;

    
    
    stream->CopySurfaceToProducer(aLayer->mTextureSurface, aLayer->mFactory);
    stream->SwapProducer(aLayer->mFactory, gfx::IntSize(aSize.width, aSize.height));
  } else {
    stream = screen->Stream();
  }

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

}
}
