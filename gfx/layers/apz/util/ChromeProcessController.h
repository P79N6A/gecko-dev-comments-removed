




#ifndef mozilla_layers_ChromeProcessController_h
#define mozilla_layers_ChromeProcessController_h

#include "mozilla/layers/GeckoContentController.h"
#include "nsCOMPtr.h"
#include "nsRefPtr.h"

class nsIDOMWindowUtils;
class nsIDocument;
class nsIPresShell;
class nsIWidget;

class MessageLoop;

namespace mozilla {

namespace layers {

class APZEventState;



class ChromeProcessController : public mozilla::layers::GeckoContentController
{
  typedef mozilla::layers::FrameMetrics FrameMetrics;
  typedef mozilla::layers::ScrollableLayerGuid ScrollableLayerGuid;

public:
  explicit ChromeProcessController(nsIWidget* aWidget, APZEventState* aAPZEventState);
  virtual void Destroy() override;

  
  virtual void RequestContentRepaint(const FrameMetrics& aFrameMetrics) override;
  virtual void PostDelayedTask(Task* aTask, int aDelayMs) override;
  virtual void RequestFlingSnap(const FrameMetrics::ViewID& aScrollId,
                                const mozilla::CSSPoint& aDestination) override;
  virtual void AcknowledgeScrollUpdate(const FrameMetrics::ViewID& aScrollId,
                                       const uint32_t& aScrollGeneration) override;

  virtual void HandleDoubleTap(const mozilla::CSSPoint& aPoint, Modifiers aModifiers,
                               const ScrollableLayerGuid& aGuid) override {}
  virtual void HandleSingleTap(const mozilla::CSSPoint& aPoint, Modifiers aModifiers,
                               const ScrollableLayerGuid& aGuid) override;
  virtual void HandleLongTap(const mozilla::CSSPoint& aPoint, Modifiers aModifiers,
                               const ScrollableLayerGuid& aGuid,
                               uint64_t aInputBlockId) override;
  virtual void SendAsyncScrollDOMEvent(bool aIsRoot, const mozilla::CSSRect &aContentRect,
                                       const mozilla::CSSSize &aScrollableSize) override {}
  virtual void NotifyAPZStateChange(const ScrollableLayerGuid& aGuid,
                                    APZStateChange aChange,
                                    int aArg) override;
  virtual void NotifyMozMouseScrollEvent(const FrameMetrics::ViewID& aScrollId,
                                         const nsString& aEvent) override;
private:
  nsCOMPtr<nsIWidget> mWidget;
  nsRefPtr<APZEventState> mAPZEventState;
  MessageLoop* mUILoop;

  void InitializeRoot();
  float GetPresShellResolution() const;
  nsIPresShell* GetPresShell() const;
  nsIDocument* GetDocument() const;
  already_AddRefed<nsIDOMWindowUtils> GetDOMWindowUtils() const;
};

} 
} 

#endif 
