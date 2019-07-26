




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

using namespace mozilla::gl;

namespace mozilla {
namespace layers {

void
ClientCanvasLayer::Initialize(const Data& aData)
{
  CopyableCanvasLayer::Initialize(aData);
 
  mCanvasClient = nullptr;

  if (mGLContext) {
    GLScreenBuffer* screen = mGLContext->Screen();
    SurfaceStreamType streamType =
        SurfaceStream::ChooseGLStreamType(SurfaceStream::OffMainThread,
                                          screen->PreserveBuffer());
    SurfaceFactory_GL* factory = nullptr;
    if (!mForceReadback) {
      if (ClientManager()->GetCompositorBackendType() == mozilla::layers::LAYERS_OPENGL) {
        if (mGLContext->GetEGLContext()) {
          bool isCrossProcess = !(XRE_GetProcessType() == GeckoProcessType_Default);

          if (!isCrossProcess) {
            
            factory = SurfaceFactory_EGLImage::Create(mGLContext, screen->Caps());
          } else {
            
#ifdef MOZ_WIDGET_GONK
            factory = new SurfaceFactory_Gralloc(mGLContext, screen->Caps(), ClientManager());
#else
            
            NS_NOTREACHED("isCrossProcess but not on native B2G!");
#endif
          }
        } else {
          
          
#ifdef XP_MACOSX
          factory = new SurfaceFactory_IOSurface(mGLContext, screen->Caps());
#else
          factory = new SurfaceFactory_GLTexture(mGLContext, nullptr, screen->Caps());
#endif
        }
      }
    }

    if (factory) {
      screen->Morph(factory, streamType);
    }
  }
}

void
ClientCanvasLayer::RenderLayer()
{
  PROFILER_LABEL("ClientCanvasLayer", "Paint");
  if (!IsDirty()) {
    return;
  }

  if (GetMaskLayer()) {
    ToClientLayer(GetMaskLayer())->RenderLayer();
  }
  
  if (!mCanvasClient) {
    TextureFlags flags = TEXTURE_IMMEDIATE_UPLOAD;
    if (mNeedsYFlip) {
      flags |= TEXTURE_NEEDS_Y_FLIP;
    }

    if (!mGLContext) {
      
      flags |= TEXTURE_IMMEDIATE_UPLOAD;
    } else {
      
      
      flags |= TEXTURE_DEALLOCATE_CLIENT;
    }
    mCanvasClient = CanvasClient::CreateCanvasClient(GetCanvasClientType(),
                                                     ClientManager(), flags);
    if (!mCanvasClient) {
      return;
    }
    if (HasShadow()) {
      mCanvasClient->Connect();
      ClientManager()->Attach(mCanvasClient, this);
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
