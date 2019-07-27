




#ifndef mozilla_layers_APZEventState_h
#define mozilla_layers_APZEventState_h

#include <stdint.h>

#include "FrameMetrics.h"     
#include "Units.h"
#include "mozilla/EventForwards.h"
#include "mozilla/layers/GeckoContentController.h"  
#include "nsCOMPtr.h"
#include "nsISupportsImpl.h"  
#include "nsIWeakReferenceUtils.h"  
#include "nsRefPtr.h"

template <class> class nsCOMPtr;
class nsIPresShell;
class nsIWidget;

namespace mozilla {
namespace layers {

class ActiveElementManager;

struct ContentReceivedInputBlockCallback {
public:
  NS_INLINE_DECL_REFCOUNTING(ContentReceivedInputBlockCallback);
  virtual void Run(const ScrollableLayerGuid& aGuid,
                   uint64_t aInputBlockId,
                   bool aPreventDefault) const = 0;
protected:
  virtual ~ContentReceivedInputBlockCallback() {}
};





class APZEventState {
  typedef GeckoContentController::APZStateChange APZStateChange;
  typedef FrameMetrics::ViewID ViewID;
public:
  APZEventState(nsIWidget* aWidget,
                const nsRefPtr<ContentReceivedInputBlockCallback>& aCallback);

  NS_INLINE_DECL_REFCOUNTING(APZEventState);

  void ProcessSingleTap(const CSSPoint& aPoint,
                        Modifiers aModifiers,
                        const ScrollableLayerGuid& aGuid,
                        float aPresShellResolution);
  void ProcessLongTap(const nsCOMPtr<nsIPresShell>& aUtils,
                      const CSSPoint& aPoint,
                      Modifiers aModifiers,
                      const ScrollableLayerGuid& aGuid,
                      uint64_t aInputBlockId,
                      float aPresShellResolution);
  void ProcessTouchEvent(const WidgetTouchEvent& aEvent,
                         const ScrollableLayerGuid& aGuid,
                         uint64_t aInputBlockId,
                         nsEventStatus aApzResponse);
  void ProcessWheelEvent(const WidgetWheelEvent& aEvent,
                         const ScrollableLayerGuid& aGuid,
                         uint64_t aInputBlockId);
  void ProcessAPZStateChange(const nsCOMPtr<nsIDocument>& aDocument,
                             ViewID aViewId,
                             APZStateChange aChange,
                             int aArg);
private:
  ~APZEventState();
  bool SendPendingTouchPreventedResponse(bool aPreventDefault,
                                         const ScrollableLayerGuid& aGuid);
  already_AddRefed<nsIWidget> GetWidget() const;
private:
  nsWeakPtr mWidget;
  nsRefPtr<ActiveElementManager> mActiveElementManager;
  nsRefPtr<ContentReceivedInputBlockCallback> mContentReceivedInputBlockCallback;
  bool mPendingTouchPreventedResponse;
  ScrollableLayerGuid mPendingTouchPreventedGuid;
  uint64_t mPendingTouchPreventedBlockId;
  bool mEndTouchIsClick;
  bool mTouchEndCancelled;
  int mActiveAPZTransforms;
};

}
}

#endif 
