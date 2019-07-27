






#include "base/basictypes.h"

#include "BasicLayers.h"
#ifdef MOZ_ENABLE_D3D9_LAYER
# include "LayerManagerD3D9.h"
#endif 
#include "mozilla/BrowserElementParent.h"
#include "mozilla/dom/ContentChild.h"
#include "mozilla/dom/TabParent.h"
#include "mozilla/layers/APZCTreeManager.h"
#include "mozilla/layers/CompositorParent.h"
#include "mozilla/layers/LayerTransactionParent.h"
#include "nsContentUtils.h"
#include "nsFrameLoader.h"
#include "nsIObserver.h"
#include "nsSubDocumentFrame.h"
#include "nsView.h"
#include "nsViewportFrame.h"
#include "RenderFrameParent.h"
#include "mozilla/layers/LayerManagerComposite.h"
#include "mozilla/layers/CompositorChild.h"
#include "ClientLayerManager.h"
#include "FrameLayerBuilder.h"

using namespace mozilla::dom;
using namespace mozilla::gfx;
using namespace mozilla::layers;

namespace mozilla {
namespace layout {

typedef FrameMetrics::ViewID ViewID;






static nsIntPoint
GetContentRectLayerOffset(nsIFrame* aContainerFrame, nsDisplayListBuilder* aBuilder)
{
  nscoord auPerDevPixel = aContainerFrame->PresContext()->AppUnitsPerDevPixel();

  
  
  
  
  nsPoint frameOffset = aBuilder->ToReferenceFrame(aContainerFrame) +
    (aContainerFrame->GetContentRect().TopLeft() - aContainerFrame->GetPosition());

  return frameOffset.ToNearestPixels(auPerDevPixel);
}





inline static bool
IsTempLayerManager(LayerManager* aManager)
{
  return (mozilla::layers::LayersBackend::LAYERS_BASIC == aManager->GetBackendType() &&
          !static_cast<BasicLayerManager*>(aManager)->IsRetained());
}

already_AddRefed<LayerManager>
GetFrom(nsFrameLoader* aFrameLoader)
{
  nsIDocument* doc = aFrameLoader->GetOwnerDoc();
  return nsContentUtils::LayerManagerForDocument(doc);
}

class RemoteContentController : public GeckoContentController {
public:
  explicit RemoteContentController(RenderFrameParent* aRenderFrame)
    : mUILoop(MessageLoop::current())
    , mRenderFrame(aRenderFrame)
    , mHaveZoomConstraints(false)
  { }

  virtual void RequestContentRepaint(const FrameMetrics& aFrameMetrics) MOZ_OVERRIDE
  {
    
    
    mUILoop->PostTask(
      FROM_HERE,
      NewRunnableMethod(this, &RemoteContentController::DoRequestContentRepaint,
                        aFrameMetrics));
  }

  virtual void AcknowledgeScrollUpdate(const FrameMetrics::ViewID& aScrollId,
                                       const uint32_t& aScrollGeneration) MOZ_OVERRIDE
  {
    if (MessageLoop::current() != mUILoop) {
      
      
      mUILoop->PostTask(
        FROM_HERE,
        NewRunnableMethod(this, &RemoteContentController::AcknowledgeScrollUpdate,
                          aScrollId, aScrollGeneration));
      return;
    }
    if (mRenderFrame) {
      TabParent* browser = static_cast<TabParent*>(mRenderFrame->Manager());
      browser->AcknowledgeScrollUpdate(aScrollId, aScrollGeneration);
    }
  }

  virtual void HandleDoubleTap(const CSSPoint& aPoint,
                               int32_t aModifiers,
                               const ScrollableLayerGuid& aGuid) MOZ_OVERRIDE
  {
    if (MessageLoop::current() != mUILoop) {
      
      
      mUILoop->PostTask(
        FROM_HERE,
        NewRunnableMethod(this, &RemoteContentController::HandleDoubleTap,
                          aPoint, aModifiers, aGuid));
      return;
    }
    if (mRenderFrame) {
      TabParent* browser = static_cast<TabParent*>(mRenderFrame->Manager());
      browser->HandleDoubleTap(aPoint, aModifiers, aGuid);
    }
  }

  virtual void HandleSingleTap(const CSSPoint& aPoint,
                               int32_t aModifiers,
                               const ScrollableLayerGuid& aGuid) MOZ_OVERRIDE
  {
    if (MessageLoop::current() != mUILoop) {
      
      
      mUILoop->PostTask(
        FROM_HERE,
        NewRunnableMethod(this, &RemoteContentController::HandleSingleTap,
                          aPoint, aModifiers, aGuid));
      return;
    }
    if (mRenderFrame) {
      TabParent* browser = static_cast<TabParent*>(mRenderFrame->Manager());
      browser->HandleSingleTap(aPoint, aModifiers, aGuid);
    }
  }

