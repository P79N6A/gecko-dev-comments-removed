



#pragma once

#include "mozwrlbase.h"

#include "mozilla/layers/GeckoContentController.h"
#include "mozilla/layers/APZCTreeManager.h"
#include "mozilla/EventForwards.h"
#include "FrameMetrics.h"
#include "Units.h"

namespace mozilla {
namespace widget {
namespace winrt {

class APZPendingResponseFlusher
{
public:
  virtual void FlushPendingContentResponse() = 0;
};

class APZController :
  public mozilla::layers::GeckoContentController
{
  typedef mozilla::layers::FrameMetrics FrameMetrics;
  typedef mozilla::layers::ScrollableLayerGuid ScrollableLayerGuid;
  typedef mozilla::layers::ZoomConstraints ZoomConstraints;

public:
  APZController() :
    mFlusher(nullptr)
  {
  }

  
  virtual void RequestContentRepaint(const FrameMetrics& aFrameMetrics);
  virtual void RequestFlingSnap(const FrameMetrics::ViewID& aScrollId,
                                const mozilla::CSSPoint& aDestination);
  virtual void AcknowledgeScrollUpdate(const FrameMetrics::ViewID& aScrollId, const uint32_t& aScrollGeneration);
  virtual void HandleDoubleTap(const mozilla::CSSPoint& aPoint,
                               Modifiers aModifiers,
                               const mozilla::layers::ScrollableLayerGuid& aGuid);
  virtual void HandleSingleTap(const mozilla::CSSPoint& aPoint,
                               Modifiers aModifiers,
                               const mozilla::layers::ScrollableLayerGuid& aGuid);
  virtual void HandleLongTap(const mozilla::CSSPoint& aPoint,
                             Modifiers aModifiers,
                             const mozilla::layers::ScrollableLayerGuid& aGuid,
                             uint64_t aInputBlockId);
  virtual void HandleLongTapUp(const mozilla::CSSPoint& aPoint,
                               Modifiers aModifiers,
                               const mozilla::layers::ScrollableLayerGuid& aGuid);
  virtual void SendAsyncScrollDOMEvent(bool aIsRoot, const mozilla::CSSRect &aContentRect, const mozilla::CSSSize &aScrollableSize);
  virtual void PostDelayedTask(Task* aTask, int aDelayMs);
  virtual bool GetRootZoomConstraints(ZoomConstraints* aOutConstraints);
  virtual void NotifyAPZStateChange(const ScrollableLayerGuid& aGuid,
                                    APZStateChange aChange,
                                    int aArg);

  void SetPendingResponseFlusher(APZPendingResponseFlusher* aFlusher);
  
  bool HitTestAPZC(mozilla::ScreenIntPoint& aPoint);
  void TransformCoordinateToGecko(const mozilla::ScreenIntPoint& aPoint,
                                  LayoutDeviceIntPoint* aRefPointOut);
  void ContentReceivedInputBlock(uint64_t aInputBlockId, bool aPreventDefault);
  nsEventStatus ReceiveInputEvent(mozilla::WidgetInputEvent* aEvent,
                                  ScrollableLayerGuid* aOutTargetGuid,
                                  uint64_t* aOutInputBlockId);

public:
  
  static nsRefPtr<mozilla::layers::APZCTreeManager> sAPZC;

private:
  APZPendingResponseFlusher* mFlusher;
};

} } }
