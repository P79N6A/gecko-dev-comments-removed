




#ifndef nsWindowBase_h_
#define nsWindowBase_h_

#include "nsBaseWidget.h"
#include "nsGUIEvent.h"
#include "npapi.h"
#include <windows.h>






class nsWindowBase : public nsBaseWidget
{
public:
  


  virtual HWND GetWindowHandle() MOZ_FINAL {
    return static_cast<HWND>(GetNativeData(NS_NATIVE_WINDOW));
  }

  


  virtual void InitEvent(nsGUIEvent& aEvent, nsIntPoint* aPoint = nullptr) = 0;

  



  virtual bool DispatchWindowEvent(nsGUIEvent* aEvent) = 0;

  


  virtual bool DispatchPluginEvent(const MSG &aMsg) MOZ_FINAL
  {
    if (!PluginHasFocus()) {
      return false;
    }
    nsPluginEvent pluginEvent(true, NS_PLUGIN_INPUT_EVENT, this);
    nsIntPoint point(0, 0);
    InitEvent(pluginEvent, &point);
    NPEvent npEvent;
    npEvent.event = aMsg.message;
    npEvent.wParam = aMsg.wParam;
    npEvent.lParam = aMsg.lParam;
    pluginEvent.pluginEvent = (void *)&npEvent;
    pluginEvent.retargetToFocusedDocument = true;
    return DispatchWindowEvent(&pluginEvent);
  }

  


  virtual bool PluginHasFocus() const MOZ_FINAL
  {
    return (mInputContext.mIMEState.mEnabled == IMEState::PLUGIN);
  }

protected:
  InputContext mInputContext;
};

#endif 
