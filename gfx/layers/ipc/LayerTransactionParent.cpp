






#include "LayerTransactionParent.h"
#include <vector>                       
#include "CompositableHost.h"           
#include "ImageLayers.h"                
#include "Layers.h"                     
#include "ShadowLayerParent.h"          
#include "gfx3DMatrix.h"                
#include "gfxPoint3D.h"                 
#include "CompositableTransactionParent.h"  
#include "ShadowLayersManager.h"        
#include "mozilla/Assertions.h"         
#include "mozilla/gfx/BasePoint3D.h"    
#include "mozilla/layers/CanvasLayerComposite.h"
#include "mozilla/layers/ColorLayerComposite.h"
#include "mozilla/layers/Compositor.h"  
#include "mozilla/layers/ContainerLayerComposite.h"
#include "mozilla/layers/ImageLayerComposite.h"
#include "mozilla/layers/LayerManagerComposite.h"
#include "mozilla/layers/LayersMessages.h"  
#include "mozilla/layers/LayersSurfaces.h"  
#include "mozilla/layers/LayersTypes.h"  
#include "mozilla/layers/PCompositableParent.h"
#include "mozilla/layers/PLayerParent.h"  
#include "mozilla/layers/ThebesLayerComposite.h"
#include "mozilla/mozalloc.h"           
#include "nsCoord.h"                    
#include "nsDebug.h"                    
#include "nsISupportsImpl.h"            
#include "nsLayoutUtils.h"              
#include "nsMathUtils.h"                
#include "nsPoint.h"                    
#include "nsTArray.h"                   
#include "nsTraceRefcnt.h"              
#include "GeckoProfiler.h"

typedef std::vector<mozilla::layers::EditReply> EditReplyVector;

using mozilla::layout::RenderFrameParent;

