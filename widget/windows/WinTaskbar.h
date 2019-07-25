







































#ifndef __WinTaskbar_h__
#define __WinTaskbar_h__

#if MOZ_WINSDK_TARGETVER >= MOZ_NTDDI_WIN7

#include <windows.h>
#include <shobjidl.h>
#include "nsIWinTaskbar.h"

namespace mozilla {
namespace widget {

class WinTaskbar : public nsIWinTaskbar
{
public: 
  WinTaskbar();
  ~WinTaskbar();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIWINTASKBAR

  
  
  static bool RegisterAppUserModelID();
  static bool GetAppUserModelID(nsAString & aDefaultGroupId);

private:
  bool Initialize();

  typedef HRESULT (WINAPI * SetCurrentProcessExplicitAppUserModelIDPtr)(PCWSTR AppID);
  ITaskbarList4 *mTaskbar;
};

} 
} 

#endif 

#endif 

