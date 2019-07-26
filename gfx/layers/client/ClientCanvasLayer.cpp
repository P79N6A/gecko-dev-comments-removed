




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
#ifdef MOZ_WIDGET_GONK
#include "SharedSurfaceGralloc.h"
#endif
#ifdef XP_MACOSX
#include "SharedSurfaceIO.h"
#endif
#include "gfxPrefs.h"                   

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
    delete mTextureSurface;
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
      caps = screen->Caps();
    }
    MOZ_ASSERT(caps.alpha == aData.mHasAlpha);

    SurfaceStreamType streamType =
        SurfaceStream::ChooseGLStreamType(SurfaceStream::OffMainThread,
                                          screen->PreserveBuffer());
    SurfaceFactory_GL* factory = nullptr;
    if (!gfxPrefs::WebGLForceLayersReadback()) {
      if (ClientManager()->AsShadowForwarder()->GetCompositorBackendType() == mozilla::layers::LayersBackend::LAYERS_OPENGL) {
        if (mGLContext->GetContextType() == GLContextType::EGL) {
          bool isCrossProcess = !(XRE_GetProcessType() == GeckoProcessType_Default);

          if (!isCrossProcess) {
            
            factory = SurfaceFactory_EGLImage::Create(mGLContext, caps);
          } else {
            
#ifdef MOZ_WIDGET_GONK
            factory = new SurfaceFactory_Gralloc(mGLContext, caps, ClientManager()->AsShadowForwarder());
#else
            
            NS_NOTREACHED("isCrossProcess but not on native B2G!");
#endif
          }
        } else {
          
          
#ifdef XP_MACOSX
          factory = new SurfaceFactory_IOSurface(mGLContext, caps);
#else
          factory = new SurfaceFactory_GLTexture(mGLContext, nullptr, caps);
#endif
        }
      }
    }

    if (mStream) {
      
      mFactory = factory;
      if (!mFactory) {
        
        mFactory = new SurfaceFactory_Basic(mGLContext, caps);
      }

      gfx::IntSize size = gfx::IntSize(aData.mSize.width, aData.mSize.height);
      mTextureSurface = SharedSurface_GLTexture::Create(mGLContext, mGLContext,
                                                        mGLContext->GetGLFormats(),
                                                        size, caps.alpha, aData.mTexID);
      SharedSurface* producer = mStream->SwapProducer(mFactory, size);
      if (!producer) {
        
        delete mFactory;
        mFactory = new SurfaceFactory_Basic(mGLContext, caps);
        producer = mStream->SwapProducer(mFactory, size);
        MOZ_ASSERT(producer, "Failed to create initial canvas surface with basic factory");
      }
    } else if (factory) {
      screen->Morph(factory, streamType);
    }
  }
}

void
ClientCanvasLayer::RenderLayer()
{
  PROFILER_LABEL("ClientCanvasLayer", "RenderLayer",
    js::ProfileEntry::Category::GRAPHICS);

  if (!IsDirty()) {
    return;
  }

  if (GetMaskLayer()) {
    ToClientLayer(GetMaskLayer())->RenderLayer();
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
    mCanvasClient = CanvasClient::CreateCanvasClient(GetCanvasClientType(),
                                                     ClientManager()->AsShadowForwarder(), flags);
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
