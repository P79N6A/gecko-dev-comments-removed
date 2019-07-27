




#include "ClientCanvasLayer.h"
#include "GLContext.h"                  
#include "GLScreenBuffer.h"             
#include "GeckoProfiler.h"              
#include "SharedSurfaceEGL.h"           
#include "SharedSurfaceGL.h"            
#include "SurfaceStream.h"              
#include "SurfaceTypes.h"               
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
  if (mTextureSurface) {
    mTextureSurface = nullptr;
  }
}

void
ClientCanvasLayer::Initialize(const Data& aData)
{
  CopyableCanvasLayer::Initialize(aData);

  mCanvasClient = nullptr;

  if (mGLContext) {
    GLScreenBuffer* screen = mGLContext->Screen();

    SurfaceCaps caps;
    if (mStream) {
      
      caps = aData.mHasAlpha ? SurfaceCaps::ForRGBA() : SurfaceCaps::ForRGB();
    } else {
      caps = screen->mCaps;
    }
    MOZ_ASSERT(caps.alpha == aData.mHasAlpha);

    SurfaceStreamType streamType =
        SurfaceStream::ChooseGLStreamType(SurfaceStream::OffMainThread,
                                          screen->PreserveBuffer());
    UniquePtr<SurfaceFactory> factory = nullptr;

    if (!gfxPrefs::WebGLForceLayersReadback()) {
      switch (ClientManager()->AsShadowForwarder()->GetCompositorBackendType()) {
        case mozilla::layers::LayersBackend::LAYERS_OPENGL: {
          if (mGLContext->GetContextType() == GLContextType::EGL) {
#ifdef MOZ_WIDGET_GONK
            factory = MakeUnique<SurfaceFactory_Gralloc>(mGLContext,
                                                         caps,
                                                         ClientManager()->AsShadowForwarder());
#else
            bool isCrossProcess = !(XRE_GetProcessType() == GeckoProcessType_Default);
            if (!isCrossProcess) {
              
              factory = SurfaceFactory_EGLImage::Create(mGLContext, caps);
            } else {
              
              NS_NOTREACHED("isCrossProcess but not on native B2G!");
            }
#endif
          } else {
            
            
#ifdef XP_MACOSX
            factory = SurfaceFactory_IOSurface::Create(mGLContext, caps);
#else
            GLContext* nullConsGL = nullptr; 
            factory = MakeUnique<SurfaceFactory_GLTexture>(mGLContext, nullConsGL, caps);
#endif
          }
          break;
        }
        case mozilla::layers::LayersBackend::LAYERS_D3D10:
        case mozilla::layers::LayersBackend::LAYERS_D3D11: {
#ifdef XP_WIN
          if (mGLContext->IsANGLE()) {
            factory = SurfaceFactory_ANGLEShareHandle::Create(mGLContext, caps);
          }
#endif
          break;
        }
        default:
          break;
      }
    }

    if (mStream) {
      
      mFactory = Move(factory);
      if (!mFactory) {
        
        mFactory = MakeUnique<SurfaceFactory_Basic>(mGLContext, caps);
      }

      gfx::IntSize size = gfx::IntSize(aData.mSize.width, aData.mSize.height);
      mTextureSurface = SharedSurface_GLTexture::Create(mGLContext, mGLContext,
                                                        mGLContext->GetGLFormats(),
                                                        size, caps.alpha, aData.mTexID);
      SharedSurface* producer = mStream->SwapProducer(mFactory.get(), size);
      if (!producer) {
        
        mFactory = MakeUnique<SurfaceFactory_Basic>(mGLContext, caps);
        producer = mStream->SwapProducer(mFactory.get(), size);
        MOZ_ASSERT(producer, "Failed to create initial canvas surface with basic factory");
      }
    } else if (factory) {
      screen->Morph(Move(factory), streamType);
    }
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

  if (!mCanvasClient) {
    TextureFlags flags = TextureFlags::IMMEDIATE_UPLOAD;
    if (mNeedsYFlip) {
      flags |= TextureFlags::NEEDS_Y_FLIP;
    }

    if (!mGLContext) {
      
      flags |= TextureFlags::IMMEDIATE_UPLOAD;
    } else {
      
      
      flags |= TextureFlags::DEALLOCATE_CLIENT;
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