namespace mozilla {
namespace layers {

class PGrallocBufferParent;



static ShadowLayerParent*
cast(const PLayerParent* in)
{
  return const_cast<ShadowLayerParent*>(
    static_cast<const ShadowLayerParent*>(in));
}

static CompositableParent*
cast(const PCompositableParent* in)
{
  return const_cast<CompositableParent*>(
    static_cast<const CompositableParent*>(in));
}

template<class OpCreateT>
static ShadowLayerParent*
AsLayerComposite(const OpCreateT& op)
{
  return cast(op.layerParent());
}

static ShadowLayerParent*
AsLayerComposite(const OpSetRoot& op)
{
  return cast(op.rootParent());
}

static ShadowLayerParent*
ShadowContainer(const OpInsertAfter& op)
{
  return cast(op.containerParent());
}
static ShadowLayerParent*
ShadowChild(const OpInsertAfter& op)
{
  return cast(op.childLayerParent());
}
static ShadowLayerParent*
ShadowAfter(const OpInsertAfter& op)
{
  return cast(op.afterParent());
}

static ShadowLayerParent*
ShadowContainer(const OpAppendChild& op)
{
  return cast(op.containerParent());
}
static ShadowLayerParent*
ShadowChild(const OpAppendChild& op)
{
  return cast(op.childLayerParent());
}

static ShadowLayerParent*
ShadowContainer(const OpRemoveChild& op)
{
  return cast(op.containerParent());
}
static ShadowLayerParent*
ShadowChild(const OpRemoveChild& op)
{
  return cast(op.childLayerParent());
}

static ShadowLayerParent*
ShadowContainer(const OpRepositionChild& op)
{
  return cast(op.containerParent());
}
static ShadowLayerParent*
ShadowChild(const OpRepositionChild& op)
{
  return cast(op.childLayerParent());
}
static ShadowLayerParent*
ShadowAfter(const OpRepositionChild& op)
{
  return cast(op.afterParent());
}

static ShadowLayerParent*
ShadowContainer(const OpRaiseToTopChild& op)
{
  return cast(op.containerParent());
}
static ShadowLayerParent*
ShadowChild(const OpRaiseToTopChild& op)
{
  return cast(op.childLayerParent());
}



LayerTransactionParent::LayerTransactionParent(LayerManagerComposite* aManager,
                                               ShadowLayersManager* aLayersManager,
                                               uint64_t aId)
  : mLayerManager(aManager)
  , mShadowLayersManager(aLayersManager)
  , mId(aId)
  , mDestroyed(false)
{
  MOZ_COUNT_CTOR(LayerTransactionParent);
}

LayerTransactionParent::~LayerTransactionParent()
{
  MOZ_COUNT_DTOR(LayerTransactionParent);
}

void
LayerTransactionParent::Destroy()
{
  mDestroyed = true;
  for (size_t i = 0; i < ManagedPLayerParent().Length(); ++i) {
    ShadowLayerParent* slp =
      static_cast<ShadowLayerParent*>(ManagedPLayerParent()[i]);
    slp->Destroy();
  }
}


bool
LayerTransactionParent::RecvUpdateNoSwap(const InfallibleTArray<Edit>& cset,
                                         const TargetConfig& targetConfig,
                                         const bool& isFirstPaint)
{
  return RecvUpdate(cset, targetConfig, isFirstPaint, nullptr);
}

bool
LayerTransactionParent::RecvUpdate(const InfallibleTArray<Edit>& cset,
                                   const TargetConfig& targetConfig,
                                   const bool& isFirstPaint,
                                   InfallibleTArray<EditReply>* reply)
{
  profiler_tracing("Paint", "Composite", TRACING_INTERVAL_START);
  PROFILER_LABEL("LayerTransactionParent", "RecvUpdate");
#ifdef COMPOSITOR_PERFORMANCE_WARNING
  TimeStamp updateStart = TimeStamp::Now();
#endif

  MOZ_LAYERS_LOG(("[ParentSide] received txn with %d edits", cset.Length()));

  if (mDestroyed || !layer_manager() || layer_manager()->IsDestroyed()) {
    return true;
  }

  EditReplyVector replyv;

  layer_manager()->BeginTransactionWithDrawTarget(nullptr);

  for (EditArray::index_type i = 0; i < cset.Length(); ++i) {
    const Edit& edit = cset[i];

    switch (edit.type()) {
    
    case Edit::TOpCreateThebesLayer: {
      MOZ_LAYERS_LOG(("[ParentSide] CreateThebesLayer"));

      nsRefPtr<ThebesLayerComposite> layer =
        layer_manager()->CreateThebesLayerComposite();
      AsLayerComposite(edit.get_OpCreateThebesLayer())->Bind(layer);
      break;
    }
    case Edit::TOpCreateContainerLayer: {
      MOZ_LAYERS_LOG(("[ParentSide] CreateContainerLayer"));

      nsRefPtr<ContainerLayer> layer = layer_manager()->CreateContainerLayerComposite();
      AsLayerComposite(edit.get_OpCreateContainerLayer())->Bind(layer);
      break;
    }
    case Edit::TOpCreateImageLayer: {
      MOZ_LAYERS_LOG(("[ParentSide] CreateImageLayer"));

      nsRefPtr<ImageLayerComposite> layer =
        layer_manager()->CreateImageLayerComposite();
      AsLayerComposite(edit.get_OpCreateImageLayer())->Bind(layer);
      break;
    }
    case Edit::TOpCreateColorLayer: {
      MOZ_LAYERS_LOG(("[ParentSide] CreateColorLayer"));

      nsRefPtr<ColorLayerComposite> layer = layer_manager()->CreateColorLayerComposite();
      AsLayerComposite(edit.get_OpCreateColorLayer())->Bind(layer);
      break;
    }
    case Edit::TOpCreateCanvasLayer: {
      MOZ_LAYERS_LOG(("[ParentSide] CreateCanvasLayer"));

      nsRefPtr<CanvasLayerComposite> layer =
        layer_manager()->CreateCanvasLayerComposite();
      AsLayerComposite(edit.get_OpCreateCanvasLayer())->Bind(layer);
      break;
    }
    case Edit::TOpCreateRefLayer: {
      MOZ_LAYERS_LOG(("[ParentSide] CreateRefLayer"));

      nsRefPtr<RefLayerComposite> layer =
        layer_manager()->CreateRefLayerComposite();
      AsLayerComposite(edit.get_OpCreateRefLayer())->Bind(layer);
      break;
    }

    
    case Edit::TOpSetLayerAttributes: {
      MOZ_LAYERS_LOG(("[ParentSide] SetLayerAttributes"));

      const OpSetLayerAttributes& osla = edit.get_OpSetLayerAttributes();
      Layer* layer = AsLayerComposite(osla)->AsLayer();
      const LayerAttributes& attrs = osla.attrs();

      const CommonLayerAttributes& common = attrs.common();
      layer->SetVisibleRegion(common.visibleRegion());
      layer->SetContentFlags(common.contentFlags());
      layer->SetOpacity(common.opacity());
      layer->SetClipRect(common.useClipRect() ? &common.clipRect() : nullptr);
      layer->SetBaseTransform(common.transform().value());
      layer->SetPostScale(common.postXScale(), common.postYScale());
      layer->SetIsFixedPosition(common.isFixedPosition());
      layer->SetFixedPositionAnchor(common.fixedPositionAnchor());
      layer->SetFixedPositionMargins(common.fixedPositionMargin());
      if (common.isStickyPosition()) {
        layer->SetStickyPositionData(common.stickyScrollContainerId(),
                                     common.stickyScrollRangeOuter(),
                                     common.stickyScrollRangeInner());
      }
      if (PLayerParent* maskLayer = common.maskLayerParent()) {
        layer->SetMaskLayer(cast(maskLayer)->AsLayer());
      } else {
        layer->SetMaskLayer(nullptr);
      }
      layer->SetAnimations(common.animations());

      typedef SpecificLayerAttributes Specific;
      const SpecificLayerAttributes& specific = attrs.specific();
      switch (specific.type()) {
      case Specific::Tnull_t:
        break;

      case Specific::TThebesLayerAttributes: {
        MOZ_LAYERS_LOG(("[ParentSide]   thebes layer"));

        ThebesLayerComposite* thebesLayer =
          static_cast<ThebesLayerComposite*>(layer);
        const ThebesLayerAttributes& attrs =
          specific.get_ThebesLayerAttributes();

        thebesLayer->SetValidRegion(attrs.validRegion());

        break;
      }
      case Specific::TContainerLayerAttributes: {
        MOZ_LAYERS_LOG(("[ParentSide]   container layer"));

        ContainerLayer* containerLayer =
          static_cast<ContainerLayer*>(layer);
        const ContainerLayerAttributes& attrs =
          specific.get_ContainerLayerAttributes();
        containerLayer->SetFrameMetrics(attrs.metrics());
        containerLayer->SetPreScale(attrs.preXScale(), attrs.preYScale());
        containerLayer->SetInheritedScale(attrs.inheritedXScale(), attrs.inheritedYScale());
        break;
      }
      case Specific::TColorLayerAttributes:
        MOZ_LAYERS_LOG(("[ParentSide]   color layer"));

        static_cast<ColorLayer*>(layer)->SetColor(
          specific.get_ColorLayerAttributes().color().value());
        static_cast<ColorLayer*>(layer)->SetBounds(
          specific.get_ColorLayerAttributes().bounds());
        break;

      case Specific::TCanvasLayerAttributes:
        MOZ_LAYERS_LOG(("[ParentSide]   canvas layer"));

        static_cast<CanvasLayer*>(layer)->SetFilter(
          specific.get_CanvasLayerAttributes().filter());
        static_cast<CanvasLayerComposite*>(layer)->SetBounds(
          specific.get_CanvasLayerAttributes().bounds());
        break;

      case Specific::TRefLayerAttributes:
        MOZ_LAYERS_LOG(("[ParentSide]   ref layer"));

        static_cast<RefLayer*>(layer)->SetReferentId(
          specific.get_RefLayerAttributes().id());
        break;

      case Specific::TImageLayerAttributes: {
        MOZ_LAYERS_LOG(("[ParentSide]   image layer"));

        ImageLayer* imageLayer = static_cast<ImageLayer*>(layer);
        const ImageLayerAttributes& attrs = specific.get_ImageLayerAttributes();
        imageLayer->SetFilter(attrs.filter());
        imageLayer->SetScaleToSize(attrs.scaleToSize(), attrs.scaleMode());
        break;
      }
      default:
        NS_RUNTIMEABORT("not reached");
      }
      break;
    }
    case Edit::TOpSetDiagnosticTypes: {
      mLayerManager->GetCompositor()->SetDiagnosticTypes(
        edit.get_OpSetDiagnosticTypes().diagnostics());
      break;
    }
    
    case Edit::TOpSetRoot: {
      MOZ_LAYERS_LOG(("[ParentSide] SetRoot"));

      mRoot = AsLayerComposite(edit.get_OpSetRoot())->AsLayer();
      break;
    }
    case Edit::TOpInsertAfter: {
      MOZ_LAYERS_LOG(("[ParentSide] InsertAfter"));

      const OpInsertAfter& oia = edit.get_OpInsertAfter();
      ShadowContainer(oia)->AsContainer()->InsertAfter(
        ShadowChild(oia)->AsLayer(), ShadowAfter(oia)->AsLayer());
      break;
    }
    case Edit::TOpAppendChild: {
      MOZ_LAYERS_LOG(("[ParentSide] AppendChild"));

      const OpAppendChild& oac = edit.get_OpAppendChild();
      ShadowContainer(oac)->AsContainer()->InsertAfter(
        ShadowChild(oac)->AsLayer(), nullptr);
      break;
    }
    case Edit::TOpRemoveChild: {
      MOZ_LAYERS_LOG(("[ParentSide] RemoveChild"));

      const OpRemoveChild& orc = edit.get_OpRemoveChild();
      Layer* childLayer = ShadowChild(orc)->AsLayer();
      ShadowContainer(orc)->AsContainer()->RemoveChild(childLayer);
      break;
    }
    case Edit::TOpRepositionChild: {
      MOZ_LAYERS_LOG(("[ParentSide] RepositionChild"));

      const OpRepositionChild& orc = edit.get_OpRepositionChild();
      ShadowContainer(orc)->AsContainer()->RepositionChild(
        ShadowChild(orc)->AsLayer(), ShadowAfter(orc)->AsLayer());
      break;
    }
    case Edit::TOpRaiseToTopChild: {
      MOZ_LAYERS_LOG(("[ParentSide] RaiseToTopChild"));

      const OpRaiseToTopChild& rtc = edit.get_OpRaiseToTopChild();
      ShadowContainer(rtc)->AsContainer()->RepositionChild(
        ShadowChild(rtc)->AsLayer(), nullptr);
      break;
    }
    case Edit::TCompositableOperation: {
      ReceiveCompositableUpdate(edit.get_CompositableOperation(),
                                replyv);
      break;
    }
    case Edit::TOpAttachCompositable: {
      const OpAttachCompositable& op = edit.get_OpAttachCompositable();
      Attach(cast(op.layerParent()), cast(op.compositableParent()), false);
      cast(op.compositableParent())->SetCompositorID(
        mLayerManager->GetCompositor()->GetCompositorID());
      break;
    }
    case Edit::TOpAttachAsyncCompositable: {
      const OpAttachAsyncCompositable& op = edit.get_OpAttachAsyncCompositable();
      CompositableParent* compositableParent = CompositableMap::Get(op.containerID());
      MOZ_ASSERT(compositableParent, "CompositableParent not found in the map");
      Attach(cast(op.layerParent()), compositableParent, true);
      compositableParent->SetCompositorID(mLayerManager->GetCompositor()->GetCompositorID());
      break;
    }
    default:
      NS_RUNTIMEABORT("not reached");
    }
  }

  layer_manager()->EndTransaction(nullptr, nullptr, LayerManager::END_NO_IMMEDIATE_REDRAW);

  if (reply) {
    reply->SetCapacity(replyv.size());
    if (replyv.size() > 0) {
      reply->AppendElements(&replyv.front(), replyv.size());
    }
  }

  
  
  
  LayerManagerComposite::PlatformSyncBeforeReplyUpdate();

  mShadowLayersManager->ShadowLayersUpdated(this, targetConfig, isFirstPaint);

#ifdef COMPOSITOR_PERFORMANCE_WARNING
  int compositeTime = (int)(mozilla::TimeStamp::Now() - updateStart).ToMilliseconds();
  if (compositeTime > 15) {
    printf_stderr("Compositor: Layers update took %i ms (blocking gecko).\n", compositeTime);
  }
#endif

  return true;
}

bool
LayerTransactionParent::RecvGetOpacity(PLayerParent* aParent,
                                       float* aOpacity)
{
  if (mDestroyed || !layer_manager() || layer_manager()->IsDestroyed()) {
    return false;
  }

  *aOpacity = cast(aParent)->AsLayer()->GetLocalOpacity();
  return true;
}

bool
LayerTransactionParent::RecvGetTransform(PLayerParent* aParent,
                                         gfx3DMatrix* aTransform)
{
  if (mDestroyed || !layer_manager() || layer_manager()->IsDestroyed()) {
    return false;
  }

  
  
  
  Layer* layer = cast(aParent)->AsLayer();
  *aTransform = layer->AsLayerComposite()->GetShadowTransform();
  if (ContainerLayer* c = layer->AsContainerLayer()) {
    aTransform->ScalePost(1.0f/c->GetInheritedXScale(),
                          1.0f/c->GetInheritedYScale(),
                          1.0f);
  }
  float scale = 1;
  gfxPoint3D scaledOrigin;
  gfxPoint3D mozOrigin;
  for (uint32_t i=0; i < layer->GetAnimations().Length(); i++) {
    if (layer->GetAnimations()[i].data().type() == AnimationData::TTransformData) {
      const TransformData& data = layer->GetAnimations()[i].data().get_TransformData();
      scale = data.appUnitsPerDevPixel();
      scaledOrigin =
        gfxPoint3D(NS_round(NSAppUnitsToFloatPixels(data.origin().x, scale)),
                   NS_round(NSAppUnitsToFloatPixels(data.origin().y, scale)),
                   0.0f);
      mozOrigin = data.mozOrigin();
      break;
    }
  }

  aTransform->Translate(-scaledOrigin);
  *aTransform = nsLayoutUtils::ChangeMatrixBasis(-scaledOrigin - mozOrigin, *aTransform);
  return true;
}

void
LayerTransactionParent::Attach(ShadowLayerParent* aLayerParent,
                               CompositableParent* aCompositable,
                               bool aIsAsyncVideo)
{
  LayerComposite* layer = aLayerParent->AsLayer()->AsLayerComposite();
  MOZ_ASSERT(layer);

  Compositor* compositor
    = static_cast<LayerManagerComposite*>(aLayerParent->AsLayer()->Manager())->GetCompositor();

  CompositableHost* compositable = aCompositable->GetCompositableHost();
  MOZ_ASSERT(compositable);
  layer->SetCompositableHost(compositable);
  compositable->Attach(aLayerParent->AsLayer(),
                       compositor,
                       aIsAsyncVideo
                         ? CompositableHost::ALLOW_REATTACH
                           | CompositableHost::KEEP_ATTACHED
                         : CompositableHost::NO_FLAGS);
}

bool
LayerTransactionParent::RecvClearCachedResources()
{
  if (mRoot) {
    
    
    
    mLayerManager->ClearCachedResources(mRoot);
  }
  return true;
}

PGrallocBufferParent*
LayerTransactionParent::AllocPGrallocBufferParent(const gfxIntSize& aSize,
                                            const uint32_t& aFormat,
                                            const uint32_t& aUsage,
                                            MaybeMagicGrallocBufferHandle* aOutHandle)
{
#ifdef MOZ_HAVE_SURFACEDESCRIPTORGRALLOC
  return GrallocBufferActor::Create(aSize, aFormat, aUsage, aOutHandle);
#else
  NS_RUNTIMEABORT("No gralloc buffers for you");
  return nullptr;
#endif
}

bool
LayerTransactionParent::DeallocPGrallocBufferParent(PGrallocBufferParent* actor)
{
#ifdef MOZ_HAVE_SURFACEDESCRIPTORGRALLOC
  delete actor;
  return true;
#else
  NS_RUNTIMEABORT("Um, how did we get here?");
  return false;
#endif
}

PLayerParent*
LayerTransactionParent::AllocPLayerParent()
{
  return new ShadowLayerParent();
}

bool
LayerTransactionParent::DeallocPLayerParent(PLayerParent* actor)
{
  delete actor;
  return true;
}

PCompositableParent*
LayerTransactionParent::AllocPCompositableParent(const TextureInfo& aInfo)
{
  return new CompositableParent(this, aInfo);
}

bool
LayerTransactionParent::DeallocPCompositableParent(PCompositableParent* actor)
{
  delete actor;
  return true;
}

} 
} 
