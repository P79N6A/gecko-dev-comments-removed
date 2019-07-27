






#ifndef __WinTaskbar_h__
#define __WinTaskbar_h__

#include <windows.h>
#include <shobjidl.h>
#undef LogSeverity // SetupAPI.h #defines this as DWORD
#include "nsIWinTaskbar.h"
#include "mozilla/Attributes.h"

namespace mozilla {
namespace widget {

class WinTaskbar MOZ_FINAL : public nsIWinTaskbar
{
  ~WinTaskbar();

public: 
  WinTaskbar();

  NS_DECL_THREADSAFE_ISUPPORTS
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

