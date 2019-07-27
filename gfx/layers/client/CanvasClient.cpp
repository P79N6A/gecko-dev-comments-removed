




#include "CanvasClient.h"

#include "ClientCanvasLayer.h"          
#include "CompositorChild.h"            
#include "GLContext.h"                  
#include "GLScreenBuffer.h"             
#include "ScopedGLHelpers.h"
#include "SurfaceStream.h"              
#include "SurfaceTypes.h"               
#include "gfx2DGlue.h"                  
#include "gfxPlatform.h"                
#include "GLReadTexImageHelper.h"
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

  switch (aType) {
  case CanvasClientTypeShSurf:
    return new CanvasClientSharedSurface(aForwarder, aFlags);

  case CanvasClientGLContext:
    aFlags |= TextureFlags::DEALLOCATE_CLIENT;
    return new CanvasClientSurfaceStream(aForwarder, aFlags);

  default:
    return new CanvasClient2D(aForwarder, aFlags);
  }
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

  SurfaceStream* stream = aLayer->mStream;
  MOZ_ASSERT(stream);

  
  
  stream->CopySurfaceToProducer(aLayer->mTextureSurface.get(),
                                aLayer->mFactory.get());
  stream->SwapProducer(aLayer->mFactory.get(),
                       gfx::IntSize(aSize.width, aSize.height));

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
      
      TextureFlags flags = GetTextureFlags() |
                           TextureFlags::DEALLOCATE_CLIENT;
      StreamTextureClient* texClient = new StreamTextureClient(flags);
      texClient->InitWith(stream);
      mBuffer = texClient;
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



CanvasClientSharedSurface::CanvasClientSharedSurface(CompositableForwarder* aLayerForwarder,
                                                     TextureFlags aFlags)
  : CanvasClient(aLayerForwarder, aFlags)
{
}




static TemporaryRef<TextureClient>
TexClientFromShSurf(SharedSurface* surf, TextureFlags flags)
{
  switch (surf->mType) {
    case SharedSurfaceType::Basic:
      return nullptr;

#ifdef MOZ_WIDGET_GONK
    case SharedSurfaceType::Gralloc:
      return GrallocTextureClientOGL::FromShSurf(surf, flags);
#endif

    default:
      return new SharedSurfaceTextureClient(flags, surf);
  }
}





static inline void SwapRB_R8G8B8A8(uint8_t* pixel) {
  
  Swap(pixel[0], pixel[2]);
}

class TexClientFactory
{
  ISurfaceAllocator* const mAllocator;
  const bool mHasAlpha;
  const gfx::IntSize mSize;
  const gfx::BackendType mBackendType;
  const TextureFlags mBaseTexFlags;
  const LayersBackend mLayersBackend;

public:
  TexClientFactory(ISurfaceAllocator* allocator, bool hasAlpha,
                   const gfx::IntSize& size, gfx::BackendType backendType,
                   TextureFlags baseTexFlags, LayersBackend layersBackend)
    : mAllocator(allocator)
    , mHasAlpha(hasAlpha)
    , mSize(size)
    , mBackendType(backendType)
    , mBaseTexFlags(baseTexFlags)
    , mLayersBackend(layersBackend)
  {
  }

protected:
  TemporaryRef<BufferTextureClient> Create(gfx::SurfaceFormat format) {
    return TextureClient::CreateForRawBufferAccess(mAllocator, format,
                                                   mSize, mBackendType,
                                                   mBaseTexFlags);
  }

public:
  TemporaryRef<BufferTextureClient> CreateB8G8R8AX8() {
    gfx::SurfaceFormat format = mHasAlpha ? gfx::SurfaceFormat::B8G8R8A8
                                          : gfx::SurfaceFormat::B8G8R8X8;
    return Create(format);
  }

  TemporaryRef<BufferTextureClient> CreateR8G8B8AX8() {
    RefPtr<BufferTextureClient> ret;

    bool areRGBAFormatsBroken = mLayersBackend == LayersBackend::LAYERS_BASIC;
    if (!areRGBAFormatsBroken) {
      gfx::SurfaceFormat format = mHasAlpha ? gfx::SurfaceFormat::R8G8B8A8
                                            : gfx::SurfaceFormat::R8G8B8X8;
      ret = Create(format);
    }

    if (!ret) {
      ret = CreateB8G8R8AX8();
      if (ret) {
        ret->AddFlags(TextureFlags::RB_SWAPPED);
      }
    }

    return ret.forget();
  }
};

