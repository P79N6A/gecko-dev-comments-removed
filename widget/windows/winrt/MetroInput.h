




#pragma once


#include "APZController.h"
#include "keyboardlayout.h"   
#include "nsBaseHashtable.h"  
#include "nsHashKeys.h"       
#include "mozwrlbase.h"
#include "nsDeque.h"
#include "mozilla/EventForwards.h"
#include "mozilla/layers/APZCTreeManager.h"


#include <EventToken.h>     
#include <stdint.h>         
#include <wrl\client.h>     
#include <wrl\implements.h> 


class MetroWidget;

namespace mozilla {
namespace dom {
class Touch;
}
}


namespace ABI {
  namespace Windows {
    namespace Devices {
      namespace Input {
        enum PointerDeviceType;
      }
    };
    namespace Foundation {
      struct Point;
    };
    namespace UI {
      namespace Core {
        struct ICoreWindow;
        struct IAcceleratorKeyEventArgs;
        struct IKeyEventArgs;
        struct IPointerEventArgs;
      };
      namespace Input {
        struct IEdgeGesture;
        struct IEdgeGestureEventArgs;
        struct IGestureRecognizer;
        struct IManipulationCompletedEventArgs;
        struct IManipulationStartedEventArgs;
        struct IManipulationUpdatedEventArgs;
        struct IPointerPoint;
        struct IRightTappedEventArgs;
        struct ITappedEventArgs;
        struct ManipulationDelta;
      };
    };
  };
};

