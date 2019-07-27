




#include "ClientThebesLayer.h"
#include "ClientTiledThebesLayer.h"     
#include "SimpleTiledContentClient.h"
#include <stdint.h>                     
#include "GeckoProfiler.h"              
#include "client/ClientLayerManager.h"  
#include "gfxContext.h"                 
#include "gfxRect.h"                    
#include "gfxPrefs.h"                   
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
#include "gfx2DGlue.h"
#include "ReadbackProcessor.h"

namespace mozilla {
namespace layers {

using namespace mozilla::gfx;

void
ClientThebesLayer::PaintThebes()
{
  PROFILER_LABEL("ClientThebesLayer", "PaintThebes",
    js::ProfileEntry::Category::GRAPHICS);

  NS_ASSERTION(ClientManager()->InDrawing(),
               "Can only draw in drawing phase");
  
  uint32_t flags = RotatedContentBuffer::PAINT_CAN_DRAW_ROTATED;
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
    mContentClient->BeginPaintBuffer(this, flags);
  mValidRegion.Sub(mValidRegion, state.mRegionToInvalidate);

  if (!state.mRegionToDraw.IsEmpty() && !ClientManager()->GetThebesLayerCallback()) {
    ClientManager()->SetTransactionIncomplete();
    return;
  }

  
  
  
  
  state.mRegionToInvalidate.And(state.mRegionToInvalidate,
                                GetEffectiveVisibleRegion());

  bool didUpdate = false;
  RotatedContentBuffer::DrawIterator iter;
  while (DrawTarget* target = mContentClient->BorrowDrawTargetForPainting(state, &iter)) {
    SetAntialiasingFlags(this, target);

    nsRefPtr<gfxContext> ctx = gfxContext::ContextForDrawTarget(target);

    ClientManager()->GetThebesLayerCallback()(this,
                                              ctx,
                                              iter.mDrawRegion,
                                              state.mClip,
                                              state.mRegionToInvalidate,
                                              ClientManager()->GetThebesLayerCallbackData());

    ctx = nullptr;
    mContentClient->ReturnDrawTargetToBuffer(target);
    didUpdate = true;
  }

  if (didUpdate) {
    Mutated();

    mValidRegion.Or(mValidRegion, state.mRegionToDraw);

    ContentClientRemote* contentClientRemote = static_cast<ContentClientRemote*>(mContentClient.get());
    MOZ_ASSERT(contentClientRemote->GetIPDLActor());

    
    
    
    ClientManager()->Hold(this);
    contentClientRemote->Updated(state.mRegionToDraw,
                                 mVisibleRegion,
                                 state.mDidSelfCopy);
  }
}

void
ClientThebesLayer::RenderLayerWithReadback(ReadbackProcessor *aReadback)
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

  nsTArray<ReadbackProcessor::Update> readbackUpdates;
  nsIntRegion readbackRegion;
  if (aReadback && UsedForReadback()) {
    aReadback->GetThebesLayerUpdates(this, &readbackUpdates);
  }

  IntPoint origin(mVisibleRegion.GetBounds().x, mVisibleRegion.GetBounds().y);
  mContentClient->BeginPaint();
  PaintThebes();
  mContentClient->EndPaint(&readbackUpdates);
}

bool
ClientLayerManager::IsOptimizedFor(ThebesLayer* aLayer, ThebesLayerCreationHint aHint)
{
#ifdef MOZ_B2G
  
  
  
  
  
  
  return aHint == aLayer->GetCreationHint();
#else
  return LayerManager::IsOptimizedFor(aLayer, aHint);
#endif
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
      gfxPlatform::GetPlatform()->UseTiling() &&
      (AsShadowForwarder()->GetCompositorBackendType() == LayersBackend::LAYERS_OPENGL ||
       AsShadowForwarder()->GetCompositorBackendType() == LayersBackend::LAYERS_D3D9 ||
       AsShadowForwarder()->GetCompositorBackendType() == LayersBackend::LAYERS_D3D11)) {
    if (gfxPrefs::LayersUseSimpleTiles()) {
      nsRefPtr<SimpleClientTiledThebesLayer> layer =
        new SimpleClientTiledThebesLayer(this, aHint);
      CREATE_SHADOW(Thebes);
      return layer.forget();
    } else {
      nsRefPtr<ClientTiledThebesLayer> layer =
        new ClientTiledThebesLayer(this, aHint);
      CREATE_SHADOW(Thebes);
      return layer.forget();
    }
  } else
  {
    nsRefPtr<ClientThebesLayer> layer =
      new ClientThebesLayer(this, aHint);
    CREATE_SHADOW(Thebes);
    return layer.forget();
  }
}


}
}
