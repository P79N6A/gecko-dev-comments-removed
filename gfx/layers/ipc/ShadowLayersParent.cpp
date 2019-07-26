






#include <vector>

#include "AutoOpenSurface.h"
#include "CompositorParent.h"
#include "gfxSharedImageSurface.h"
#include "ImageLayers.h"
#include "mozilla/layout/RenderFrameParent.h"
#include "mozilla/unused.h"
#include "RenderTrace.h"
#include "ShadowLayerParent.h"
#include "ShadowLayersParent.h"
#include "ShadowLayers.h"
#include "ShadowLayerUtils.h"
#include "TiledLayerBuffer.h"
#include "gfxPlatform.h"
#include "mozilla/layers/TextureParent.h"
#include "CompositableHost.h"

typedef std::vector<mozilla::layers::EditReply> EditReplyVector;

using mozilla::layout::RenderFrameParent;

namespace mozilla {
namespace layers {



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

template<class OpPaintT>
static TextureHost*
AsTextureHost(const OpPaintT& op)
{
  return static_cast<TextureParent*>(op.textureParent())->GetTextureHost();
}

template<class OpCreateT>
static ShadowLayerParent*
AsShadowLayer(const OpCreateT& op)
{
  return cast(op.layerParent());
}

static ShadowLayerParent*
AsShadowLayer(const OpSetRoot& op)
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



ShadowLayersParent::ShadowLayersParent(ShadowLayerManager* aManager,
                                       ShadowLayersManager* aLayersManager,
                                       uint64_t aId)
  : mLayerManager(aManager)
  , mShadowLayersManager(aLayersManager)
  , mId(aId)
  , mDestroyed(false)
{
  MOZ_COUNT_CTOR(ShadowLayersParent);
}

ShadowLayersParent::~ShadowLayersParent()
{
  MOZ_COUNT_DTOR(ShadowLayersParent);
}

void
ShadowLayersParent::Destroy()
{
  mDestroyed = true;
  for (size_t i = 0; i < ManagedPLayerParent().Length(); ++i) {
    ShadowLayerParent* slp =
      static_cast<ShadowLayerParent*>(ManagedPLayerParent()[i]);
    slp->Destroy();
  }
}


bool
ShadowLayersParent::RecvUpdateNoSwap(const InfallibleTArray<Edit>& cset,
                                     const TargetConfig& targetConfig,
                                     const bool& isFirstPaint)
{
  return RecvUpdate(cset, targetConfig, isFirstPaint, nullptr);
}

bool
ShadowLayersParent::RecvUpdate(const InfallibleTArray<Edit>& cset,
                               const TargetConfig& targetConfig,
                               const bool& isFirstPaint,
                               InfallibleTArray<EditReply>* reply)
{
#ifdef COMPOSITOR_PERFORMANCE_WARNING
  TimeStamp updateStart = TimeStamp::Now();
#endif

  MOZ_LAYERS_LOG(("[ParentSide] received txn with %d edits", cset.Length()));

  if (mDestroyed || !layer_manager() || layer_manager()->IsDestroyed()) {
    return true;
  }

  EditReplyVector replyv;

  layer_manager()->BeginTransactionWithTarget(NULL);

  for (EditArray::index_type i = 0; i < cset.Length(); ++i) {
    const Edit& edit = cset[i];

    switch (edit.type()) {
    
    case Edit::TOpCreateThebesLayer: {
      MOZ_LAYERS_LOG(("[ParentSide] CreateThebesLayer"));

      nsRefPtr<ShadowThebesLayer> layer =
        layer_manager()->CreateShadowThebesLayer();
      AsShadowLayer(edit.get_OpCreateThebesLayer())->Bind(layer);
      break;
    }
    case Edit::TOpCreateContainerLayer: {
      MOZ_LAYERS_LOG(("[ParentSide] CreateContainerLayer"));

      nsRefPtr<ContainerLayer> layer = layer_manager()->CreateShadowContainerLayer();
      AsShadowLayer(edit.get_OpCreateContainerLayer())->Bind(layer);
      break;
    }
    case Edit::TOpCreateImageLayer: {
      MOZ_LAYERS_LOG(("[ParentSide] CreateImageLayer"));

      nsRefPtr<ShadowImageLayer> layer =
        layer_manager()->CreateShadowImageLayer();
      AsShadowLayer(edit.get_OpCreateImageLayer())->Bind(layer);
      break;
    }
    case Edit::TOpCreateColorLayer: {
      MOZ_LAYERS_LOG(("[ParentSide] CreateColorLayer"));

      nsRefPtr<ShadowColorLayer> layer = layer_manager()->CreateShadowColorLayer();
      AsShadowLayer(edit.get_OpCreateColorLayer())->Bind(layer);
      break;
    }
    case Edit::TOpCreateCanvasLayer: {
      MOZ_LAYERS_LOG(("[ParentSide] CreateCanvasLayer"));

      nsRefPtr<ShadowCanvasLayer> layer =
        layer_manager()->CreateShadowCanvasLayer();
      AsShadowLayer(edit.get_OpCreateCanvasLayer())->Bind(layer);
      break;
    }
    case Edit::TOpCreateRefLayer: {
      MOZ_LAYERS_LOG(("[ParentSide] CreateRefLayer"));

      nsRefPtr<ShadowRefLayer> layer =
        layer_manager()->CreateShadowRefLayer();
      AsShadowLayer(edit.get_OpCreateRefLayer())->Bind(layer);
      break;
    }

      
    case Edit::TOpSetLayerAttributes: {
      MOZ_LAYERS_LOG(("[ParentSide] SetLayerAttributes"));

      const OpSetLayerAttributes& osla = edit.get_OpSetLayerAttributes();
      Layer* layer = AsShadowLayer(osla)->AsLayer();
      const LayerAttributes& attrs = osla.attrs();

      const CommonLayerAttributes& common = attrs.common();
      layer->SetVisibleRegion(common.visibleRegion());
      layer->SetContentFlags(common.contentFlags());
      layer->SetOpacity(common.opacity());
      layer->SetClipRect(common.useClipRect() ? &common.clipRect() : NULL);
      layer->SetBaseTransform(common.transform().value());
      layer->SetPostScale(common.postXScale(), common.postYScale());
      layer->SetIsFixedPosition(common.isFixedPosition());
      layer->SetFixedPositionAnchor(common.fixedPositionAnchor());
      layer->SetFixedPositionMargins(common.fixedPositionMargin());
      if (PLayerParent* maskLayer = common.maskLayerParent()) {
        layer->SetMaskLayer(cast(maskLayer)->AsLayer());
      } else {
        layer->SetMaskLayer(NULL);
      }
      layer->SetAnimations(common.animations());

      typedef SpecificLayerAttributes Specific;
      const SpecificLayerAttributes& specific = attrs.specific();
      switch (specific.type()) {
      case Specific::Tnull_t:
        break;

      case Specific::TThebesLayerAttributes: {
        MOZ_LAYERS_LOG(("[ParentSide]   thebes layer"));

        ShadowThebesLayer* thebesLayer =
          static_cast<ShadowThebesLayer*>(layer);
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
        break;

      case Specific::TCanvasLayerAttributes:
        MOZ_LAYERS_LOG(("[ParentSide]   canvas layer"));

        static_cast<CanvasLayer*>(layer)->SetFilter(
          specific.get_CanvasLayerAttributes().filter());
        static_cast<ShadowCanvasLayer*>(layer)->SetBounds(
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
        break;
      }
      default:
        NS_RUNTIMEABORT("not reached");
      }
      break;
    }
    
    case Edit::TOpSetRoot: {
      MOZ_LAYERS_LOG(("[ParentSide] SetRoot"));

      mRoot = AsShadowLayer(edit.get_OpSetRoot())->AsContainer();
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
        ShadowChild(oac)->AsLayer(), NULL);
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
        ShadowChild(rtc)->AsLayer(), NULL);
      break;
    }
    case Edit::TCompositableOperation: {
      ReceiveCompositableUpdate(edit.get_CompositableOperation(),
                                replyv);
      break;
    }
    case Edit::TOpAttachCompositable: {
      const OpAttachCompositable& op = edit.get_OpAttachCompositable();
      Attach(cast(op.layerParent()), cast(op.compositableParent()));
      break;
    }
    case Edit::TOpAttachAsyncCompositable: {
      const OpAttachAsyncCompositable& op = edit.get_OpAttachAsyncCompositable();
      CompositableParent* compositableParent = CompositableMap::Get(op.containerID());
      MOZ_ASSERT(compositableParent, "CompositableParent not found in the map");
      Attach(cast(op.layerParent()), compositableParent);
      compositableParent->SetCompositorID(mLayerManager->GetCompositor()->GetCompositorID());
      break;
    }
    case Edit::TOpPaintTiledLayerBuffer: {
      MOZ_LAYERS_LOG(("[ParentSide] Paint TiledLayerBuffer"));
      const OpPaintTiledLayerBuffer& op = edit.get_OpPaintTiledLayerBuffer();
      ShadowLayerParent* shadow = AsShadowLayer(op);

      LayerComposite* compositeLayer = shadow->AsLayer()->AsLayerComposite();
      compositeLayer->EnsureBuffer(BUFFER_TILED);
      TiledLayerComposer* tileComposer = compositeLayer->AsTiledLayerComposer();

      NS_ASSERTION(tileComposer, "compositeLayer is not a tile composer");

      BasicTiledLayerBuffer* p = reinterpret_cast<BasicTiledLayerBuffer*>(op.tiledLayerBuffer());
      tileComposer->PaintedTiledLayerBuffer(p);
      break;
    }
    default:
      NS_RUNTIMEABORT("not reached");
    }
  }

