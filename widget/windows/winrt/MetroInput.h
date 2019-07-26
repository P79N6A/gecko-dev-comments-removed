




#pragma once


#include "keyboardlayout.h"   
#include "nsBaseHashtable.h"  
#include "nsHashKeys.h"       
#include "mozwrlbase.h"
#include "nsDeque.h"
#include "mozilla/EventForwards.h"


#include <EventToken.h>     
#include <stdint.h>         
#include <wrl\client.h>     
#include <wrl\implements.h> 


class MetroWidget;
struct nsIntPoint;

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

class MetroInput : public Microsoft::WRL::RuntimeClass<IInspectable>
{
  InspectableClass(L"MetroInput", BaseTrust);

private:
  
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

  
  
  
  
  
  HRESULT OnManipulationStarted(IGestureRecognizer* aSender,
                                IManipulationStartedEventArgs* aArgs);
  HRESULT OnManipulationUpdated(IGestureRecognizer* aSender,
                                IManipulationUpdatedEventArgs* aArgs);
  HRESULT OnManipulationCompleted(IGestureRecognizer* aSender,
                                  IManipulationCompletedEventArgs* aArgs);
  HRESULT OnTapped(IGestureRecognizer* aSender, ITappedEventArgs* aArgs);
  HRESULT OnRightTapped(IGestureRecognizer* aSender,
                        IRightTappedEventArgs* aArgs);

  void HandleSingleTap(const Point& aPoint);
  void HandleLongTap(const Point& aPoint);

private:
  Microsoft::WRL::ComPtr<ICoreWindow> mWindow;
  Microsoft::WRL::ComPtr<MetroWidget> mWidget;
  Microsoft::WRL::ComPtr<IGestureRecognizer> mGestureRecognizer;

  ModifierKeyState mModifierKeyState;

  
  void RegisterInputEvents();
  void UnregisterInputEvents();

  
  bool mChromeHitTestCacheForTouch;
  bool HitTestChrome(const LayoutDeviceIntPoint& pt);

  
  void TransformRefPoint(const Point& aPosition,
                         LayoutDeviceIntPoint& aRefPointOut);
  void OnPointerNonTouch(IPointerPoint* aPoint);
  void AddPointerMoveDataToRecognizer(IPointerEventArgs* aArgs);
  void InitGeckoMouseEventFromPointerPoint(nsMouseEvent* aEvent,
                                           IPointerPoint* aPoint);
  void ProcessManipulationDelta(ManipulationDelta const& aDelta,
                                Point const& aPosition,
                                uint32_t aMagEventType,
                                uint32_t aRotEventType);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  bool mTouchStartDefaultPrevented;
  bool mTouchMoveDefaultPrevented;
  bool mIsFirstTouchMove;
  bool mCancelable;
  bool mTouchCancelSent;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  void InitTouchEventTouchList(nsTouchEvent* aEvent);
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

  
  
  
  
  EventRegistrationToken mTokenManipulationStarted;
  EventRegistrationToken mTokenManipulationUpdated;
  EventRegistrationToken mTokenManipulationCompleted;
  EventRegistrationToken mTokenTapped;
  EventRegistrationToken mTokenRightTapped;

  
  
  
  
  
  
  

  
  void DispatchAsyncEventIgnoreStatus(nsInputEvent* aEvent);
  void DispatchAsyncTouchEventIgnoreStatus(nsTouchEvent* aEvent);
  void DispatchAsyncTouchEventWithCallback(nsTouchEvent* aEvent, void (MetroInput::*Callback)());

  
  void DeliverNextQueuedEventIgnoreStatus();
  nsEventStatus DeliverNextQueuedTouchEvent();

  
  void OnPointerPressedCallback();
  void OnFirstPointerMoveCallback();

  
  void DispatchEventIgnoreStatus(nsGUIEvent *aEvent);
  void DispatchTouchCancel();

  nsDeque mInputEventQueue;
  static nsEventStatus sThrowawayStatus;
};

} } }
