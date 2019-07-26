



#pragma once

#include "mozwrlbase.h"

#include "mozilla/layers/GeckoContentController.h"
#include "FrameMetrics.h"
#include "Units.h"

namespace mozilla {
namespace widget {
namespace winrt {

class APZController : public mozilla::layers::GeckoContentController
{
  typedef mozilla::layers::FrameMetrics FrameMetrics;

public:
  virtual void RequestContentRepaint(const FrameMetrics& aFrameMetrics);
  virtual void HandleDoubleTap(const mozilla::CSSIntPoint& aPoint);
  virtual void HandleSingleTap(const mozilla::CSSIntPoint& aPoint);
  virtual void HandleLongTap(const mozilla::CSSIntPoint& aPoint);
  virtual void SendAsyncScrollDOMEvent(FrameMetrics::ViewID aScrollId, const mozilla::CSSRect &aContentRect, const mozilla::CSSSize &aScrollableSize);
  virtual void PostDelayedTask(Task* aTask, int aDelayMs);
  virtual void HandlePanBegin();
  virtual void HandlePanEnd();
};

} } }