  virtual void HandleLongTap(const CSSPoint& aPoint,
                             int32_t aModifiers,
                             const ScrollableLayerGuid& aGuid) MOZ_OVERRIDE
  {
    if (MessageLoop::current() != mUILoop) {
      
      
      mUILoop->PostTask(
        FROM_HERE,
        NewRunnableMethod(this, &RemoteContentController::HandleLongTap,
                          aPoint, aModifiers, aGuid));
      return;
    }
    if (mRenderFrame) {
      TabParent* browser = static_cast<TabParent*>(mRenderFrame->Manager());
      browser->HandleLongTap(aPoint, aModifiers, aGuid);
    }
  }

  virtual void HandleLongTapUp(const CSSPoint& aPoint,
                               int32_t aModifiers,
                               const ScrollableLayerGuid& aGuid) MOZ_OVERRIDE
  {
    if (MessageLoop::current() != mUILoop) {
      
      
      mUILoop->PostTask(
        FROM_HERE,
        NewRunnableMethod(this, &RemoteContentController::HandleLongTapUp,
                          aPoint, aModifiers, aGuid));
      return;
    }
    if (mRenderFrame) {
      TabParent* browser = static_cast<TabParent*>(mRenderFrame->Manager());
      browser->HandleLongTapUp(aPoint, aModifiers, aGuid);
    }
  }

  void ClearRenderFrame() { mRenderFrame = nullptr; }

  virtual void SendAsyncScrollDOMEvent(bool aIsRoot,
                                       const CSSRect& aContentRect,
                                       const CSSSize& aContentSize) MOZ_OVERRIDE
  {
    if (MessageLoop::current() != mUILoop) {
      mUILoop->PostTask(
        FROM_HERE,
        NewRunnableMethod(this,
                          &RemoteContentController::SendAsyncScrollDOMEvent,
                          aIsRoot, aContentRect, aContentSize));
      return;
    }
    if (mRenderFrame && aIsRoot) {
      TabParent* browser = static_cast<TabParent*>(mRenderFrame->Manager());
      BrowserElementParent::DispatchAsyncScrollEvent(browser, aContentRect,
                                                     aContentSize);
    }
  }

  virtual void PostDelayedTask(Task* aTask, int aDelayMs) MOZ_OVERRIDE
  {
    MessageLoop::current()->PostDelayedTask(FROM_HERE, aTask, aDelayMs);
  }

  virtual bool GetRootZoomConstraints(ZoomConstraints* aOutConstraints)
  {
    if (mHaveZoomConstraints && aOutConstraints) {
      *aOutConstraints = mZoomConstraints;
    }
    return mHaveZoomConstraints;
  }

  virtual bool GetTouchSensitiveRegion(CSSRect* aOutRegion)
  {
    if (mTouchSensitiveRegion.IsEmpty())
      return false;

    *aOutRegion = CSSRect::FromAppUnits(mTouchSensitiveRegion.GetBounds());
    return true;
  }

  virtual void NotifyAPZStateChange(const ScrollableLayerGuid& aGuid,
                                    APZStateChange aChange,
                                    int aArg)
  {
    if (MessageLoop::current() != mUILoop) {
      mUILoop->PostTask(
        FROM_HERE,
        NewRunnableMethod(this, &RemoteContentController::NotifyAPZStateChange,
                          aGuid, aChange, aArg));
      return;
    }
    if (mRenderFrame) {
      TabParent* browser = static_cast<TabParent*>(mRenderFrame->Manager());
      browser->NotifyAPZStateChange(aGuid.mScrollId, aChange, aArg);
    }
  }

  

  void SaveZoomConstraints(const ZoomConstraints& aConstraints)
  {
    mHaveZoomConstraints = true;
    mZoomConstraints = aConstraints;
  }

  void SetTouchSensitiveRegion(const nsRegion& aRegion)
  {
    mTouchSensitiveRegion = aRegion;
  }
private:
  void DoRequestContentRepaint(const FrameMetrics& aFrameMetrics)
  {
    if (mRenderFrame) {
      TabParent* browser = static_cast<TabParent*>(mRenderFrame->Manager());
      browser->UpdateFrame(aFrameMetrics);
    }
  }

  MessageLoop* mUILoop;
  RenderFrameParent* mRenderFrame;

