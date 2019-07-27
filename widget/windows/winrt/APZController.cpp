



#include "APZController.h"
#include "base/message_loop.h"
#include "mozilla/layers/GeckoContentController.h"
#include "nsThreadUtils.h"
#include "MetroUtils.h"
#include "nsPrintfCString.h"
#include "mozilla/layers/APZCCallbackHelper.h"
#include "nsIDocument.h"
#include "nsPresContext.h"
#include "nsIDOMElement.h"
#include "mozilla/dom/Element.h"
#include "nsIDOMWindowUtils.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsLayoutUtils.h"
#include "mozilla/TouchEvents.h"



#ifdef DEBUG_CONTROLLER
#include "WinUtils.h"
using namespace mozilla::widget;
#endif

namespace mozilla {
namespace widget {
namespace winrt {

nsRefPtr<mozilla::layers::APZCTreeManager> APZController::sAPZC;





static bool
IsTab(nsCOMPtr<nsIDocument>& aSubDocument)
{
  nsRefPtr<nsIDocument> parent = aSubDocument->GetParentDocument();
  if (!parent) {
    NS_WARNING("huh? IsTab should always get a sub document for a parameter");
    return false;
  }
  return parent->IsRootDisplayDocument();
}




static bool
GetDOMTargets(uint64_t aScrollId,
              nsCOMPtr<nsIDocument>& aSubDocument,
              nsCOMPtr<nsIContent>& aTargetContent)
{
  
  aTargetContent = nsLayoutUtils::FindContentFor(aScrollId);
  if (!aTargetContent) {
    return false;
  }
  nsCOMPtr<mozilla::dom::Element> domElement = do_QueryInterface(aTargetContent);
  if (!domElement) {
    return false;
  }

  aSubDocument = domElement->OwnerDoc();

  if (!aSubDocument) {
    return false;
  }

  
  
  if (aSubDocument->GetRootElement() == domElement && IsTab(aSubDocument)) {
    aTargetContent = nullptr;
  }

  return true;
}

void
APZController::SetPendingResponseFlusher(APZPendingResponseFlusher* aFlusher)
{
  mFlusher = aFlusher;
}

void
APZController::ContentReceivedInputBlock(const uint64_t aInputBlockId, bool aPreventDefault)
{
  if (!sAPZC) {
    return;
  }
  sAPZC->ContentReceivedInputBlock(aInputBlockId, aPreventDefault);
}

bool
APZController::HitTestAPZC(ScreenIntPoint& aPoint)
{
  if (!sAPZC) {
    return false;
  }
  return sAPZC->HitTestAPZC(aPoint);
}

void
APZController::TransformCoordinateToGecko(const ScreenIntPoint& aPoint,
                                          LayoutDeviceIntPoint* aRefPointOut)
{
  if (!sAPZC || !aRefPointOut) {
    return;
  }
  sAPZC->TransformCoordinateToGecko(aPoint, aRefPointOut);
}

nsEventStatus
APZController::ReceiveInputEvent(WidgetInputEvent* aEvent,
                                 ScrollableLayerGuid* aOutTargetGuid,
                                 uint64_t* aOutInputBlockId)
{
  MOZ_ASSERT(aEvent);

  if (!sAPZC) {
    return nsEventStatus_eIgnore;
  }
  return sAPZC->ReceiveInputEvent(*aEvent->AsInputEvent(), aOutTargetGuid, aOutInputBlockId);
}



void
APZController::RequestContentRepaint(const FrameMetrics& aFrameMetrics)
{
#ifdef DEBUG_CONTROLLER
  WinUtils::Log("APZController::RequestContentRepaint scrollid=%I64d",
    aFrameMetrics.GetScrollId());
#endif

  
  MOZ_ASSERT(NS_IsMainThread());

#ifdef DEBUG_CONTROLLER
  WinUtils::Log("APZController: mScrollOffset: %f %f", aFrameMetrics.mScrollOffset.x,
    aFrameMetrics.mScrollOffset.y);
#endif

  nsCOMPtr<nsIDocument> subDocument;
  nsCOMPtr<nsIContent> targetContent;
  if (!GetDOMTargets(aFrameMetrics.GetScrollId(),
                     subDocument, targetContent)) {
    return;
  }

  
  
  if (targetContent) {
#ifdef DEBUG_CONTROLLER
    WinUtils::Log("APZController: detected subframe or content editable");
#endif
    FrameMetrics metrics = aFrameMetrics;
    mozilla::layers::APZCCallbackHelper::UpdateSubFrame(targetContent, metrics);
    return;
  }

#ifdef DEBUG_CONTROLLER
  WinUtils::Log("APZController: detected tab");
#endif

  
  nsCOMPtr<nsIDOMWindowUtils> utils;
  nsCOMPtr<nsIDOMWindow> window = subDocument->GetDefaultView();
  if (window) {
    utils = do_GetInterface(window);
    if (utils) {
      FrameMetrics metrics = aFrameMetrics;
      if (subDocument->GetShell()) {
        mozilla::layers::APZCCallbackHelper::UpdateRootFrame(utils, subDocument->GetShell(), metrics);
      }

#ifdef DEBUG_CONTROLLER
      WinUtils::Log("APZController: %I64d mDisplayPortMargins: %0.2f %0.2f %0.2f %0.2f",
        metrics.GetScrollId(),
        metrics.GetDisplayPortMargins().left,
        metrics.GetDisplayPortMargins().top,
        metrics.GetDisplayPortMargins().right,
        metrics.GetDisplayPortMargins().bottom);
#endif
    }
  }
}

void
APZController::RequestFlingSnap(const FrameMetrics::ViewID& aScrollId,
                                const mozilla::CSSPoint& aDestination)
{
#ifdef DEBUG_CONTROLLER
  WinUtils::Log("APZController::RequestFlingSnap scrollid=%I64d destination: %lu %lu",
    aScrollId, aDestination.x, aDestination.y);
#endif
  mozilla::layers::APZCCallbackHelper::RequestFlingSnap(aScrollId, aDestination);
}

void
APZController::AcknowledgeScrollUpdate(const FrameMetrics::ViewID& aScrollId,
                                       const uint32_t& aScrollGeneration)
{
#ifdef DEBUG_CONTROLLER
  WinUtils::Log("APZController::AcknowledgeScrollUpdate scrollid=%I64d gen=%lu",
    aScrollId, aScrollGeneration);
#endif
  mozilla::layers::APZCCallbackHelper::AcknowledgeScrollUpdate(aScrollId, aScrollGeneration);
}

void
APZController::HandleDoubleTap(const CSSPoint& aPoint,
                               Modifiers aModifiers,
                               const ScrollableLayerGuid& aGuid)
{
}

void
APZController::HandleSingleTap(const CSSPoint& aPoint,
                               Modifiers aModifiers,
                               const ScrollableLayerGuid& aGuid)
{
}

void
APZController::HandleLongTap(const CSSPoint& aPoint,
                             Modifiers aModifiers,
                             const mozilla::layers::ScrollableLayerGuid& aGuid,
                             uint64_t aInputBlockId)
{
  if (mFlusher) {
    mFlusher->FlushPendingContentResponse();
  }
  ContentReceivedInputBlock(aInputBlockId, false);
}


void
APZController::SendAsyncScrollDOMEvent(bool aIsRoot,
                                       const CSSRect &aContentRect,
                                       const CSSSize &aScrollableSize)
{
}

void
APZController::PostDelayedTask(Task* aTask, int aDelayMs)
{
  MessageLoop::current()->PostDelayedTask(FROM_HERE, aTask, aDelayMs);
}

bool
APZController::GetRootZoomConstraints(ZoomConstraints* aOutConstraints)
{
  if (aOutConstraints) {
    
    
    aOutConstraints->mAllowZoom = true;
    aOutConstraints->mAllowDoubleTapZoom = false;
    aOutConstraints->mMinZoom = CSSToParentLayerScale(0.25f);
    aOutConstraints->mMaxZoom = CSSToParentLayerScale(4.0f);
    return true;
  }
  return false;
}



class TransformedStartEvent : public nsRunnable
{
  NS_IMETHOD Run() {
    MetroUtils::FireObserver("apzc-transform-start", L"");
    return NS_OK;
  }
};

class TransformedEndEvent : public nsRunnable
{
  NS_IMETHOD Run() {
    MetroUtils::FireObserver("apzc-transform-end", L"");
    return NS_OK;
  }
};

void
APZController::NotifyAPZStateChange(const ScrollableLayerGuid& aGuid,
                                    APZStateChange aChange,
                                    int aArg)
{
  switch (aChange) {
    case APZStateChange::TransformBegin:
    {
      if (NS_IsMainThread()) {
        MetroUtils::FireObserver("apzc-transform-begin", L"");
        return;
      }
      nsCOMPtr<nsIRunnable> runnable = new TransformedStartEvent();
      NS_DispatchToMainThread(runnable);
      break;
    }
    case APZStateChange::TransformEnd:
    {
      if (NS_IsMainThread()) {
        MetroUtils::FireObserver("apzc-transform-end", L"");
        return;
      }
      nsCOMPtr<nsIRunnable> runnable = new TransformedEndEvent();
      NS_DispatchToMainThread(runnable);
      break;
    }
    default:
    {
      
      break;
    }
  }
}

} } }
