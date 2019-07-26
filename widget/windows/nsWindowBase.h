




#ifndef nsWindowBase_h_
#define nsWindowBase_h_

#include "mozilla/EventForwards.h"
#include "nsBaseWidget.h"
#include <windows.h>





class nsWindowBase : public nsBaseWidget
{
public:
  


  virtual HWND GetWindowHandle() MOZ_FINAL {
    return static_cast<HWND>(GetNativeData(NS_NATIVE_WINDOW));
  }

  


  virtual nsWindowBase* GetParentWindowBase(bool aIncludeOwner) = 0;

  


  virtual bool IsTopLevelWidget() = 0;

  




  virtual void InitEvent(mozilla::WidgetGUIEvent& aEvent,
                         nsIntPoint* aPoint = nullptr) = 0;

  



  virtual bool DispatchWindowEvent(mozilla::WidgetGUIEvent* aEvent) = 0;

  




  virtual bool DispatchKeyboardEvent(mozilla::WidgetGUIEvent* aEvent) = 0;

  


  virtual bool DispatchPluginEvent(const MSG& aMsg);

  


  virtual bool PluginHasFocus() const MOZ_FINAL
  {
    return (mInputContext.mIMEState.mEnabled == IMEState::PLUGIN);
  }

protected:
  InputContext mInputContext;
};

#endif 