static TemporaryRef<TextureClient>
TexClientFromReadback(SharedSurface* src, ISurfaceAllocator* allocator,
                      TextureFlags baseFlags, LayersBackend layersBackend)
{
  auto backendType = gfx::BackendType::CAIRO;
  TexClientFactory factory(allocator, src->mHasAlpha, src->mSize, backendType,
                           baseFlags, layersBackend);

  RefPtr<BufferTextureClient> texClient;

  {
    gl::ScopedReadbackFB autoReadback(src);

    
    GLenum destFormat = LOCAL_GL_BGRA;
    GLenum destType = LOCAL_GL_UNSIGNED_BYTE;
    GLenum readFormat;
    GLenum readType;

    
    
    auto gl = src->mGL;
    GetActualReadFormats(gl, destFormat, destType, &readFormat, &readType);

    MOZ_ASSERT(readFormat == LOCAL_GL_RGBA ||
               readFormat == LOCAL_GL_BGRA);
    MOZ_ASSERT(readType == LOCAL_GL_UNSIGNED_BYTE);

    
    if (readFormat == LOCAL_GL_BGRA &&
        readType == LOCAL_GL_UNSIGNED_BYTE)
    {
      
      
      texClient = factory.CreateB8G8R8AX8();

    } else if (readFormat == LOCAL_GL_RGBA &&
               readType == LOCAL_GL_UNSIGNED_BYTE)
    {
      
      texClient = factory.CreateR8G8B8AX8();
    } else {
      MOZ_CRASH("Bad `read{Format,Type}`.");
    }

    MOZ_ASSERT(texClient);
    if (!texClient)
        return nullptr;

    
    MOZ_ALWAYS_TRUE( texClient->Lock(OpenMode::OPEN_WRITE) );

    uint8_t* lockedBytes = texClient->GetLockedData();

    
    auto width = src->mSize.width;
    auto height = src->mSize.height;

    {
      ScopedPackAlignment autoAlign(gl, 4);

      gl->raw_fReadPixels(0, 0, width, height, readFormat, readType, lockedBytes);
    }

    
    
    
    bool layersNeedsManualSwap = layersBackend == LayersBackend::LAYERS_BASIC ||
                                 layersBackend == LayersBackend::LAYERS_D3D9 ||
                                 layersBackend == LayersBackend::LAYERS_D3D11;
    if (texClient->HasFlags(TextureFlags::RB_SWAPPED) &&
        layersNeedsManualSwap)
    {
      size_t pixels = width * height;
      uint8_t* itr = lockedBytes;
      for (size_t i = 0; i < pixels; i++) {
        SwapRB_R8G8B8A8(itr);
        itr += 4;
      }

      texClient->RemoveFlags(TextureFlags::RB_SWAPPED);
    }

    texClient->Unlock();
  }

  return texClient.forget();
}



void
CanvasClientSharedSurface::Update(gfx::IntSize aSize, ClientCanvasLayer* aLayer)
{
  aLayer->mGLContext->MakeCurrent();
  GLScreenBuffer* screen = aLayer->mGLContext->Screen();

  if (mFront) {
    mPrevFront = mFront;
    mFront = nullptr;
  }

  mFront = screen->Front();
  if (!mFront)
    return;

  
  SharedSurface* surf = mFront->Surf();
  auto forwarder = GetForwarder();
  auto flags = GetTextureFlags() | TextureFlags::IMMUTABLE;

  
  RefPtr<TextureClient> newTex = TexClientFromShSurf(surf, flags);
  if (!newTex) {
    auto manager = aLayer->ClientManager();
    auto shadowForwarder = manager->AsShadowForwarder();
    auto layersBackend = shadowForwarder->GetCompositorBackendType();

    newTex = TexClientFromReadback(surf, forwarder, flags, layersBackend);
  }
  MOZ_ASSERT(newTex);

  
  MOZ_ALWAYS_TRUE( newTex->InitIPDLActor(forwarder) );
  MOZ_ASSERT(newTex->GetIPDLActor());

  
  if (mFrontTex) {
    
    RefPtr<AsyncTransactionTracker> tracker = new RemoveTextureFromCompositableTracker();
    
    tracker->SetTextureClient(mFrontTex);
    mFrontTex->SetRemoveFromCompositableTracker(tracker);
    
    GetForwarder()->RemoveTextureFromCompositableAsync(tracker, this, mFrontTex);

    mFrontTex = nullptr;
  }

  
  mFrontTex = newTex;

  forwarder->UpdatedTexture(this, mFrontTex, nullptr);
  forwarder->UseTexture(this, mFrontTex);

  aLayer->Painted();
}

}
}
