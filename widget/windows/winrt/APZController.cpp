



#include "APZController.h"
#include "base/message_loop.h"
#include "mozilla/layers/GeckoContentController.h"
#include "nsThreadUtils.h"
#include "MetroUtils.h"
#include "nsPrintfCString.h"
#include "nsIWidgetListener.h"
#include "APZCCallbackHelper.h"
#include "nsIDocument.h"
#include "nsPresContext.h"
#include "nsIDOMElement.h"
#include "mozilla/dom/Element.h"
#include "nsIDOMWindowUtils.h"
#include "nsIInterfaceRequestorUtils.h"



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
GetDOMTargets(nsIPresShell* aPresShell, uint64_t aScrollId,
              nsCOMPtr<nsIDocument>& aSubDocument,
              nsCOMPtr<nsIDOMElement>& aTargetElement)
{
  MOZ_ASSERT(aPresShell);
  nsRefPtr<nsIDocument> rootDocument = aPresShell->GetDocument();
  if (!rootDocument) {
    return false;
  }
  nsCOMPtr<nsIDOMWindowUtils> rootUtils;
  nsCOMPtr<nsIDOMWindow> rootWindow = rootDocument->GetDefaultView();
  if (!rootWindow) {
    return false;
  }
  rootUtils = do_GetInterface(rootWindow);
  if (!rootUtils) {
    return false;
  }

  
  rootUtils->FindElementWithViewId(aScrollId, getter_AddRefs(aTargetElement));
  if (!aTargetElement) {
    return false;
  }
  nsCOMPtr<mozilla::dom::Element> domElement = do_QueryInterface(aTargetElement);
  if (!domElement) {
    return false;
  }

  aSubDocument = domElement->OwnerDoc();

  if (!aSubDocument) {
    return false;
  }

  
  
  if (aSubDocument->GetRootElement() == domElement && IsTab(aSubDocument)) {
    aTargetElement = nullptr;
  }

  return true;
}

class RequestContentRepaintEvent : public nsRunnable
{
  typedef mozilla::layers::FrameMetrics FrameMetrics;

public:
  RequestContentRepaintEvent(const FrameMetrics& aFrameMetrics,
                             nsIWidgetListener* aListener,
                             CSSIntPoint* aLastOffsetOut) :
    mFrameMetrics(aFrameMetrics),
    mWidgetListener(aListener),
    mLastOffsetOut(aLastOffsetOut)
  {
  }

  NS_IMETHOD Run() {
    
    MOZ_ASSERT(NS_IsMainThread());

#ifdef DEBUG_CONTROLLER
    WinUtils::Log("APZController: mScrollOffset: %f %f", mFrameMetrics.mScrollOffset.x,
      mFrameMetrics.mScrollOffset.y);
#endif

    nsIPresShell* presShell = mWidgetListener->GetPresShell();
    if (!presShell) {
      return NS_OK;
    }

    nsCOMPtr<nsIDocument> subDocument;
    nsCOMPtr<nsIDOMElement> targetElement;
    if (!GetDOMTargets(presShell, mFrameMetrics.mScrollId,
                       subDocument, targetElement)) {
      return NS_OK;
    }

    
    
    if (targetElement) {
#ifdef DEBUG_CONTROLLER
      WinUtils::Log("APZController: detected subframe or content editable");
#endif
      nsCOMPtr<nsIContent> content = do_QueryInterface(targetElement);
      if (content) {
        APZCCallbackHelper::UpdateSubFrame(content, mFrameMetrics);
      }
      return NS_OK;
    }

#ifdef DEBUG_CONTROLLER
    WinUtils::Log("APZController: detected tab");
#endif

    
    nsCOMPtr<nsIDOMWindowUtils> utils;
    nsCOMPtr<nsIDOMWindow> window = subDocument->GetDefaultView();
    if (window) {
      utils = do_GetInterface(window);
      if (utils) {
        APZCCallbackHelper::UpdateRootFrame(utils, mFrameMetrics);

        
        
        CSSIntPoint actualScrollOffset;
        utils->GetScrollXY(false, &actualScrollOffset.x, &actualScrollOffset.y);
        if (mLastOffsetOut) {
          *mLastOffsetOut = actualScrollOffset;
        }

#ifdef DEBUG_CONTROLLER
        WinUtils::Log("APZController: %I64d mDisplayPort: %0.2f %0.2f %0.2f %0.2f",
          mFrameMetrics.mScrollId,
          mFrameMetrics.mDisplayPort.x,
          mFrameMetrics.mDisplayPort.y,
          mFrameMetrics.mDisplayPort.width,
          mFrameMetrics.mDisplayPort.height);
#endif
      }
    }
    return NS_OK;
  }
protected:
  FrameMetrics mFrameMetrics;
  nsIWidgetListener* mWidgetListener;
  CSSIntPoint* mLastOffsetOut;
};

void
APZController::SetWidgetListener(nsIWidgetListener* aWidgetListener)
{
  mWidgetListener = aWidgetListener;
}



void
APZController::RequestContentRepaint(const FrameMetrics& aFrameMetrics)
{
  
  if (!mWidgetListener) {
    NS_WARNING("Can't update display port, !mWidgetListener");
    return;
  }

#ifdef DEBUG_CONTROLLER
  WinUtils::Log("APZController::RequestContentRepaint scroll id = %I64d",
    aFrameMetrics.mScrollId);
#endif
  nsCOMPtr<nsIRunnable> r1 = new RequestContentRepaintEvent(aFrameMetrics,
                                                            mWidgetListener,
                                                            &mLastScrollOffset);
  if (!NS_IsMainThread()) {
    NS_DispatchToMainThread(r1);
  } else {
    r1->Run();
  }
}




void
APZController::UpdateScrollOffset(const mozilla::layers::ScrollableLayerGuid& aScrollLayerId,
                                  CSSIntPoint& aScrollOffset)
{
#ifdef DEBUG_CONTROLLER
  WinUtils::Log("APZController::UpdateScrollOffset: %d %d == %d %d",
    aScrollOffset.x, aScrollOffset.y,
    mLastScrollOffset.x, mLastScrollOffset.y);
#endif
  
  if (!sAPZC || mLastScrollOffset == aScrollOffset) {
    return;
  }
  sAPZC->UpdateScrollOffset(aScrollLayerId, aScrollOffset);
}



void
APZController::HandleDoubleTap(const CSSIntPoint& aPoint)
{
}

void
APZController::HandleSingleTap(const CSSIntPoint& aPoint)
{
}

void
APZController::HandleLongTap(const CSSIntPoint& aPoint)
{
}


void
APZController::SendAsyncScrollDOMEvent(FrameMetrics::ViewID aScrollId,
                                       const CSSRect &aContentRect,
                                       const CSSSize &aScrollableSize)
{
}

void
APZController::PostDelayedTask(Task* aTask, int aDelayMs)
{
  MessageLoop::current()->PostDelayedTask(FROM_HERE, aTask, aDelayMs);
}



void
APZController::HandlePanBegin()
{
  MetroUtils::FireObserver("apzc-handle-pan-begin", L"");
}

void
APZController::HandlePanEnd()
{
  MetroUtils::FireObserver("apzc-handle-pan-end", L"");
}

} } }