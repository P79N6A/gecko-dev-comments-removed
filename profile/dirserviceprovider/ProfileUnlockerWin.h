





#ifndef mozilla_ProfileUnlockerWin_h
#define mozilla_ProfileUnlockerWin_h

#include <windows.h>
#include <RestartManager.h>

#include "nsIProfileUnlocker.h"
#include "nsProfileStringTypes.h"
#include "nsWindowsHelpers.h"

namespace mozilla {

class ProfileUnlockerWin MOZ_FINAL : public nsIProfileUnlocker
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPROFILEUNLOCKER

  explicit ProfileUnlockerWin(const nsAString& aFileName);

  nsresult Init();

  DWORD StartSession(DWORD& aHandle);
  void EndSession(DWORD aHandle);

private:
  ~ProfileUnlockerWin();
  nsresult TryToTerminate(RM_UNIQUE_PROCESS& aProcess);

private:
  typedef DWORD (WINAPI *RMSTARTSESSION)(DWORD*, DWORD, WCHAR[]);
  typedef DWORD (WINAPI *RMREGISTERRESOURCES)(DWORD, UINT, LPCWSTR[], UINT,
                                              RM_UNIQUE_PROCESS[], UINT,
                                              LPCWSTR[]);
  typedef DWORD (WINAPI *RMGETLIST)(DWORD, UINT*, UINT*, RM_PROCESS_INFO[],
                                    LPDWORD);
  typedef DWORD (WINAPI *RMENDSESSION)(DWORD);
  typedef BOOL (WINAPI *QUERYFULLPROCESSIMAGENAME)(HANDLE, DWORD, LPWSTR, PDWORD);

private:
  nsModuleHandle            mRestartMgrModule;
  RMSTARTSESSION            mRmStartSession;
  RMREGISTERRESOURCES       mRmRegisterResources;
  RMGETLIST                 mRmGetList;
  RMENDSESSION              mRmEndSession;
  QUERYFULLPROCESSIMAGENAME mQueryFullProcessImageName;

  nsString                  mFileName;
};

} 

#endif 

