




#ifndef nsWindowBase_h_
#define nsWindowBase_h_

#include "mozilla/EventForwards.h"
#include "nsBaseWidget.h"
#include "nsClassHashtable.h"

#include <windows.h>
#include "touchinjection_sdk80.h"





class nsWindowBase : public nsBaseWidget
{
public:
  


  virtual HWND GetWindowHandle() final {
    return static_cast<HWND>(GetNativeData(NS_NATIVE_WINDOW));
  }

  


  virtual nsWindowBase* GetParentWindowBase(bool aIncludeOwner) = 0;

  


  virtual bool IsTopLevelWidget() = 0;

  




  virtual void InitEvent(mozilla::WidgetGUIEvent& aEvent,
                         nsIntPoint* aPoint = nullptr) = 0;

  



  virtual bool DispatchWindowEvent(mozilla::WidgetGUIEvent* aEvent) = 0;

  




  virtual bool DispatchKeyboardEvent(mozilla::WidgetKeyboardEvent* aEvent) = 0;

  




  virtual bool DispatchWheelEvent(mozilla::WidgetWheelEvent* aEvent) = 0;

  




  virtual bool DispatchContentCommandEvent(mozilla::WidgetContentCommandEvent* aEvent) = 0;

  


  virtual bool DispatchPluginEvent(const MSG& aMsg);

  


  virtual bool PluginHasFocus() const final
  {
    return (mInputContext.mIMEState.mEnabled == IMEState::PLUGIN);
  }

  


  virtual nsresult SynthesizeNativeTouchPoint(uint32_t aPointerId,
                                              TouchPointerState aPointerState,
                                              nsIntPoint aPointerScreenPoint,
                                              double aPointerPressure,
                                              uint32_t aPointerOrientation,
                                              nsIObserver* aObserver) override;
  virtual nsresult ClearNativeTouchSequence(nsIObserver* aObserver) override;

  



  virtual bool HandleAppCommandMsg(const MSG& aAppCommandMsg,
                                   LRESULT *aRetValue);

protected:
  static bool InitTouchInjection();
  bool InjectTouchPoint(uint32_t aId, nsIntPoint& aPointerScreenPoint,
                        POINTER_FLAGS aFlags, uint32_t aPressure = 1024,
                        uint32_t aOrientation = 90);

  class PointerInfo
  {
  public:
    PointerInfo(int32_t aPointerId, nsIntPoint& aPoint) :
      mPointerId(aPointerId),
      mPosition(aPoint)
    {
    }

    int32_t mPointerId;
    nsIntPoint mPosition;
  };

  static PLDHashOperator CancelTouchPoints(const unsigned int& aPointerId, nsAutoPtr<PointerInfo>& aInfo, void* aUserArg);

  nsClassHashtable<nsUint32HashKey, PointerInfo> mActivePointers;
  static bool sTouchInjectInitialized;
  static InjectTouchInputPtr sInjectTouchFuncPtr;

protected:
  InputContext mInputContext;
};

#endif 
