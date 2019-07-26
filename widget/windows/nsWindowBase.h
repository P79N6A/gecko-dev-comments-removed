




#ifndef nsWindowBase_h_
#define nsWindowBase_h_

#include "nsBaseWidget.h"






class nsWindowBase : public nsBaseWidget
{
public:
  


  virtual HWND GetWindowHandle() MOZ_FINAL {
    return static_cast<HWND>(GetNativeData(NS_NATIVE_WINDOW));
  }

  


  virtual void InitEvent(nsGUIEvent& aEvent, nsIntPoint* aPoint = nullptr) = 0;

  


  virtual bool DispatchWindowEvent(nsGUIEvent* aEvent) = 0;
};

#endif 
