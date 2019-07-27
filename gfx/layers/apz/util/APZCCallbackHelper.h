




#ifndef mozilla_layers_APZCCallbackHelper_h
#define mozilla_layers_APZCCallbackHelper_h

#include "FrameMetrics.h"
#include "mozilla/EventForwards.h"
#include "mozilla/layers/APZUtils.h"
#include "nsIDOMWindowUtils.h"

class nsIContent;
class nsIDocument;
class nsIWidget;
template<class T> struct already_AddRefed;

namespace mozilla {
namespace layers {



struct SetAllowedTouchBehaviorCallback {
public:
  NS_INLINE_DECL_REFCOUNTING(SetAllowedTouchBehaviorCallback)
  virtual void Run(uint64_t aInputBlockId, const nsTArray<TouchBehaviorFlags>& aFlags) const = 0;
protected:
  virtual ~SetAllowedTouchBehaviorCallback() {}
};







class APZCCallbackHelper
{
    typedef mozilla::layers::FrameMetrics FrameMetrics;
    typedef mozilla::layers::ScrollableLayerGuid ScrollableLayerGuid;

public:
    



    static bool HasValidPresShellId(nsIDOMWindowUtils* aUtils,
                                    const FrameMetrics& aMetrics);

    





    static void UpdateRootFrame(nsIDOMWindowUtils* aUtils,
                                nsIPresShell* aPresShell,
                                FrameMetrics& aMetrics);

    





    static void UpdateSubFrame(nsIContent* aContent,
                               FrameMetrics& aMetrics);

    
    static already_AddRefed<nsIDOMWindowUtils> GetDOMWindowUtils(const nsIDocument* aDoc);

    

    static already_AddRefed<nsIDOMWindowUtils> GetDOMWindowUtils(const nsIContent* aContent);

    



    static bool GetOrCreateScrollIdentifiers(nsIContent* aContent,
                                             uint32_t* aPresShellIdOut,
                                             FrameMetrics::ViewID* aViewIdOut);

    




    static void RequestFlingSnap(const FrameMetrics::ViewID& aScrollId,
                                 const mozilla::CSSPoint& aDestination);

    

    static void AcknowledgeScrollUpdate(const FrameMetrics::ViewID& aScrollId,
                                        const uint32_t& aScrollGeneration);

    






    static CSSPoint ApplyCallbackTransform(const CSSPoint& aInput,
                                           const ScrollableLayerGuid& aGuid,
                                           float aPresShellResolution);

    


    static mozilla::LayoutDeviceIntPoint
    ApplyCallbackTransform(const LayoutDeviceIntPoint& aPoint,
                           const ScrollableLayerGuid& aGuid,
                           const CSSToLayoutDeviceScale& aScale,
                           float aPresShellResolution);

    

    static void ApplyCallbackTransform(WidgetTouchEvent& aEvent,
                                       const ScrollableLayerGuid& aGuid,
                                       const CSSToLayoutDeviceScale& aScale,
                                       float aPresShellResolution);

    


    static nsEventStatus DispatchWidgetEvent(WidgetGUIEvent& aEvent);

    

    static nsEventStatus DispatchSynthesizedMouseEvent(uint32_t aMsg,
                                                       uint64_t aTime,
                                                       const LayoutDevicePoint& aRefPoint,
                                                       Modifiers aModifiers,
                                                       nsIWidget* aWidget);

    

    static bool DispatchMouseEvent(const nsCOMPtr<nsIDOMWindowUtils>& aUtils,
                                   const nsString& aType,
                                   const CSSPoint& aPoint,
                                   int32_t aButton,
                                   int32_t aClickCount,
                                   int32_t aModifiers,
                                   bool aIgnoreRootScrollFrame,
                                   unsigned short aInputSourceArg);

    

    static void FireSingleTapEvent(const LayoutDevicePoint& aPoint,
                                   Modifiers aModifiers,
                                   nsIWidget* aWidget);

    








    static void SendSetTargetAPZCNotification(nsIWidget* aWidget,
                                              nsIDocument* aDocument,
                                              const WidgetGUIEvent& aEvent,
                                              const ScrollableLayerGuid& aGuid,
                                              uint64_t aInputBlockId);

    

    static void SendSetAllowedTouchBehaviorNotification(nsIWidget* aWidget,
                                                         const WidgetTouchEvent& aEvent,
                                                         uint64_t aInputBlockId,
                                                         const nsRefPtr<SetAllowedTouchBehaviorCallback>& aCallback);

    
    static void NotifyMozMouseScrollEvent(const FrameMetrics::ViewID& aScrollId, const nsString& aEvent);
};

}
}

#endif 
