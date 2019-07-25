





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






uint32_t Compatibility::sConsumers = Compatibility::UNKNOWN;

void
Compatibility::Init()
{
  

  HMODULE jawsHandle = ::GetModuleHandleW(L"jhook");
  if (jawsHandle)
    sConsumers |= (IsModuleVersionLessThan(jawsHandle, 8, 2173)) ?
                   OLDJAWS : JAWS;

  if (::GetModuleHandleW(L"gwm32inc"))
    sConsumers |= WE;

  if (::GetModuleHandleW(L"dolwinhk"))
    sConsumers |= DOLPHIN;

  if (::GetModuleHandleW(L"STSA32"))
    sConsumers |= SEROTEK;

  if (::GetModuleHandleW(L"nvdaHelperRemote"))
    sConsumers |= NVDA;

  if (::GetModuleHandleW(L"OsmHooks"))
    sConsumers |= COBRA;

  if (::GetModuleHandleW(L"WebFinderRemote"))
    sConsumers |= ZOOMTEXT;

  if (::GetModuleHandleW(L"Kazahook"))
    sConsumers |= KAZAGURU;

  if (::GetModuleHandleW(L"TextExtractorImpl32") ||
      ::GetModuleHandleW(L"TextExtractorImpl64"))
    sConsumers |= YOUDAO;

  if (::GetModuleHandleW(L"uiautomation") ||
      ::GetModuleHandleW(L"uiautomationcore"))
    sConsumers |= UIAUTOMATION;

  
  if (sConsumers != Compatibility::UNKNOWN)
    sConsumers ^= Compatibility::UNKNOWN;

  
  uint32_t temp = sConsumers;
  for (int i = 0; temp; i++) {
    if (temp & 0x1)
      statistics::A11yConsumers(i);

    temp >>= 1;
  }

  
  if (sConsumers & (JAWS | OLDJAWS | WE)) {
    
    
    if (!Preferences::HasUserValue("browser.ctrlTab.disallowForScreenReaders"))
      Preferences::SetBool("browser.ctrlTab.disallowForScreenReaders", true);
  }
}

