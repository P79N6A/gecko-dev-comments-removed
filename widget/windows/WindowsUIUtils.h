




#ifndef mozilla_widget_WindowsUIUtils_h__
#define mozilla_widget_WindowsUIUtils_h__

#include "nsIWindowsUIUtils.h"

enum TabletModeState {
  eTabletModeUnknown = 0,
  eTabletModeOff,
  eTabletModeOn
};

class WindowsUIUtils final : public nsIWindowsUIUtils {
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIWINDOWSUIUTILS

  WindowsUIUtils();
protected:
  ~WindowsUIUtils();

  TabletModeState mInTabletMode;
};

#endif 
