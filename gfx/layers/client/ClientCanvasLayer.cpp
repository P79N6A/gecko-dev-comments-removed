




#include "ClientCanvasLayer.h"
#include "GLContext.h"                  
#include "GLScreenBuffer.h"             
#include "GeckoProfiler.h"              
#include "SharedSurfaceEGL.h"           
#include "SharedSurfaceGL.h"            
#include "ClientLayerManager.h"         
#include "mozilla/gfx/Point.h"          
#include "mozilla/layers/CompositorTypes.h"
#include "mozilla/layers/LayersTypes.h"
#include "nsCOMPtr.h"                   
#include "nsISupportsImpl.h"            
#include "nsRect.h"                     
#include "nsXULAppAPI.h"                
#include "gfxPrefs.h"                   

#ifdef XP_WIN
#include "SharedSurfaceANGLE.h"         
#include "gfxWindowsPlatform.h"
#endif

#ifdef MOZ_WIDGET_GONK
#include "SharedSurfaceGralloc.h"
#endif

#ifdef XP_MACOSX
#include "SharedSurfaceIO.h"
#endif

using namespace mozilla::gfx;
using namespace mozilla::gl;

namespace mozilla {
namespace layers {

ClientCanvasLayer::~ClientCanvasLayer()
{
  MOZ_COUNT_DTOR(ClientCanvasLayer);
  if (mCanvasClient) {
    mCanvasClient->OnDetach();
    mCanvasClient = nullptr;
  }
}

void
ClientCanvasLayer::Initialize(const Data& aData)
{
  CopyableCanvasLayer::Initialize(aData);

  mCanvasClient = nullptr;

  if (!mGLContext)
    return;

  GLScreenBuffer* screen = mGLContext->Screen();

  SurfaceCaps caps;
  if (mGLFrontbuffer) {
    
    caps = mGLFrontbuffer->mHasAlpha ? SurfaceCaps::ForRGBA()
                                     : SurfaceCaps::ForRGB();
  } else {
    MOZ_ASSERT(screen);
    caps = screen->mCaps;
  }
  MOZ_ASSERT(caps.alpha == aData.mHasAlpha);

  auto forwarder = ClientManager()->AsShadowForwarder();

  mFlags = TextureFlags::ORIGIN_BOTTOM_LEFT;
  if (!aData.mIsGLAlphaPremult) {
    mFlags |= TextureFlags::NON_PREMULTIPLIED;
  }

  UniquePtr<SurfaceFactory> factory;

  if (!gfxPrefs::WebGLForceLayersReadback()) {
    switch (forwarder->GetCompositorBackendType()) {
      case mozilla::layers::LayersBackend::LAYERS_OPENGL: {
#if defined(XP_MACOSX)
        factory = SurfaceFactory_IOSurface::Create(mGLContext, caps, forwarder, mFlags);
#elif defined(MOZ_WIDGET_GONK)
        factory = MakeUnique<SurfaceFactory_Gralloc>(mGLContext, caps, forwarder, mFlags);
#else
        if (mGLContext->GetContextType() == GLContextType::EGL) {
          bool isCrossProcess = (XRE_GetProcessType() != GeckoProcessType_Default);
          if (!isCrossProcess) {
            factory = SurfaceFactory_EGLImage::Create(mGLContext, caps, forwarder,
                                                      mFlags);
          }
        }
#endif
        break;
      }
      case mozilla::layers::LayersBackend::LAYERS_D3D11: {
#ifdef XP_WIN
        if (mGLContext->IsANGLE() &&
            gfxWindowsPlatform::GetPlatform()->DoesD3D11TextureSharingWork())
        {
          factory = SurfaceFactory_ANGLEShareHandle::Create(mGLContext, caps, forwarder,
                                                            mFlags);
        }
#endif
        break;
      }
      default:
        break;
    }
  }

  if (mGLFrontbuffer) {
    
    
    mFactory = Move(factory);
    if (!mFactory) {
      
      mFactory = MakeUnique<SurfaceFactory_Basic>(mGLContext, caps, mFlags);
    }
  } else {
    if (factory)
      screen->Morph(Move(factory));
  }
}

void
ClientCanvasLayer::RenderLayer()
{
  PROFILER_LABEL("ClientCanvasLayer", "RenderLayer",
    js::ProfileEntry::Category::GRAPHICS);

  if (GetMaskLayer()) {
    ToClientLayer(GetMaskLayer())->RenderLayer();
  }

  if (!IsDirty()) {
    return;
  }
  Painted();

  if (!mCanvasClient) {
    TextureFlags flags = TextureFlags::IMMEDIATE_UPLOAD;
    if (mOriginPos == gl::OriginPos::BottomLeft) {
      flags |= TextureFlags::ORIGIN_BOTTOM_LEFT;
    }

    if (!mGLContext) {
      
      flags |= TextureFlags::IMMEDIATE_UPLOAD;
    }

    if (!mIsAlphaPremultiplied) {
      flags |= TextureFlags::NON_PREMULTIPLIED;
    }

    mCanvasClient = CanvasClient::CreateCanvasClient(GetCanvasClientType(),
                                                     ClientManager()->AsShadowForwarder(),
                                                     flags);
    if (!mCanvasClient) {
      return;
    }
    if (HasShadow()) {
      mCanvasClient->Connect();
      ClientManager()->AsShadowForwarder()->Attach(mCanvasClient, this);
    }
  }

  FirePreTransactionCallback();
  mCanvasClient->Update(gfx::IntSize(mBounds.width, mBounds.height), this);

  FireDidTransactionCallback();

  ClientManager()->Hold(this);
  mCanvasClient->Updated();
  mCanvasClient->OnTransaction();
}

CanvasClient::CanvasClientType
ClientCanvasLayer::GetCanvasClientType()
{
  if (mGLContext) {
    return CanvasClient::CanvasClientTypeShSurf;
  }
  return CanvasClient::CanvasClientSurface;
}

already_AddRefed<CanvasLayer>
ClientLayerManager::CreateCanvasLayer()
{
  NS_ASSERTION(InConstruction(), "Only allowed in construction phase");
  nsRefPtr<ClientCanvasLayer> layer =
    new ClientCanvasLayer(this);
  CREATE_SHADOW(Canvas);
  return layer.forget();
}

}
}
