




#ifndef mozilla_layers_ChromeProcessController_h
#define mozilla_layers_ChromeProcessController_h

#include "mozilla/layers/GeckoContentController.h"
#include "nsCOMPtr.h"
#include "nsRefPtr.h"

class nsIDOMWindowUtils;
class nsIPresShell;
class nsIWidget;

class MessageLoop;

namespace mozilla {

namespace layers {

class APZEventState;
class CompositorParent;



class ChromeProcessController : public mozilla::layers::GeckoContentController
{
  typedef mozilla::layers::FrameMetrics FrameMetrics;
  typedef mozilla::layers::ScrollableLayerGuid ScrollableLayerGuid;

public:
  explicit ChromeProcessController(nsIWidget* aWidget, APZEventState* aAPZEventState);
  virtual void Destroy() MOZ_OVERRIDE;

  
  virtual void RequestContentRepaint(const FrameMetrics& aFrameMetrics) MOZ_OVERRIDE;
  virtual void PostDelayedTask(Task* aTask, int aDelayMs) MOZ_OVERRIDE;
  virtual void AcknowledgeScrollUpdate(const FrameMetrics::ViewID& aScrollId,
                                       const uint32_t& aScrollGeneration) MOZ_OVERRIDE;

  virtual void HandleDoubleTap(const mozilla::CSSPoint& aPoint, int32_t aModifiers,
                               const ScrollableLayerGuid& aGuid) MOZ_OVERRIDE {}
  virtual void HandleSingleTap(const mozilla::CSSPoint& aPoint, int32_t aModifiers,
                               const ScrollableLayerGuid& aGuid) MOZ_OVERRIDE;
  virtual void HandleLongTap(const mozilla::CSSPoint& aPoint, int32_t aModifiers,
                               const ScrollableLayerGuid& aGuid,
                               uint64_t aInputBlockId) MOZ_OVERRIDE;
  virtual void HandleLongTapUp(const CSSPoint& aPoint, int32_t aModifiers,
                               const ScrollableLayerGuid& aGuid) MOZ_OVERRIDE;
  virtual void SendAsyncScrollDOMEvent(bool aIsRoot, const mozilla::CSSRect &aContentRect,
                                       const mozilla::CSSSize &aScrollableSize) MOZ_OVERRIDE {}
  virtual void NotifyAPZStateChange(const ScrollableLayerGuid& aGuid,
                                    APZStateChange aChange,
                                    int aArg) MOZ_OVERRIDE;
private:
  nsCOMPtr<nsIWidget> mWidget;
  nsRefPtr<APZEventState> mAPZEventState;
  MessageLoop* mUILoop;

  void InitializeRoot();
  float GetPresShellResolution() const;
  nsIPresShell* GetPresShell() const;
  already_AddRefed<nsIDOMWindowUtils> GetDOMWindowUtils() const;
};

} 
} 

#endif 