  layer_manager()->EndTransaction(NULL, NULL, LayerManager::END_NO_IMMEDIATE_REDRAW);

  if (reply) {
    reply->SetCapacity(replyv.size());
    if (replyv.size() > 0) {
      reply->AppendElements(&replyv.front(), replyv.size());
    }
  }

  
  
  
  ShadowLayerManager::PlatformSyncBeforeReplyUpdate();

  mShadowLayersManager->ShadowLayersUpdated(this, targetConfig, isFirstPaint);

#ifdef COMPOSITOR_PERFORMANCE_WARNING
  int compositeTime = (int)(mozilla::TimeStamp::Now() - updateStart).ToMilliseconds();
  if (compositeTime > 15) {
    printf_stderr("Compositor: Layers update took %i ms (blocking gecko).\n", compositeTime);
  }
#endif

  return true;
}

void
ShadowLayersParent::Attach(ShadowLayerParent* aLayerParent, CompositableParent* aCompositable)
{
  ShadowLayer* layer = aLayerParent->AsLayer()->AsShadowLayer();
  MOZ_ASSERT(layer);
  LayerComposite* layerComposite = aLayerParent->AsLayer()->AsLayerComposite();

  Compositor* compositor
    = static_cast<LayerManagerComposite*>(aLayerParent->AsLayer()->Manager())->GetCompositor();

  CompositableHost* compositable = aCompositable->GetCompositableHost();
  MOZ_ASSERT(compositable);
  layerComposite->SetCompositableHost(compositable);
  compositable->Attach(aLayerParent->AsLayer(), compositor);
}

bool
ShadowLayersParent::RecvClearCachedResources()
{
  if (mRoot) {
    
    
    
    mLayerManager->ClearCachedResources(mRoot);
  }
  return true;
}

PGrallocBufferParent*
ShadowLayersParent::AllocPGrallocBuffer(const gfxIntSize& aSize,
                                        const gfxContentType& aContent,
                                        MaybeMagicGrallocBufferHandle* aOutHandle)
{
#ifdef MOZ_HAVE_SURFACEDESCRIPTORGRALLOC
  return GrallocBufferActor::Create(aSize, aContent, aOutHandle);
#else
  NS_RUNTIMEABORT("No gralloc buffers for you");
  return nullptr;
#endif
}

bool
ShadowLayersParent::DeallocPGrallocBuffer(PGrallocBufferParent* actor)
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
ShadowLayersParent::AllocPLayer()
{
  return new ShadowLayerParent();
}

bool
ShadowLayersParent::DeallocPLayer(PLayerParent* actor)
{
  delete actor;
  return true;
}

PCompositableParent*
ShadowLayersParent::AllocPCompositable(const CompositableType& aType)
{
  return new CompositableParent(this, aType);
}

bool
ShadowLayersParent::DeallocPCompositable(PCompositableParent* actor)
{
  delete actor;
  return true;
}

} 
} 
