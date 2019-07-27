




#include "CanvasClient.h"

#include "ClientCanvasLayer.h"          
#include "CompositorChild.h"            
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
  if (aType == CanvasClientGLContext) {
    aFlags |= TextureFlags::DEALLOCATE_CLIENT;
    return new CanvasClientSurfaceStream(aForwarder, aFlags);
  }
  return new CanvasClient2D(aForwarder, aFlags);
}

void
CanvasClient2D::Update(gfx::IntSize aSize, ClientCanvasLayer* aLayer)
{
  AutoRemoveTexture autoRemove(this);
  if (mBuffer &&
      (mBuffer->IsImmutable() || mBuffer->GetSize() != aSize)) {
    autoRemove.mTexture = mBuffer;
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

    gfx::SurfaceFormat surfaceFormat = gfx::ImageFormatToSurfaceFormat(format);
    mBuffer = CreateTextureClientForCanvas(surfaceFormat, aSize, flags, aLayer);
    if (!mBuffer) {
      NS_WARNING("Failed to allocate the TextureClient");
      return;
    }
    MOZ_ASSERT(mBuffer->CanExposeDrawTarget());

    bufferCreated = true;
  }

  if (!mBuffer->Lock(OpenMode::OPEN_WRITE_ONLY)) {
    mBuffer = nullptr;
    return;
  }

  bool updated = false;
  {
    
    RefPtr<DrawTarget> target =
      mBuffer->BorrowDrawTarget();
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

TemporaryRef<TextureClient>
CanvasClient2D::CreateTextureClientForCanvas(gfx::SurfaceFormat aFormat,
                                             gfx::IntSize aSize,
                                             TextureFlags aFlags,
                                             ClientCanvasLayer* aLayer)
{
  if (aLayer->IsGLLayer()) {
    
    
    
    return TextureClient::CreateForRawBufferAccess(GetForwarder(),
                                                   aFormat, aSize, BackendType::CAIRO,
                                                   mTextureInfo.mTextureFlags | aFlags);
  }

  gfx::BackendType backend = gfxPlatform::GetPlatform()->GetPreferredCanvasBackend();
#ifdef XP_WIN
  return CreateTextureClientForDrawing(aFormat, aSize, backend, aFlags);
#else
  
  
  return TextureClient::CreateForRawBufferAccess(GetForwarder(),
                                                 aFormat, aSize, backend,
                                                 mTextureInfo.mTextureFlags | aFlags);
#endif
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

    
    
    stream->CopySurfaceToProducer(aLayer->mTextureSurface.get(),
                                  aLayer->mFactory.get());
    stream->SwapProducer(aLayer->mFactory.get(),
                         gfx::IntSize(aSize.width, aSize.height));
  } else {
    stream = screen->Stream();
  }

#ifdef MOZ_WIDGET_GONK
  SharedSurface* surf = stream->SwapConsumer();
  if (!surf) {
    printf_stderr("surf is null post-SwapConsumer!\n");
    return;
  }

  if (surf->mType != SharedSurfaceType::Gralloc) {
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
    UseTexture(grallocTextureClient);
  }

  if (mBuffer) {
    
    RefPtr<AsyncTransactionTracker> tracker = new RemoveTextureFromCompositableTracker();
    
    tracker->SetTextureClient(mBuffer);
    mBuffer->SetRemoveFromCompositableTracker(tracker);
    
    GetForwarder()->RemoveTextureFromCompositableAsync(tracker, this, mBuffer);
  }
  mBuffer = grallocTextureClient;
#else
  bool isCrossProcess = !(XRE_GetProcessType() == GeckoProcessType_Default);
  if (isCrossProcess) {
    printf_stderr("isCrossProcess, but not MOZ_WIDGET_GONK! Someone needs to write some code!");
    MOZ_ASSERT(false);
  } else {
    bool bufferCreated = false;
    if (!mBuffer) {
      StreamTextureClient* textureClient =
        new StreamTextureClient(mTextureInfo.mTextureFlags);
      textureClient->InitWith(stream);
      mBuffer = textureClient;
      bufferCreated = true;
    }

    if (bufferCreated && !AddTextureClient(mBuffer)) {
      mBuffer = nullptr;
    }

    if (mBuffer) {
      GetForwarder()->UpdatedTexture(this, mBuffer, nullptr);
      GetForwarder()->UseTexture(this, mBuffer);
    }
  }
#endif

  aLayer->Painted();
}

}
}