namespace mozilla {
namespace widget {
namespace winrt {

class MetroInput : public Microsoft::WRL::RuntimeClass<IInspectable>,
                   public APZPendingResponseFlusher
{
  InspectableClass(L"MetroInput", BaseTrust);

private:
  typedef mozilla::layers::AllowedTouchBehavior AllowedTouchBehavior;
  typedef uint32_t TouchBehaviorFlags;

  
  typedef ABI::Windows::Devices::Input::PointerDeviceType PointerDeviceType;

  
  typedef ABI::Windows::Foundation::Point Point;

  
  typedef ABI::Windows::UI::Core::ICoreWindow ICoreWindow;
  typedef ABI::Windows::UI::Core::IAcceleratorKeyEventArgs \
                                  IAcceleratorKeyEventArgs;
  typedef ABI::Windows::UI::Core::IKeyEventArgs IKeyEventArgs;
  typedef ABI::Windows::UI::Core::IPointerEventArgs IPointerEventArgs;

  
  typedef ABI::Windows::UI::Input::IEdgeGesture IEdgeGesture;
  typedef ABI::Windows::UI::Input::IEdgeGestureEventArgs IEdgeGestureEventArgs;
  typedef ABI::Windows::UI::Input::IGestureRecognizer IGestureRecognizer;
  typedef ABI::Windows::UI::Input::IManipulationCompletedEventArgs \
                                   IManipulationCompletedEventArgs;
  typedef ABI::Windows::UI::Input::IManipulationStartedEventArgs \
                                   IManipulationStartedEventArgs;
  typedef ABI::Windows::UI::Input::IManipulationUpdatedEventArgs \
                                   IManipulationUpdatedEventArgs;
  typedef ABI::Windows::UI::Input::IPointerPoint IPointerPoint;
  typedef ABI::Windows::UI::Input::IRightTappedEventArgs IRightTappedEventArgs;
  typedef ABI::Windows::UI::Input::ITappedEventArgs ITappedEventArgs;
  typedef ABI::Windows::UI::Input::ManipulationDelta ManipulationDelta;

  typedef mozilla::layers::ScrollableLayerGuid ScrollableLayerGuid;

public:
  MetroInput(MetroWidget* aWidget,
             ICoreWindow* aWindow);
  virtual ~MetroInput();

  
  
  
  
  
  HRESULT OnPointerPressed(ICoreWindow* aSender,
                           IPointerEventArgs* aArgs);
  HRESULT OnPointerReleased(ICoreWindow* aSender,
                            IPointerEventArgs* aArgs);
  HRESULT OnPointerMoved(ICoreWindow* aSender,
                         IPointerEventArgs* aArgs);
  HRESULT OnPointerEntered(ICoreWindow* aSender,
                           IPointerEventArgs* aArgs);
  HRESULT OnPointerExited(ICoreWindow* aSender,
                          IPointerEventArgs* aArgs);

  
  
  HRESULT OnEdgeGestureStarted(IEdgeGesture* aSender,
                               IEdgeGestureEventArgs* aArgs);
  HRESULT OnEdgeGestureCanceled(IEdgeGesture* aSender,
                                IEdgeGestureEventArgs* aArgs);
  HRESULT OnEdgeGestureCompleted(IEdgeGesture* aSender,
                                 IEdgeGestureEventArgs* aArgs);

  
  HRESULT OnManipulationCompleted(IGestureRecognizer* aSender,
                                  IManipulationCompletedEventArgs* aArgs);

  
  HRESULT OnTapped(IGestureRecognizer* aSender, ITappedEventArgs* aArgs);
  HRESULT OnRightTapped(IGestureRecognizer* aSender,
                        IRightTappedEventArgs* aArgs);

  void HandleTap(const Point& aPoint, unsigned int aTapCount);
  void HandleLongTap(const Point& aPoint);

  
  void FlushPendingContentResponse();

  static bool IsInputModeImprecise();

private:
  Microsoft::WRL::ComPtr<ICoreWindow> mWindow;
  Microsoft::WRL::ComPtr<MetroWidget> mWidget;
  Microsoft::WRL::ComPtr<IGestureRecognizer> mGestureRecognizer;

  ModifierKeyState mModifierKeyState;

  
  enum InputPrecisionLevel {
    LEVEL_PRECISE,
    LEVEL_IMPRECISE
  };
  static InputPrecisionLevel sCurrentInputLevel;
  void UpdateInputLevel(InputPrecisionLevel aInputLevel);

  
  void RegisterInputEvents();
  void UnregisterInputEvents();

  
  bool mNonApzTargetForTouch;
  bool HitTestChrome(const LayoutDeviceIntPoint& pt);

  
  bool TransformRefPoint(const Point& aPosition,
                         LayoutDeviceIntPoint& aRefPointOut);
  void TransformTouchEvent(WidgetTouchEvent* aEvent);
  void OnPointerNonTouch(IPointerPoint* aPoint);
  void AddPointerMoveDataToRecognizer(IPointerEventArgs* aArgs);
  void InitGeckoMouseEventFromPointerPoint(WidgetMouseEvent* aEvent,
                                           IPointerPoint* aPoint);
  void ProcessManipulationDelta(ManipulationDelta const& aDelta,
                                Point const& aPosition,
                                uint32_t aMagEventType,
                                uint32_t aRotEventType);
  uint16_t ProcessInputTypeForGesture(IEdgeGestureEventArgs* aArgs);
  bool ShouldDeliverInputToRecognizer();

  
  
  void GetAllowedTouchBehavior(WidgetTouchEvent* aTransformedEvent, nsTArray<TouchBehaviorFlags>& aOutBehaviors);

  
  
  
  
  
  
  
  
  
  
  
  bool mCancelable;
  bool mRecognizerWantsEvents;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  void InitTouchEventTouchList(WidgetTouchEvent* aEvent);
  nsBaseHashtable<nsUint32HashKey,
                  nsRefPtr<mozilla::dom::Touch>,
                  nsRefPtr<mozilla::dom::Touch> > mTouches;

  
  
  
  
  EventRegistrationToken mTokenPointerPressed;
  EventRegistrationToken mTokenPointerReleased;
  EventRegistrationToken mTokenPointerMoved;
  EventRegistrationToken mTokenPointerEntered;
  EventRegistrationToken mTokenPointerExited;

  
  
  EventRegistrationToken mTokenEdgeStarted;
  EventRegistrationToken mTokenEdgeCanceled;
  EventRegistrationToken mTokenEdgeCompleted;

  
  
  
  
  EventRegistrationToken mTokenManipulationCompleted;
  EventRegistrationToken mTokenTapped;
  EventRegistrationToken mTokenRightTapped;

  
  
  
  
  
  
  

  
  void DispatchAsyncEventIgnoreStatus(WidgetInputEvent* aEvent);
  void DispatchAsyncTouchEvent(WidgetTouchEvent* aEvent);

  
  void DeliverNextQueuedEventIgnoreStatus();
  void DeliverNextQueuedTouchEvent();

  void HandleTouchStartEvent(WidgetTouchEvent* aEvent);
  void HandleFirstTouchMoveEvent(WidgetTouchEvent* aEvent);
  void SendPointerCancelToContent(const WidgetTouchEvent& aEvent);
  bool SendPendingResponseToApz();
  void CancelGesture();

  
  void DispatchEventIgnoreStatus(WidgetGUIEvent* aEvent);

  nsDeque mInputEventQueue;
  mozilla::layers::ScrollableLayerGuid mTargetAPZCGuid;
  uint64_t mInputBlockId;
  static nsEventStatus sThrowawayStatus;
};

} } }
