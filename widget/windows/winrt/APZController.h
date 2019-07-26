



#pragma once

#include "mozwrlbase.h"

#include "mozilla/layers/GeckoContentController.h"
#include "mozilla/layers/APZCTreeManager.h"
#include "mozilla/EventForwards.h"
#include "FrameMetrics.h"
#include "Units.h"

class nsIWidgetListener;

namespace mozilla {
namespace widget {
namespace winrt {

class APZController :
  public mozilla::layers::GeckoContentController
{
  typedef mozilla::layers::FrameMetrics FrameMetrics;
  typedef mozilla::layers::ScrollableLayerGuid ScrollableLayerGuid;

public:
  APZController() :
    mWidgetListener(nullptr)
  {
  }

  
  virtual void RequestContentRepaint(const FrameMetrics& aFrameMetrics);
  virtual void HandleDoubleTap(const mozilla::CSSIntPoint& aPoint);
  virtual void HandleSingleTap(const mozilla::CSSIntPoint& aPoint);
  virtual void HandleLongTap(const mozilla::CSSIntPoint& aPoint);
  virtual void SendAsyncScrollDOMEvent(bool aIsRoot, const mozilla::CSSRect &aContentRect, const mozilla::CSSSize &aScrollableSize);
  virtual void PostDelayedTask(Task* aTask, int aDelayMs);
  virtual void HandlePanBegin();
  virtual void HandlePanEnd();
  
  void SetWidgetListener(nsIWidgetListener* aWidgetListener);
  void UpdateScrollOffset(const mozilla::layers::ScrollableLayerGuid& aScrollLayerId, CSSIntPoint& aScrollOffset);

  bool HitTestAPZC(mozilla::ScreenIntPoint& aPoint);
  void TransformCoordinateToGecko(const mozilla::ScreenIntPoint& aPoint,
                                  LayoutDeviceIntPoint* aRefPointOut);
  void ContentReceivedTouch(const ScrollableLayerGuid& aGuid, bool aPreventDefault);
  nsEventStatus ReceiveInputEvent(mozilla::WidgetInputEvent* aEvent,
                                  ScrollableLayerGuid* aOutTargetGuid);
  nsEventStatus ReceiveInputEvent(mozilla::WidgetInputEvent* aInEvent,
                                  ScrollableLayerGuid* aOutTargetGuid,
                                  mozilla::WidgetInputEvent* aOutEvent);

public:
  
  static nsRefPtr<mozilla::layers::APZCTreeManager> sAPZC;

private:
  nsIWidgetListener* mWidgetListener;
  ScrollableLayerGuid mLastScrollLayerGuid;
  CSSIntPoint mLastScrollOffset;
};

} } }