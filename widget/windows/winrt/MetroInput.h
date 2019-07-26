




#pragma once


#include "keyboardlayout.h"   
#include "nsBaseHashtable.h"  
#include "nsGUIEvent.h"       
#include "nsHashKeys.h"       
#include "mozwrlbase.h"


#include <EventToken.h>     
#include <stdint.h>         
#include <wrl\client.h>     
#include <wrl\implements.h> 


class MetroWidget;
class nsIDOMTouch;
enum nsEventStatus;
class nsGUIEvent;
struct nsIntPoint;


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
        struct CorePhysicalKeyStatus;
        struct ICoreWindow;
        struct ICoreDispatcher;
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

  
  typedef ABI::Windows::UI::Core::CorePhysicalKeyStatus CorePhysicalKeyStatus;
  typedef ABI::Windows::UI::Core::ICoreWindow ICoreWindow;
  typedef ABI::Windows::UI::Core::IAcceleratorKeyEventArgs \
                                  IAcceleratorKeyEventArgs;
  typedef ABI::Windows::UI::Core::ICoreDispatcher ICoreDispatcher;
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
             ICoreWindow* aWindow,
             ICoreDispatcher* aDispatcher);
  virtual ~MetroInput();

  
  
  
  HRESULT OnAcceleratorKeyActivated(ICoreDispatcher* aSender,
                                    IAcceleratorKeyEventArgs* aArgs);

  
  
  
  
  
  HRESULT OnPointerWheelChanged(ICoreWindow* aSender,
                                IPointerEventArgs* aArgs);
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

private:
  Microsoft::WRL::ComPtr<ICoreWindow> mWindow;
  Microsoft::WRL::ComPtr<MetroWidget> mWidget;
  Microsoft::WRL::ComPtr<ICoreDispatcher> mDispatcher;
  Microsoft::WRL::ComPtr<IGestureRecognizer> mGestureRecognizer;

  ModifierKeyState mModifierKeyState;

  
  void RegisterInputEvents();
  void UnregisterInputEvents();

  
  void OnKeyDown(uint32_t aVKey,
                 CorePhysicalKeyStatus const& aKeyStatus);
  void OnKeyUp(uint32_t aVKey,
               CorePhysicalKeyStatus const& aKeyStatus);
  void OnCharacterReceived(uint32_t aVKey,
                           CorePhysicalKeyStatus const& aKeyStatus);
  void OnPointerNonTouch(IPointerPoint* aPoint);
  void InitGeckoMouseEventFromPointerPoint(nsMouseEvent& aEvent,
                                           IPointerPoint* aPoint);
  void ProcessManipulationDelta(ManipulationDelta const& aDelta,
                                Point const& aPosition,
                                uint32_t aMagEventType,
                                uint32_t aRotEventType);

  void DispatchEventIgnoreStatus(nsGUIEvent *aEvent);
  static nsEventStatus sThrowawayStatus;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  bool mTouchStartDefaultPrevented;
  bool mTouchMoveDefaultPrevented;
  bool mIsFirstTouchMove;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  nsEventStatus mTouchEventStatus;
  nsTouchEvent mTouchEvent;
  void DispatchPendingTouchEvent();
  void DispatchPendingTouchEvent(nsEventStatus& status);
  nsBaseHashtable<nsUint32HashKey,
                  nsCOMPtr<nsIDOMTouch>,
                  nsCOMPtr<nsIDOMTouch> > mTouches;

  
  
  
  
  
  
  static uint32_t sVirtualKeyMap[255];
  static bool sIsVirtualKeyMapInitialized;
  static void InitializeVirtualKeyMap();
  static uint32_t GetMozKeyCode(uint32_t aKey);

  
  
  
  
  EventRegistrationToken mTokenPointerPressed;
  EventRegistrationToken mTokenPointerReleased;
  EventRegistrationToken mTokenPointerMoved;
  EventRegistrationToken mTokenPointerEntered;
  EventRegistrationToken mTokenPointerExited;
  EventRegistrationToken mTokenPointerWheelChanged;

  
  
  
  
  EventRegistrationToken mTokenAcceleratorKeyActivated;

  
  
  EventRegistrationToken mTokenEdgeGesture;

  
  
  
  
  EventRegistrationToken mTokenManipulationStarted;
  EventRegistrationToken mTokenManipulationUpdated;
  EventRegistrationToken mTokenManipulationCompleted;
  EventRegistrationToken mTokenTapped;
  EventRegistrationToken mTokenRightTapped;
};

} } }