  bool mHaveZoomConstraints;
  ZoomConstraints mZoomConstraints;
  nsRegion mTouchSensitiveRegion;
};

RenderFrameParent::RenderFrameParent(nsFrameLoader* aFrameLoader,
                                     ScrollingBehavior aScrollingBehavior,
                                     TextureFactoryIdentifier* aTextureFactoryIdentifier,
                                     uint64_t* aId,
                                     bool* aSuccess)
  : mLayersId(0)
  , mFrameLoader(aFrameLoader)
  , mFrameLoaderDestroyed(false)
  , mBackgroundColor(gfxRGBA(1, 1, 1))
{
  *aSuccess = false;
  if (!mFrameLoader) {
    return;
  }

  *aId = 0;

  nsRefPtr<LayerManager> lm = GetFrom(mFrameLoader);
  
  if (lm && lm->GetBackendType() == LayersBackend::LAYERS_CLIENT) {
    *aTextureFactoryIdentifier =
      static_cast<ClientLayerManager*>(lm.get())->GetTextureFactoryIdentifier();
  } else {
    *aTextureFactoryIdentifier = TextureFactoryIdentifier();
  }

  if (XRE_GetProcessType() == GeckoProcessType_Default) {
    
    
    *aId = mLayersId = CompositorParent::AllocateLayerTreeId();
    if (lm && lm->GetBackendType() == LayersBackend::LAYERS_CLIENT) {
      ClientLayerManager *clientManager =
        static_cast<ClientLayerManager*>(lm.get());
      clientManager->GetRemoteRenderer()->SendNotifyChildCreated(mLayersId);
    }
    if (aScrollingBehavior == ASYNC_PAN_ZOOM) {
      mContentController = new RemoteContentController(this);
      CompositorParent::SetControllerForLayerTree(mLayersId, mContentController);
    }
  } else if (XRE_GetProcessType() == GeckoProcessType_Content) {
    ContentChild::GetSingleton()->SendAllocateLayerTreeId(aId);
    mLayersId = *aId;
    CompositorChild::Get()->SendNotifyChildCreated(mLayersId);
  }
  
  mFrameLoader->SetCurrentRemoteFrame(this);
  *aSuccess = true;
}

APZCTreeManager*
RenderFrameParent::GetApzcTreeManager()
{
  
  
  
  
  if (!mApzcTreeManager) {
    mApzcTreeManager = CompositorParent::GetAPZCTreeManager(mLayersId);
  }
  return mApzcTreeManager.get();
}

RenderFrameParent::~RenderFrameParent()
{}

void
RenderFrameParent::Destroy()
{
  mFrameLoaderDestroyed = true;
}

already_AddRefed<Layer>
RenderFrameParent::BuildLayer(nsDisplayListBuilder* aBuilder,
                              nsIFrame* aFrame,
                              LayerManager* aManager,
                              const nsIntRect& aVisibleRect,
                              nsDisplayItem* aItem,
                              const ContainerLayerParameters& aContainerParameters)
{
  NS_ABORT_IF_FALSE(aFrame,
                    "makes no sense to have a shadow tree without a frame");
  NS_ABORT_IF_FALSE(!mContainer ||
                    IsTempLayerManager(aManager) ||
                    mContainer->Manager() == aManager,
                    "retaining manager changed out from under us ... HELP!");

  if (IsTempLayerManager(aManager) ||
      (mContainer && mContainer->Manager() != aManager)) {
    
    
    
    
    
    
    NS_WARNING("Remote iframe not rendered");
    return nullptr;
  }

  uint64_t id = GetLayerTreeId();
  if (!id) {
    return nullptr;
  }

  nsRefPtr<Layer> layer =
    (aManager->GetLayerBuilder()->GetLeafLayerFor(aBuilder, aItem));
  if (!layer) {
    layer = aManager->CreateRefLayer();
  }
  if (!layer) {
    
    
    return nullptr;
  }
  static_cast<RefLayer*>(layer.get())->SetReferentId(id);
  nsIntPoint offset = GetContentRectLayerOffset(aFrame, aBuilder);
  
  
  
  MOZ_ASSERT(aContainerParameters.mOffset == nsIntPoint());
  gfx::Matrix4x4 m;
  m.Translate(offset.x, offset.y, 0.0);
  
  
  m.Scale(aContainerParameters.mXScale, aContainerParameters.mYScale, 1.0);
  layer->SetBaseTransform(m);

  return layer.forget();
}

void
RenderFrameParent::OwnerContentChanged(nsIContent* aContent)
{
  NS_ABORT_IF_FALSE(mFrameLoader->GetOwnerContent() == aContent,
                    "Don't build new map if owner is same!");
}

nsEventStatus
RenderFrameParent::NotifyInputEvent(WidgetInputEvent& aEvent,
                                    ScrollableLayerGuid* aOutTargetGuid)
{
  if (GetApzcTreeManager()) {
    return GetApzcTreeManager()->ReceiveInputEvent(aEvent, aOutTargetGuid);
  }
  return nsEventStatus_eIgnore;
}

void
RenderFrameParent::ActorDestroy(ActorDestroyReason why)
{
  if (mLayersId != 0) {
    if (XRE_GetProcessType() == GeckoProcessType_Content) {
      ContentChild::GetSingleton()->SendDeallocateLayerTreeId(mLayersId);
    } else {
      CompositorParent::DeallocateLayerTreeId(mLayersId);
    }
    if (mContentController) {
      
      
      mContentController->ClearRenderFrame();
      
    }
  }

  if (mFrameLoader && mFrameLoader->GetCurrentRemoteFrame() == this) {
    
    
    
    
    
    
    mFrameLoader->SetCurrentRemoteFrame(nullptr);
  }
  mFrameLoader = nullptr;
}

bool
RenderFrameParent::RecvNotifyCompositorTransaction()
{
  TriggerRepaint();
  return true;
}

bool
RenderFrameParent::RecvUpdateHitRegion(const nsRegion& aRegion)
{
  mTouchRegion = aRegion;
  if (mContentController) {
    
    
    
    
    mContentController->SetTouchSensitiveRegion(aRegion);
  }
  return true;
}

void
RenderFrameParent::TriggerRepaint()
{
  mFrameLoader->SetCurrentRemoteFrame(this);

  nsIFrame* docFrame = mFrameLoader->GetPrimaryFrameOfOwningContent();
  if (!docFrame) {
    
    
    
    
    
    return;
  }

  docFrame->InvalidateLayer(nsDisplayItem::TYPE_REMOTE);
}

uint64_t
RenderFrameParent::GetLayerTreeId() const
{
  return mLayersId;
}

void
RenderFrameParent::BuildDisplayList(nsDisplayListBuilder* aBuilder,
                                    nsSubDocumentFrame* aFrame,
                                    const nsRect& aDirtyRect,
                                    const nsDisplayListSet& aLists)
{
  
  
  DisplayListClipState::AutoSaveRestore clipState(aBuilder);

  nsPoint offset = aBuilder->ToReferenceFrame(aFrame);
  nsRect bounds = aFrame->EnsureInnerView()->GetBounds() + offset;
  clipState.ClipContentDescendants(bounds);

  aLists.Content()->AppendToTop(
    new (aBuilder) nsDisplayRemote(aBuilder, aFrame, this));
}

void
RenderFrameParent::ZoomToRect(uint32_t aPresShellId, ViewID aViewId,
                              const CSSRect& aRect)
{
  if (GetApzcTreeManager()) {
    GetApzcTreeManager()->ZoomToRect(ScrollableLayerGuid(mLayersId, aPresShellId, aViewId),
                                     aRect);
  }
}

void
RenderFrameParent::ContentReceivedTouch(const ScrollableLayerGuid& aGuid,
                                        bool aPreventDefault)
{
  if (aGuid.mLayersId != mLayersId) {
    
    NS_ERROR("Unexpected layers id in ContentReceivedTouch; dropping message...");
    return;
  }
  if (GetApzcTreeManager()) {
    GetApzcTreeManager()->ContentReceivedTouch(aGuid, aPreventDefault);
  }
}

void
RenderFrameParent::UpdateZoomConstraints(uint32_t aPresShellId,
                                         ViewID aViewId,
                                         bool aIsRoot,
                                         const ZoomConstraints& aConstraints)
{
  if (mContentController && aIsRoot) {
    mContentController->SaveZoomConstraints(aConstraints);
  }
  if (GetApzcTreeManager()) {
    GetApzcTreeManager()->UpdateZoomConstraints(ScrollableLayerGuid(mLayersId, aPresShellId, aViewId),
                                                aConstraints);
  }
}

bool
RenderFrameParent::HitTest(const nsRect& aRect)
{
  return mTouchRegion.Contains(aRect);
}

}  
}  

already_AddRefed<Layer>
nsDisplayRemote::BuildLayer(nsDisplayListBuilder* aBuilder,
                            LayerManager* aManager,
                            const ContainerLayerParameters& aContainerParameters)
{
  int32_t appUnitsPerDevPixel = mFrame->PresContext()->AppUnitsPerDevPixel();
  nsIntRect visibleRect = GetVisibleRect().ToNearestPixels(appUnitsPerDevPixel);
  visibleRect += aContainerParameters.mOffset;
  nsRefPtr<Layer> layer = mRemoteFrame->BuildLayer(aBuilder, mFrame, aManager, visibleRect, this, aContainerParameters);
  return layer.forget();
}

void
nsDisplayRemote::HitTest(nsDisplayListBuilder* aBuilder, const nsRect& aRect,
                         HitTestState* aState, nsTArray<nsIFrame*> *aOutFrames)
{
  if (mRemoteFrame->HitTest(aRect)) {
    aOutFrames->AppendElement(mFrame);
  }
}

