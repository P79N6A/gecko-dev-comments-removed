




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
#ifdef GL_PROVIDER_GLX
#include "GLXLibrary.h"
#endif
#endif
#ifdef MOZ_WIDGET_GONK
#include <cutils/properties.h>
#include "mozilla/layers/GrallocTextureClient.h"
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
                                              TextureFlags aTextureFlags,
                                              gfx::BackendType aMoz2DBackend)
{










  if (gfxPlatform::GetPlatform()->PreferMemoryOverShmem()) {
    RefPtr<BufferTextureClient> result = new MemoryTextureClient(this, aFormat,
                                                                 aMoz2DBackend,
                                                                 aTextureFlags);
    return result.forget();
  }
  RefPtr<BufferTextureClient> result = new ShmemTextureClient(this, aFormat,
                                                              aMoz2DBackend,
                                                              aTextureFlags);
  return result.forget();
}

#ifdef MOZ_WIDGET_GONK
static bool
DisableGralloc(SurfaceFormat aFormat)
{
  if (aFormat == gfx::SurfaceFormat::A8) {
    return true;
  }
#if ANDROID_VERSION <= 15
  static bool checkedDevice = false;
  static bool disableGralloc = false;

  if (!checkedDevice) {
    char propValue[PROPERTY_VALUE_MAX];
    property_get("ro.product.device", propValue, "None");

    if (strcmp("crespo",propValue) == 0) {
      NS_WARNING("Nexus S has issues with gralloc, falling back to shmem");
      disableGralloc = true;
    }

    checkedDevice = true;
  }

  if (disableGralloc) {
    return true;
  }
  return false;
#else
  return false;
#endif
}
#endif

TemporaryRef<TextureClient>
CompositableClient::CreateTextureClientForDrawing(SurfaceFormat aFormat,
                                                  TextureFlags aTextureFlags,
                                                  gfx::BackendType aMoz2DBackend)
{
  if (aMoz2DBackend == gfx::BackendType::NONE) {
    aMoz2DBackend = gfxPlatform::GetPlatform()->GetContentBackend();
  }

  RefPtr<TextureClient> result;

#ifdef XP_WIN
  LayersBackend parentBackend = GetForwarder()->GetCompositorBackendType();
  if (parentBackend == LayersBackend::LAYERS_D3D11 &&
      (aMoz2DBackend == gfx::BackendType::DIRECT2D ||
        aMoz2DBackend == gfx::BackendType::DIRECT2D1_1) &&
      gfxWindowsPlatform::GetPlatform()->GetD2DDevice() &&
      !(aTextureFlags & TEXTURE_ALLOC_FALLBACK)) {
    result = new TextureClientD3D11(aFormat, aTextureFlags);
  }
  if (parentBackend == LayersBackend::LAYERS_D3D9 &&
      aMoz2DBackend == gfx::BackendType::CAIRO &&
      !GetForwarder()->ForwardsToDifferentProcess() &&
      !(aTextureFlags & TEXTURE_ALLOC_FALLBACK)) {
    if (!gfxWindowsPlatform::GetPlatform()->GetD3D9Device()) {
      result = new DIBTextureClientD3D9(aFormat, aTextureFlags);
    } else {
      result = new CairoTextureClientD3D9(aFormat, aTextureFlags);
    }
  }
#endif

#ifdef MOZ_X11
  LayersBackend parentBackend = GetForwarder()->GetCompositorBackendType();
  gfxSurfaceType type =
    gfxPlatform::GetPlatform()->ScreenReferenceSurface()->GetType();

  if (parentBackend == LayersBackend::LAYERS_BASIC &&
      aMoz2DBackend == gfx::BackendType::CAIRO &&
      type == gfxSurfaceType::Xlib &&
      !(aTextureFlags & TEXTURE_ALLOC_FALLBACK))
  {
    result = new TextureClientX11(aFormat, aTextureFlags);
  }
#ifdef GL_PROVIDER_GLX
  if (parentBackend == LayersBackend::LAYERS_OPENGL &&
      aMoz2DBackend == gfx::BackendType::CAIRO &&
      type == gfxSurfaceType::Xlib &&
      !(aTextureFlags & TEXTURE_ALLOC_FALLBACK) &&
      aFormat != SurfaceFormat::A8 &&
      gl::sGLXLibrary.UseTextureFromPixmap())
  {
    result = new TextureClientX11(aFormat, aTextureFlags);
  }
#endif
#endif

#ifdef MOZ_WIDGET_GONK
  if (!DisableGralloc(aFormat)) {
    result = new GrallocTextureClientOGL(this, aFormat, aTextureFlags);
  }
#endif

  
  if (!result) {
    result = CreateBufferTextureClient(aFormat, aTextureFlags, aMoz2DBackend);
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
