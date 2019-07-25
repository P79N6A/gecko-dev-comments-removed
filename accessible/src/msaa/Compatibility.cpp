






































#include "Compatibility.h"

#include "nsWinUtils.h"
#include "Statistics.h"

#include "mozilla/Preferences.h"

using namespace mozilla;
using namespace mozilla::a11y;




bool
IsModuleVersionLessThan(HMODULE aModuleHandle, DWORD aMajor, DWORD aMinor)
{
  PRUnichar fileName[MAX_PATH];
  ::GetModuleFileNameW(aModuleHandle, fileName, MAX_PATH);

  DWORD dummy = 0;
  DWORD length = ::GetFileVersionInfoSizeW(fileName, &dummy);

  LPBYTE versionInfo = new BYTE[length];
  ::GetFileVersionInfoW(fileName, 0, length, versionInfo);

  UINT uLen;
  VS_FIXEDFILEINFO* fixedFileInfo = NULL;
  ::VerQueryValueW(versionInfo, L"\\", (LPVOID*)&fixedFileInfo, &uLen);
  DWORD dwFileVersionMS = fixedFileInfo->dwFileVersionMS;
  DWORD dwFileVersionLS = fixedFileInfo->dwFileVersionLS;
  delete [] versionInfo;

  DWORD dwLeftMost = HIWORD(dwFileVersionMS);
  DWORD dwSecondRight = HIWORD(dwFileVersionLS);
  return (dwLeftMost < aMajor ||
    (dwLeftMost == aMajor && dwSecondRight < aMinor));
}






PRUint32 Compatibility::sMode = Compatibility::NoCompatibilityMode;

void
Compatibility::Init()
{
  

  HMODULE jawsHandle = ::GetModuleHandleW(L"jhook");
  if (jawsHandle) {
    sMode |= JAWSMode;
    
    if (IsModuleVersionLessThan(jawsHandle, 8, 2173)) {
      sMode |= IA2OffMode;
      statistics::A11yConsumers(OLDJAWS);
    } else {
      statistics::A11yConsumers(JAWS);
    }
  }

  if (::GetModuleHandleW(L"gwm32inc")) {
    sMode |= WEMode;
    statistics::A11yConsumers(WE);
  }
  if (::GetModuleHandleW(L"dolwinhk")) {
    sMode |= DolphinMode;
    statistics::A11yConsumers(DOLPHIN);
  }

  if (::GetModuleHandleW(L"STSA32"))
    statistics::A11yConsumers(SEROTEK);

  if (::GetModuleHandleW(L"nvdaHelperRemote"))
    statistics::A11yConsumers(NVDA);

  if (::GetModuleHandleW(L"OsmHooks"))
    statistics::A11yConsumers(COBRA);

  
  if (sMode & JAWSMode || sMode & WEMode) {
    
    
    if (!Preferences::HasUserValue("browser.ctrlTab.disallowForScreenReaders"))
      Preferences::SetBool("browser.ctrlTab.disallowForScreenReaders", true);
  }
}

