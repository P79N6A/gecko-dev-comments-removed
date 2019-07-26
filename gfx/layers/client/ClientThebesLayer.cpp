




#include "ClientThebesLayer.h"
#include "ClientTiledThebesLayer.h"     
#include <stdint.h>                     
#include "GeckoProfiler.h"              
#include "client/ClientLayerManager.h"  
#include "gfxASurface.h"                
#include "gfxContext.h"                 
#include "gfxRect.h"                    
#include "mozilla/Assertions.h"         
#include "mozilla/gfx/2D.h"             
#include "mozilla/gfx/Matrix.h"         
#include "mozilla/gfx/Rect.h"           
#include "mozilla/gfx/Types.h"          
#include "mozilla/layers/LayersTypes.h"
#include "mozilla/Preferences.h"
#include "nsAutoPtr.h"                  
#include "nsCOMPtr.h"                   
#include "nsISupportsImpl.h"            
#include "nsRect.h"                     

using namespace mozilla::gfx;

namespace mozilla {
namespace layers {

void
ClientThebesLayer::PaintThebes()
{
  PROFILER_LABEL("ClientThebesLayer", "PaintThebes");
  NS_ASSERTION(ClientManager()->InDrawing(),
               "Can only draw in drawing phase");
  
  
  
  
  mContentClient->SyncFrontBufferToBackBuffer();

  bool canUseOpaqueSurface = CanUseOpaqueSurface();
  ContentType contentType =
    canUseOpaqueSurface ? GFX_CONTENT_COLOR :
                          GFX_CONTENT_COLOR_ALPHA;

  {
    uint32_t flags = 0;
#ifndef MOZ_WIDGET_ANDROID
    if (ClientManager()->CompositorMightResample()) {
      flags |= RotatedContentBuffer::PAINT_WILL_RESAMPLE;
    }
    if (!(flags & RotatedContentBuffer::PAINT_WILL_RESAMPLE)) {
      if (MayResample()) {
        flags |= RotatedContentBuffer::PAINT_WILL_RESAMPLE;
      }
    }
#endif
    PaintState state =
      mContentClient->BeginPaintBuffer(this, contentType, flags);
    mValidRegion.Sub(mValidRegion, state.mRegionToInvalidate);

    if (state.mContext) {
      
      
      
      
      state.mRegionToInvalidate.And(state.mRegionToInvalidate,
                                    GetEffectiveVisibleRegion());
      nsIntRegion extendedDrawRegion = state.mRegionToDraw;
      SetAntialiasingFlags(this, state.mContext);

      PaintBuffer(state.mContext,
                  state.mRegionToDraw, extendedDrawRegion, state.mRegionToInvalidate,
                  state.mDidSelfCopy, state.mClip);
      MOZ_LAYERS_LOG_IF_SHADOWABLE(this, ("Layer::Mutated(%p) PaintThebes", this));
      Mutated();
    } else {
      
      
      
      NS_WARN_IF_FALSE(state.mRegionToDraw.IsEmpty(),
                       "No context when we have something to draw, resource exhaustion?");
    }
  }
}

void
ClientThebesLayer::RenderLayer()
{
  if (GetMaskLayer()) {
    ToClientLayer(GetMaskLayer())->RenderLayer();
  }
  
  if (!mContentClient) {
    mContentClient = ContentClient::CreateContentClient(ClientManager()->AsShadowForwarder());
    if (!mContentClient) {
      return;
    }
    mContentClient->Connect();
    ClientManager()->AsShadowForwarder()->Attach(mContentClient, this);
    MOZ_ASSERT(mContentClient->GetForwarder());
  }

  mContentClient->BeginPaint();
  PaintThebes();
  mContentClient->EndPaint();
  
  
  
  
  
  
  
  
  
  
  
  mContentClient->OnTransaction();
}

void
ClientThebesLayer::PaintBuffer(gfxContext* aContext,
                               const nsIntRegion& aRegionToDraw,
                               const nsIntRegion& aExtendedRegionToDraw,
                               const nsIntRegion& aRegionToInvalidate,
                               bool aDidSelfCopy, DrawRegionClip aClip)
{
  ContentClientRemote* contentClientRemote = static_cast<ContentClientRemote*>(mContentClient.get());
  MOZ_ASSERT(contentClientRemote->GetIPDLActor());

  
  
  mValidRegion.SimplifyInward(8);

  if (!ClientManager()->GetThebesLayerCallback()) {
    ClientManager()->SetTransactionIncomplete();
    return;
  }
  ClientManager()->GetThebesLayerCallback()(this, 
                                            aContext, 
                                            aExtendedRegionToDraw,
                                            aClip,
                                            aRegionToInvalidate,
                                            ClientManager()->GetThebesLayerCallbackData());

  
  
  
  
  nsIntRegion tmp;
  tmp.Or(mVisibleRegion, aExtendedRegionToDraw);
  mValidRegion.Or(mValidRegion, tmp);

  
  
  
  ClientManager()->Hold(this);
  contentClientRemote->Updated(aRegionToDraw,
                               mVisibleRegion,
                               aDidSelfCopy);
}

already_AddRefed<ThebesLayer>
ClientLayerManager::CreateThebesLayer()
{
  return CreateThebesLayerWithHint(NONE);
}

already_AddRefed<ThebesLayer>
ClientLayerManager::CreateThebesLayerWithHint(ThebesLayerCreationHint aHint)
{
  NS_ASSERTION(InConstruction(), "Only allowed in construction phase");
  if (
#ifdef MOZ_B2G
      aHint == SCROLLABLE &&
#endif
      gfxPlatform::GetPrefLayersEnableTiles() && AsShadowForwarder()->GetCompositorBackendType() == LAYERS_OPENGL) {
    nsRefPtr<ClientTiledThebesLayer> layer =
      new ClientTiledThebesLayer(this);
    CREATE_SHADOW(Thebes);
    return layer.forget();
  } else
  {
    nsRefPtr<ClientThebesLayer> layer =
      new ClientThebesLayer(this);
    CREATE_SHADOW(Thebes);
    return layer.forget();
  }
}


}
}
