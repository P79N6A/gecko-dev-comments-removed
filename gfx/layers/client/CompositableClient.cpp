




#include "mozilla/layers/CompositableClient.h"
#include <stdint.h>                     
#include "gfxPlatform.h"                
#include "mozilla/layers/CompositableForwarder.h"
#include "mozilla/layers/TextureClient.h"  
#include "mozilla/layers/TextureClientOGL.h"
#include "mozilla/mozalloc.h"           
#include "gfxASurface.h"                
#ifdef XP_WIN
#include "mozilla/layers/TextureD3D9.h"
#include "mozilla/layers/TextureD3D11.h"
#include "gfxWindowsPlatform.h"
#include "gfx2DGlue.h"
#endif
#ifdef MOZ_X11
#include "mozilla/layers/TextureClientX11.h"
#endif

using namespace mozilla::gfx;

namespace mozilla {
namespace layers {

CompositableClient::CompositableClient(CompositableForwarder* aForwarder)
: mCompositableChild(nullptr)
, mForwarder(aForwarder)
{
  MOZ_COUNT_CTOR(CompositableClient);
}


CompositableClient::~CompositableClient()
{
  MOZ_COUNT_DTOR(CompositableClient);
  Destroy();
}

LayersBackend
CompositableClient::GetCompositorBackendType() const
{
  return mForwarder->GetCompositorBackendType();
}

void
CompositableClient::SetIPDLActor(CompositableChild* aChild)
{
  mCompositableChild = aChild;
}

CompositableChild*
CompositableClient::GetIPDLActor() const
{
  return mCompositableChild;
}

bool
CompositableClient::Connect()
{
  if (!GetForwarder() || GetIPDLActor()) {
    return false;
  }
  GetForwarder()->Connect(this);
  return true;
}

void
CompositableClient::Destroy()
{
  if (!mCompositableChild) {
    return;
  }
  mCompositableChild->SetClient(nullptr);
  mCompositableChild->Destroy();
  mCompositableChild = nullptr;
}

uint64_t
CompositableClient::GetAsyncID() const
{
  if (mCompositableChild) {
    return mCompositableChild->GetAsyncID();
  }
  return 0; 
}


void
CompositableChild::Destroy()
{
  Send__delete__(this);
}

TemporaryRef<DeprecatedTextureClient>
CompositableClient::CreateDeprecatedTextureClient(DeprecatedTextureClientType aDeprecatedTextureClientType,
                                                  gfxContentType aContentType)
{
  MOZ_ASSERT(GetForwarder(), "Can't create a texture client if the compositable is not connected to the compositor.");
  LayersBackend parentBackend = GetForwarder()->GetCompositorBackendType();
  RefPtr<DeprecatedTextureClient> result;

  switch (aDeprecatedTextureClientType) {
  case TEXTURE_SHARED_GL:
  case TEXTURE_SHARED_GL_EXTERNAL:
  case TEXTURE_STREAM_GL:
    MOZ_CRASH("Unsupported. this should not be reached");
  case TEXTURE_YCBCR:
    if (parentBackend == LayersBackend::LAYERS_D3D9 ||
        parentBackend == LayersBackend::LAYERS_D3D11 ||
        parentBackend == LayersBackend::LAYERS_BASIC) {
      result = new DeprecatedTextureClientShmemYCbCr(GetForwarder(), GetTextureInfo());
    } else {
      MOZ_CRASH("Unsupported. this should not be reached");
    }
    break;
  case TEXTURE_CONTENT:
#ifdef XP_WIN
    if (parentBackend == LayersBackend::LAYERS_D3D11 && gfxWindowsPlatform::GetPlatform()->GetD2DDevice()) {
      result = new DeprecatedTextureClientD3D11(GetForwarder(), GetTextureInfo());
      break;
    }
    if (parentBackend == LayersBackend::LAYERS_D3D9 &&
        !GetForwarder()->ForwardsToDifferentProcess()) {
      
      
      
      
      
      
      if (aContentType == gfxContentType::COLOR_ALPHA ||
          !gfxWindowsPlatform::GetPlatform()->GetD3D9Device()) {
        result = new DeprecatedTextureClientDIB(GetForwarder(), GetTextureInfo());
      } else {
        result = new DeprecatedTextureClientD3D9(GetForwarder(), GetTextureInfo());
      }
      break;
    }
#endif
     
  case TEXTURE_SHMEM:
    result = new DeprecatedTextureClientShmem(GetForwarder(), GetTextureInfo());
    break;
  case TEXTURE_FALLBACK:
#ifdef XP_WIN
    if (parentBackend == LayersBackend::LAYERS_D3D11 ||
        parentBackend == LayersBackend::LAYERS_D3D9) {
      result = new DeprecatedTextureClientShmem(GetForwarder(), GetTextureInfo());
    }
#endif
    break;
  default:
    MOZ_ASSERT(false, "Unhandled texture client type");
  }

  
  
  
  if (!result) {
    return nullptr;
  }

  MOZ_ASSERT(result->SupportsType(aDeprecatedTextureClientType),
             "Created the wrong texture client?");
  result->SetFlags(GetTextureInfo().mTextureFlags);

  return result.forget();
}

TemporaryRef<BufferTextureClient>
CompositableClient::CreateBufferTextureClient(SurfaceFormat aFormat,
                                              TextureFlags aTextureFlags)
{










  if (gfxPlatform::GetPlatform()->PreferMemoryOverShmem()) {
    RefPtr<BufferTextureClient> result = new MemoryTextureClient(this, aFormat, aTextureFlags);
    return result.forget();
  }
  RefPtr<BufferTextureClient> result = new ShmemTextureClient(this, aFormat, aTextureFlags);
  return result.forget();
}

TemporaryRef<TextureClient>
CompositableClient::CreateTextureClientForDrawing(SurfaceFormat aFormat,
                                                  TextureFlags aTextureFlags)
{
  RefPtr<TextureClient> result;

#ifdef XP_WIN
  LayersBackend parentBackend = GetForwarder()->GetCompositorBackendType();
  if (parentBackend == LayersBackend::LAYERS_D3D11 && gfxWindowsPlatform::GetPlatform()->GetD2DDevice() &&
      !(aTextureFlags & TEXTURE_ALLOC_FALLBACK)) {
    result = new TextureClientD3D11(aFormat, aTextureFlags);
  }
  if (parentBackend == LayersBackend::LAYERS_D3D9 &&
      !GetForwarder()->ForwardsToDifferentProcess() &&
      !(aTextureFlags & TEXTURE_ALLOC_FALLBACK)) {
    
    if (ContentForFormat(aFormat) != gfxContentType::COLOR) {
      result = new DIBTextureClientD3D9(aFormat, aTextureFlags);
    } else {
      result = new CairoTextureClientD3D9(aFormat, aTextureFlags);
    }
  }
#endif

#ifdef MOZ_X11
  LayersBackend parentBackend = GetForwarder()->GetCompositorBackendType();
  if (parentBackend == LayersBackend::LAYERS_BASIC &&
      gfxPlatform::GetPlatform()->ScreenReferenceSurface()->GetType() == gfxSurfaceType::Xlib &&
      !(aTextureFlags & TEXTURE_ALLOC_FALLBACK))
  {
    result = new TextureClientX11(aFormat, aTextureFlags);
  }
#endif

  
  if (!result) {
    result = CreateBufferTextureClient(aFormat, aTextureFlags);
  }

  MOZ_ASSERT(!result || result->AsTextureClientDrawTarget(),
             "Not a TextureClientDrawTarget?");
  return result;
}

bool
CompositableClient::AddTextureClient(TextureClient* aClient)
{
  return aClient->InitIPDLActor(mForwarder);
}

void
CompositableClient::OnTransaction()
{
}

} 
} 
