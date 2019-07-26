




#include "imgIContainer.h"
#include "imgIRequest.h"
#include "nsIDOMElement.h"
#include "nsIDOMHTMLImageElement.h"
#include "nsIImageLoadingContent.h"
#include "nsIPrefService.h"
#include "nsIPrefLocalizedString.h"
#include "nsIServiceManager.h"
#include "nsIStringBundle.h"
#include "nsNetUtil.h"
#include "nsShellService.h"
#include "nsWindowsShellService.h"
#include "nsIProcess.h"
#include "nsICategoryManager.h"
#include "nsBrowserCompsCID.h"
#include "nsDirectoryServiceUtils.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsDirectoryServiceDefs.h"
#include "nsIWindowsRegKey.h"
#include "nsUnicharUtils.h"
#include "nsIWinTaskbar.h"
#include "nsISupportsPrimitives.h"
#include "nsThreadUtils.h"
#include "nsXULAppAPI.h"

#include "windows.h"
#include "shellapi.h"

#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#define _WIN32_WINNT 0x0600
#define INITGUID
#include <shlobj.h>

#include <mbstring.h>
#include <shlwapi.h>

#ifndef MAX_BUF
#define MAX_BUF 4096
#endif

#define REG_SUCCEEDED(val) \
  (val == ERROR_SUCCESS)

#define REG_FAILED(val) \
  (val != ERROR_SUCCESS)

#define NS_TASKBAR_CONTRACTID "@mozilla.org/windows-taskbar;1"

NS_IMPL_ISUPPORTS2(nsWindowsShellService, nsIWindowsShellService, nsIShellService)

static nsresult
OpenKeyForReading(HKEY aKeyRoot, const nsAString& aKeyName, HKEY* aKey)
{
  const nsString &flatName = PromiseFlatString(aKeyName);

  DWORD res = ::RegOpenKeyExW(aKeyRoot, flatName.get(), 0, KEY_READ, aKey);
  switch (res) {
  case ERROR_SUCCESS:
    break;
  case ERROR_ACCESS_DENIED:
    return NS_ERROR_FILE_ACCESS_DENIED;
  case ERROR_FILE_NOT_FOUND:
    return NS_ERROR_NOT_AVAILABLE;
  }

  return NS_OK;
}






























































typedef struct {
  const char* keyName;
  const char* valueData;
  const char* oldValueData;
} SETTING;

#define APP_REG_NAME L"Firefox"
#define VAL_FILE_ICON "%APPPATH%,1"
#define VAL_OPEN "\"%APPPATH%\" -osint -url \"%1\""
#define OLD_VAL_OPEN "\"%APPPATH%\" -requestPending -osint -url \"%1\""
#define DI "\\DefaultIcon"
#define SOC "\\shell\\open\\command"
#define SOD "\\shell\\open\\ddeexec"

#define FTP_SOC L"Software\\Classes\\ftp\\shell\\open\\command"

#define MAKE_KEY_NAME1(PREFIX, MID) \
  PREFIX MID








static SETTING gSettings[] = {
  
  { MAKE_KEY_NAME1("FirefoxHTML", SOC), VAL_OPEN, OLD_VAL_OPEN },

  
  { MAKE_KEY_NAME1("FirefoxURL", SOC), VAL_OPEN, OLD_VAL_OPEN },

  
  { MAKE_KEY_NAME1("HTTP", DI), VAL_FILE_ICON },
  { MAKE_KEY_NAME1("HTTP", SOC), VAL_OPEN, OLD_VAL_OPEN },
  { MAKE_KEY_NAME1("HTTPS", DI), VAL_FILE_ICON },
  { MAKE_KEY_NAME1("HTTPS", SOC), VAL_OPEN, OLD_VAL_OPEN }
};




static SETTING gDDESettings[] = {
  
  { MAKE_KEY_NAME1("Software\\Classes\\FirefoxHTML", SOD) },

  
  { MAKE_KEY_NAME1("Software\\Classes\\FirefoxURL", SOD) },

  
  { MAKE_KEY_NAME1("Software\\Classes\\FTP", SOD) },
  { MAKE_KEY_NAME1("Software\\Classes\\HTTP", SOD) },
  { MAKE_KEY_NAME1("Software\\Classes\\HTTPS", SOD) }
};

nsresult
GetHelperPath(nsAutoString& aPath)
{
  nsresult rv;
  nsCOMPtr<nsIProperties> directoryService = 
    do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIFile> appHelper;
  rv = directoryService->Get(XRE_EXECUTABLE_FILE,
                             NS_GET_IID(nsIFile),
                             getter_AddRefs(appHelper));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = appHelper->SetNativeLeafName(NS_LITERAL_CSTRING("uninstall"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = appHelper->AppendNative(NS_LITERAL_CSTRING("helper.exe"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = appHelper->GetPath(aPath);

  aPath.Insert(L'"', 0);
  aPath.Append(L'"');
  return rv;
}

nsresult
LaunchHelper(nsAutoString& aPath)
{
  STARTUPINFOW si = {sizeof(si), 0};
  PROCESS_INFORMATION pi = {0};

  if (!CreateProcessW(nullptr, (LPWSTR)aPath.get(), nullptr, nullptr, FALSE, 0, nullptr,
                      nullptr, &si, &pi)) {
    return NS_ERROR_FAILURE;
  }

  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
  return NS_OK;
}

NS_IMETHODIMP
nsWindowsShellService::ShortcutMaintenance()
{
  nsresult rv;

  
  

  
  
  
  
  
  

  nsCOMPtr<nsIWinTaskbar> taskbarInfo =
    do_GetService(NS_TASKBAR_CONTRACTID);
  if (!taskbarInfo) 
    return NS_OK;

  
  bool isSupported = false;
  taskbarInfo->GetAvailable(&isSupported);
  if (!isSupported)
    return NS_OK;

  nsAutoString appId;
  if (NS_FAILED(taskbarInfo->GetDefaultGroupId(appId)))
    return NS_ERROR_UNEXPECTED;

  NS_NAMED_LITERAL_CSTRING(prefName, "browser.taskbar.lastgroupid");
  nsCOMPtr<nsIPrefService> prefs =
    do_GetService(NS_PREFSERVICE_CONTRACTID);
  if (!prefs)
    return NS_ERROR_UNEXPECTED;

  nsCOMPtr<nsIPrefBranch> prefBranch;
  prefs->GetBranch(nullptr, getter_AddRefs(prefBranch));
  if (!prefBranch)
    return NS_ERROR_UNEXPECTED;

  nsCOMPtr<nsISupportsString> prefString;
  rv = prefBranch->GetComplexValue(prefName.get(),
                                   NS_GET_IID(nsISupportsString),
                                   getter_AddRefs(prefString));
  if (NS_SUCCEEDED(rv)) {
    nsAutoString version;
    prefString->GetData(version);
    if (!version.IsEmpty() && version.Equals(appId)) {
      
      return NS_OK;
    }
  }
  
  prefString =
    do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID, &rv);
  if (NS_FAILED(rv))
    return rv;

  prefString->SetData(appId);
  rv = prefBranch->SetComplexValue(prefName.get(),
                                   NS_GET_IID(nsISupportsString),
                                   prefString);
  if (NS_FAILED(rv)) {
    NS_WARNING("Couldn't set last user model id!");
    return NS_ERROR_UNEXPECTED;
  }

  nsAutoString appHelperPath;
  if (NS_FAILED(GetHelperPath(appHelperPath)))
    return NS_ERROR_UNEXPECTED;

  appHelperPath.AppendLiteral(" /UpdateShortcutAppUserModelIds");

  return LaunchHelper(appHelperPath);
}

static bool
IsWin8OrLater()
{
  OSVERSIONINFOW osInfo;
  osInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOW);
  GetVersionExW(&osInfo);
  return osInfo.dwMajorVersion > 6 || 
         (osInfo.dwMajorVersion >= 6 && osInfo.dwMinorVersion >= 2);
}

static bool
IsAARDefaultHTTP(IApplicationAssociationRegistration* pAAR,
                 bool* aIsDefaultBrowser)
{
  
  LPWSTR registeredApp;
  HRESULT hr = pAAR->QueryCurrentDefault(L"http", AT_URLPROTOCOL, AL_EFFECTIVE,
                                         &registeredApp);
  if (SUCCEEDED(hr)) {
    LPCWSTR firefoxHTTPProgID = L"FirefoxURL";
    *aIsDefaultBrowser = !wcsicmp(registeredApp, firefoxHTTPProgID);
    CoTaskMemFree(registeredApp);
  } else {
    *aIsDefaultBrowser = false;
  }
  return SUCCEEDED(hr);
}

static bool
IsAARDefaultHTML(IApplicationAssociationRegistration* pAAR,
                 bool* aIsDefaultBrowser)
{
  LPWSTR registeredApp;
  HRESULT hr = pAAR->QueryCurrentDefault(L".html", AT_FILEEXTENSION, AL_EFFECTIVE,
                                         &registeredApp);
  if (SUCCEEDED(hr)) {
    LPCWSTR firefoxHTMLProgID = L"FirefoxHTML";
    *aIsDefaultBrowser = !wcsicmp(registeredApp, firefoxHTMLProgID);
    CoTaskMemFree(registeredApp);
  } else {
    *aIsDefaultBrowser = false;
  }
  return SUCCEEDED(hr);
}

bool
nsWindowsShellService::IsDefaultBrowserVista(bool aCheckAllTypes,
                                             bool* aIsDefaultBrowser)
{
  IApplicationAssociationRegistration* pAAR;
  HRESULT hr = CoCreateInstance(CLSID_ApplicationAssociationRegistration,
                                nullptr,
                                CLSCTX_INPROC,
                                IID_IApplicationAssociationRegistration,
                                (void**)&pAAR);

  if (SUCCEEDED(hr)) {
    if (aCheckAllTypes) {
      BOOL res;
      hr = pAAR->QueryAppIsDefaultAll(AL_EFFECTIVE,
                                      APP_REG_NAME,
                                      &res);
      *aIsDefaultBrowser = res;

      
      
      if (*aIsDefaultBrowser && IsWin8OrLater()) {
        IsAARDefaultHTTP(pAAR, aIsDefaultBrowser);
        if (*aIsDefaultBrowser) {
          IsAARDefaultHTML(pAAR, aIsDefaultBrowser);
        }
      }
    } else {
      IsAARDefaultHTTP(pAAR, aIsDefaultBrowser);
    }

    pAAR->Release();
    return true;
  }
  return false;
}

NS_IMETHODIMP
nsWindowsShellService::IsDefaultBrowser(bool aStartupCheck,
                                        bool aForAllTypes,
                                        bool* aIsDefaultBrowser)
{
  
  
  
  if (aStartupCheck)
    mCheckedThisSession = true;

  
  
  if (!aForAllTypes && IsWin8OrLater()) {
    return IsDefaultBrowserVista(false,
                                 aIsDefaultBrowser) ? NS_OK : NS_ERROR_FAILURE;
  }

  
  
  *aIsDefaultBrowser = true;

  PRUnichar exePath[MAX_BUF];
  if (!::GetModuleFileNameW(0, exePath, MAX_BUF))
    return NS_ERROR_FAILURE;

  
  
  if (!::GetLongPathNameW(exePath, exePath, MAX_BUF))
    return NS_ERROR_FAILURE;

  nsAutoString appLongPath(exePath);

  HKEY theKey;
  DWORD res;
  nsresult rv;
  PRUnichar currValue[MAX_BUF];

  SETTING* settings;
  SETTING* end = gSettings + sizeof(gSettings) / sizeof(SETTING);

  for (settings = gSettings; settings < end; ++settings) {
    NS_ConvertUTF8toUTF16 keyName(settings->keyName);
    NS_ConvertUTF8toUTF16 valueData(settings->valueData);
    int32_t offset = valueData.Find("%APPPATH%");
    valueData.Replace(offset, 9, appLongPath);

    rv = OpenKeyForReading(HKEY_CLASSES_ROOT, keyName, &theKey);
    if (NS_FAILED(rv)) {
      *aIsDefaultBrowser = false;
      return NS_OK;
    }

    ::ZeroMemory(currValue, sizeof(currValue));
    DWORD len = sizeof currValue;
    res = ::RegQueryValueExW(theKey, L"", nullptr, nullptr, (LPBYTE)currValue, &len);
    
    ::RegCloseKey(theKey);
    if (REG_FAILED(res) ||
        !valueData.Equals(currValue, CaseInsensitiveCompare)) {
      
      NS_ConvertUTF8toUTF16 oldValueData(settings->oldValueData);
      offset = oldValueData.Find("%APPPATH%");
      oldValueData.Replace(offset, 9, appLongPath);
      
      if (!oldValueData.Equals(currValue, CaseInsensitiveCompare)) {
        *aIsDefaultBrowser = false;
        return NS_OK;
      }

      res = ::RegOpenKeyExW(HKEY_CLASSES_ROOT, PromiseFlatString(keyName).get(),
                            0, KEY_SET_VALUE, &theKey);
      if (REG_FAILED(res)) {
        
        
        *aIsDefaultBrowser = false;
        return NS_OK;
      }

      const nsString &flatValue = PromiseFlatString(valueData);
      res = ::RegSetValueExW(theKey, L"", 0, REG_SZ,
                             (const BYTE *) flatValue.get(),
                             (flatValue.Length() + 1) * sizeof(PRUnichar));
      
      ::RegCloseKey(theKey);
      if (REG_FAILED(res)) {
        
        
        *aIsDefaultBrowser = false;
        return NS_OK;
      }
    }
  }

  
  
  if (*aIsDefaultBrowser) {
    IsDefaultBrowserVista(true, aIsDefaultBrowser);
  }

  
  
  
  
  
  if (*aIsDefaultBrowser) {
    

    end = gDDESettings + sizeof(gDDESettings) / sizeof(SETTING);

    for (settings = gDDESettings; settings < end; ++settings) {
      NS_ConvertUTF8toUTF16 keyName(settings->keyName);

      rv = OpenKeyForReading(HKEY_CURRENT_USER, keyName, &theKey);
      if (NS_FAILED(rv)) {
        ::RegCloseKey(theKey);
        
        
        *aIsDefaultBrowser = false;
        return NS_OK;
      }

      ::ZeroMemory(currValue, sizeof(currValue));
      DWORD len = sizeof currValue;
      res = ::RegQueryValueExW(theKey, L"", nullptr, nullptr, (LPBYTE)currValue,
                               &len);
      
      ::RegCloseKey(theKey);
      if (REG_FAILED(res) || PRUnichar('\0') != *currValue) {
        
        
        const nsString &flatName = PromiseFlatString(keyName);
        ::SHDeleteKeyW(HKEY_CURRENT_USER, flatName.get());
        res = ::RegCreateKeyExW(HKEY_CURRENT_USER, flatName.get(), 0, nullptr,
                                REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, nullptr,
                                &theKey, nullptr);
        if (REG_FAILED(res)) {
          
          
          *aIsDefaultBrowser = false;
          return NS_OK;
        }

        res = ::RegSetValueExW(theKey, L"", 0, REG_SZ, (const BYTE *) L"",
                               sizeof(PRUnichar));
        
        ::RegCloseKey(theKey);
        if (REG_FAILED(res)) {
          
          
          *aIsDefaultBrowser = false;
          return NS_OK;
        }
      }
    }

    
    
    res = ::RegOpenKeyExW(HKEY_CURRENT_USER, FTP_SOC, 0, KEY_ALL_ACCESS,
                          &theKey);
    
    
    if (NS_FAILED(rv)) {
      return NS_OK;
    }

    NS_ConvertUTF8toUTF16 oldValueOpen(OLD_VAL_OPEN);
    int32_t offset = oldValueOpen.Find("%APPPATH%");
    oldValueOpen.Replace(offset, 9, appLongPath);

    ::ZeroMemory(currValue, sizeof(currValue));
    DWORD len = sizeof currValue;
    res = ::RegQueryValueExW(theKey, L"", nullptr, nullptr, (LPBYTE)currValue,
                             &len);

    
    
    if (REG_FAILED(res) ||
        !oldValueOpen.Equals(currValue, CaseInsensitiveCompare)) {
      ::RegCloseKey(theKey);
      return NS_OK;
    }

    NS_ConvertUTF8toUTF16 valueData(VAL_OPEN);
    valueData.Replace(offset, 9, appLongPath);
    const nsString &flatValue = PromiseFlatString(valueData);
    res = ::RegSetValueExW(theKey, L"", 0, REG_SZ,
                           (const BYTE *) flatValue.get(),
                           (flatValue.Length() + 1) * sizeof(PRUnichar));
    
    ::RegCloseKey(theKey);
    
    
    
    if (REG_FAILED(res)) {
      *aIsDefaultBrowser = false;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsWindowsShellService::GetCanSetDesktopBackground(bool* aResult)
{
  *aResult = true;
  return NS_OK;
}

static nsresult
DynSHOpenWithDialog(HWND hwndParent, const OPENASINFO *poainfo)
{
  typedef HRESULT (WINAPI * SHOpenWithDialogPtr)(HWND hwndParent,
                                                 const OPENASINFO *poainfo);
  static SHOpenWithDialogPtr SHOpenWithDialogFn = nullptr;
  if (!SHOpenWithDialogFn) {
    
    
    static const PRUnichar kSehllLibraryName[] =  L"shell32.dll";
    HMODULE shellDLL = ::LoadLibraryW(kSehllLibraryName);
    if (!shellDLL) {
      return NS_ERROR_FAILURE;
    }

    SHOpenWithDialogFn =
      (SHOpenWithDialogPtr)GetProcAddress(shellDLL, "SHOpenWithDialog");
    FreeLibrary(shellDLL);

    if (!SHOpenWithDialogFn) {
      return NS_ERROR_FAILURE;
    }
  }

  return SUCCEEDED(SHOpenWithDialogFn(hwndParent, poainfo)) ? NS_OK :
                                                              NS_ERROR_FAILURE;
}

nsresult
nsWindowsShellService::LaunchControlPanelDefaultPrograms()
{
  
  WCHAR controlEXEPath[MAX_PATH + 1] = { '\0' };
  if (!GetSystemDirectoryW(controlEXEPath, MAX_PATH)) {
    return NS_ERROR_FAILURE;
  }
  LPCWSTR controlEXE = L"control.exe";
  if (wcslen(controlEXEPath) + wcslen(controlEXE) >= MAX_PATH) {
    return NS_ERROR_FAILURE;
  }
  if (!PathAppendW(controlEXEPath, controlEXE)) {
    return NS_ERROR_FAILURE;
  }

  WCHAR params[] = L"control.exe /name Microsoft.DefaultPrograms /page pageDefaultProgram";
  STARTUPINFOW si = {sizeof(si), 0};
  si.dwFlags = STARTF_USESHOWWINDOW;
  si.wShowWindow = SW_SHOWDEFAULT;
  PROCESS_INFORMATION pi = {0};
  if (!CreateProcessW(controlEXEPath, params, nullptr, nullptr, FALSE, 0, nullptr,
                      nullptr, &si, &pi)) {
    return NS_ERROR_FAILURE;
  }
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);

  return NS_OK;
}

nsresult
nsWindowsShellService::LaunchHTTPHandlerPane()
{
  OPENASINFO info;
  info.pcszFile = L"http";
  info.pcszClass = nullptr;
  info.oaifInFlags = OAIF_FORCE_REGISTRATION | 
                     OAIF_URL_PROTOCOL |
                     OAIF_REGISTER_EXT;
  return DynSHOpenWithDialog(nullptr, &info);
}

NS_IMETHODIMP
nsWindowsShellService::SetDefaultBrowser(bool aClaimAllTypes, bool aForAllUsers)
{
  nsAutoString appHelperPath;
  if (NS_FAILED(GetHelperPath(appHelperPath)))
    return NS_ERROR_FAILURE;

  if (aForAllUsers) {
    appHelperPath.AppendLiteral(" /SetAsDefaultAppGlobal");
  } else {
    appHelperPath.AppendLiteral(" /SetAsDefaultAppUser");
  }

  nsresult rv = LaunchHelper(appHelperPath);
  if (NS_SUCCEEDED(rv) && IsWin8OrLater()) {
    if (aClaimAllTypes) {
      rv = LaunchControlPanelDefaultPrograms();
      
      
      if (NS_FAILED(rv)) {
        rv = LaunchHTTPHandlerPane();
      }
    } else {
      rv = LaunchHTTPHandlerPane();
      
      
      if (NS_FAILED(rv)) {
        rv = LaunchControlPanelDefaultPrograms();
      }
    }
  }

  return rv;
}

NS_IMETHODIMP
nsWindowsShellService::GetShouldCheckDefaultBrowser(bool* aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);

  
  
  if (mCheckedThisSession) {
    *aResult = false;
    return NS_OK;
  }

  nsCOMPtr<nsIPrefBranch> prefs;
  nsresult rv;
  nsCOMPtr<nsIPrefService> pserve(do_GetService(NS_PREFSERVICE_CONTRACTID, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = pserve->GetBranch("", getter_AddRefs(prefs));
  NS_ENSURE_SUCCESS(rv, rv);

  return prefs->GetBoolPref(PREF_CHECKDEFAULTBROWSER, aResult);
}

NS_IMETHODIMP
nsWindowsShellService::SetShouldCheckDefaultBrowser(bool aShouldCheck)
{
  nsCOMPtr<nsIPrefBranch> prefs;
  nsresult rv;

  nsCOMPtr<nsIPrefService> pserve(do_GetService(NS_PREFSERVICE_CONTRACTID, &rv));
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = pserve->GetBranch("", getter_AddRefs(prefs));
  NS_ENSURE_SUCCESS(rv, rv);

  return prefs->SetBoolPref(PREF_CHECKDEFAULTBROWSER, aShouldCheck);
}

static nsresult
WriteBitmap(nsIFile* aFile, imgIContainer* aImage)
{
  nsresult rv;

  nsRefPtr<gfxASurface> surface;
  aImage->GetFrame(imgIContainer::FRAME_FIRST,
                   imgIContainer::FLAG_SYNC_DECODE,
                   getter_AddRefs(surface));
  NS_ENSURE_TRUE(surface, NS_ERROR_FAILURE);

  nsRefPtr<gfxImageSurface> image(surface->GetAsReadableARGB32ImageSurface());
  NS_ENSURE_TRUE(image, NS_ERROR_FAILURE);

  int32_t width = image->Width();
  int32_t height = image->Height();

  uint8_t* bits = image->Data();
  uint32_t length = image->GetDataSize();
  uint32_t bpr = uint32_t(image->Stride());
  int32_t bitCount = bpr/width;

  
  
  BITMAPINFOHEADER bmi;
  bmi.biSize = sizeof(BITMAPINFOHEADER);
  bmi.biWidth = width;
  bmi.biHeight = height;
  bmi.biPlanes = 1;
  bmi.biBitCount = (WORD)bitCount*8;
  bmi.biCompression = BI_RGB;
  bmi.biSizeImage = length;
  bmi.biXPelsPerMeter = 0;
  bmi.biYPelsPerMeter = 0;
  bmi.biClrUsed = 0;
  bmi.biClrImportant = 0;

  BITMAPFILEHEADER bf;
  bf.bfType = 0x4D42; 
  bf.bfReserved1 = 0;
  bf.bfReserved2 = 0;
  bf.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
  bf.bfSize = bf.bfOffBits + bmi.biSizeImage;

  
  nsCOMPtr<nsIOutputStream> stream;
  rv = NS_NewLocalFileOutputStream(getter_AddRefs(stream), aFile);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = NS_ERROR_FAILURE;
  if (stream) {
    uint32_t written;
    stream->Write((const char*)&bf, sizeof(BITMAPFILEHEADER), &written);
    if (written == sizeof(BITMAPFILEHEADER)) {
      stream->Write((const char*)&bmi, sizeof(BITMAPINFOHEADER), &written);
      if (written == sizeof(BITMAPINFOHEADER)) {
        
        
        uint32_t i = length;
        do {
          i -= bpr;
          stream->Write(((const char*)bits) + i, bpr, &written);
          if (written == bpr) {
            rv = NS_OK;
          } else {
            rv = NS_ERROR_FAILURE;
            break;
          }
        } while (i != 0);
      }
    }

    stream->Close();
  }

  return rv;
}

NS_IMETHODIMP
nsWindowsShellService::SetDesktopBackground(nsIDOMElement* aElement, 
                                            int32_t aPosition)
{
  nsresult rv;

  nsCOMPtr<imgIContainer> container;
  nsCOMPtr<nsIDOMHTMLImageElement> imgElement(do_QueryInterface(aElement));
  if (!imgElement) {
    
    return NS_ERROR_NOT_AVAILABLE;
  } 
  else {
    nsCOMPtr<nsIImageLoadingContent> imageContent =
      do_QueryInterface(aElement, &rv);
    if (!imageContent)
      return rv;

    
    nsCOMPtr<imgIRequest> request;
    rv = imageContent->GetRequest(nsIImageLoadingContent::CURRENT_REQUEST,
                                  getter_AddRefs(request));
    if (!request)
      return rv;
    rv = request->GetImage(getter_AddRefs(container));
    if (!container)
      return NS_ERROR_FAILURE;
  }

  
  nsCOMPtr<nsIStringBundleService>
    bundleService(do_GetService(NS_STRINGBUNDLE_CONTRACTID, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIStringBundle> shellBundle;
  rv = bundleService->CreateBundle(SHELLSERVICE_PROPERTIES,
                                   getter_AddRefs(shellBundle));
  NS_ENSURE_SUCCESS(rv, rv);
 
  
  nsString fileLeafName;
  rv = shellBundle->GetStringFromName
                      (NS_LITERAL_STRING("desktopBackgroundLeafNameWin").get(),
                       getter_Copies(fileLeafName));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIFile> file;
  rv = NS_GetSpecialDirectory(NS_APP_APPLICATION_REGISTRY_DIR,
                              getter_AddRefs(file));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = file->Append(fileLeafName);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString path;
  rv = file->GetPath(path);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = WriteBitmap(file, container);

  
  if (NS_SUCCEEDED(rv)) {
    nsCOMPtr<nsIWindowsRegKey> regKey =
      do_CreateInstance("@mozilla.org/windows-registry-key;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = regKey->Create(nsIWindowsRegKey::ROOT_KEY_CURRENT_USER,
                        NS_LITERAL_STRING("Control Panel\\Desktop"),
                        nsIWindowsRegKey::ACCESS_SET_VALUE);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoString tile;
    nsAutoString style;
    switch (aPosition) {
      case BACKGROUND_TILE:
        style.AssignLiteral("0");
        tile.AssignLiteral("1");
        break;
      case BACKGROUND_CENTER:
        style.AssignLiteral("0");
        tile.AssignLiteral("0");
        break;
      case BACKGROUND_STRETCH:
        style.AssignLiteral("2");
        tile.AssignLiteral("0");
        break;
      case BACKGROUND_FILL:
        style.AssignLiteral("10");
        tile.AssignLiteral("0");
        break;
      case BACKGROUND_FIT:
        style.AssignLiteral("6");
        tile.AssignLiteral("0");
        break;
    }

    rv = regKey->WriteStringValue(NS_LITERAL_STRING("TileWallpaper"), tile);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = regKey->WriteStringValue(NS_LITERAL_STRING("WallpaperStyle"), style);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = regKey->Close();
    NS_ENSURE_SUCCESS(rv, rv);

    ::SystemParametersInfoW(SPI_SETDESKWALLPAPER, 0, (PVOID)path.get(),
                            SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
  }
  return rv;
}

NS_IMETHODIMP
nsWindowsShellService::OpenApplication(int32_t aApplication)
{
  nsAutoString application;
  switch (aApplication) {
  case nsIShellService::APPLICATION_MAIL:
    application.AssignLiteral("Mail");
    break;
  case nsIShellService::APPLICATION_NEWS:
    application.AssignLiteral("News");
    break;
  }

  
  
  

  
  
  
  
  

  
  HKEY theKey;
  nsresult rv = OpenKeyForReading(HKEY_CLASSES_ROOT, application, &theKey);
  if (NS_FAILED(rv))
    return rv;

  PRUnichar buf[MAX_BUF];
  DWORD type, len = sizeof buf;
  DWORD res = ::RegQueryValueExW(theKey, EmptyString().get(), 0,
                                 &type, (LPBYTE)&buf, &len);

  if (REG_FAILED(res) || !*buf)
    return NS_OK;

  
  ::RegCloseKey(theKey);

  
  application.AppendLiteral("\\");
  application.Append(buf);
  application.AppendLiteral("\\shell\\open\\command");

  rv = OpenKeyForReading(HKEY_CLASSES_ROOT, application, &theKey);
  if (NS_FAILED(rv))
    return rv;

  ::ZeroMemory(buf, sizeof(buf));
  len = sizeof buf;
  res = ::RegQueryValueExW(theKey, EmptyString().get(), 0,
                           &type, (LPBYTE)&buf, &len);
  if (REG_FAILED(res) || !*buf)
    return NS_ERROR_FAILURE;

  
  ::RegCloseKey(theKey);

  
  
  nsAutoString path(buf);
  int32_t end = path.Length();
  int32_t cursor = 0, temp = 0;
  ::ZeroMemory(buf, sizeof(buf));
  do {
    cursor = path.FindChar('%', cursor);
    if (cursor < 0) 
      break;

    temp = path.FindChar('%', cursor + 1);
    ++cursor;

    ::ZeroMemory(&buf, sizeof(buf));

    ::GetEnvironmentVariableW(nsAutoString(Substring(path, cursor, temp - cursor)).get(),
                              buf, sizeof(buf));
    
    
    
    path.Replace((cursor - 1), temp - cursor + 2, nsDependentString(buf));

    ++cursor;
  }
  while (cursor < end);

  STARTUPINFOW si;
  PROCESS_INFORMATION pi;

  ::ZeroMemory(&si, sizeof(STARTUPINFOW));
  ::ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

  BOOL success = ::CreateProcessW(nullptr, (LPWSTR)path.get(), nullptr,
                                  nullptr, FALSE, 0, nullptr,  nullptr,
                                  &si, &pi);
  if (!success)
    return NS_ERROR_FAILURE;

  return NS_OK;
}

NS_IMETHODIMP
nsWindowsShellService::GetDesktopBackgroundColor(uint32_t* aColor)
{
  uint32_t color = ::GetSysColor(COLOR_DESKTOP);
  *aColor = (GetRValue(color) << 16) | (GetGValue(color) << 8) | GetBValue(color);
  return NS_OK;
}

NS_IMETHODIMP
nsWindowsShellService::SetDesktopBackgroundColor(uint32_t aColor)
{
  int aParameters[2] = { COLOR_BACKGROUND, COLOR_DESKTOP };
  BYTE r = (aColor >> 16);
  BYTE g = (aColor << 16) >> 24;
  BYTE b = (aColor << 24) >> 24;
  COLORREF colors[2] = { RGB(r,g,b), RGB(r,g,b) };

  ::SetSysColors(sizeof(aParameters) / sizeof(int), aParameters, colors);

  nsresult rv;
  nsCOMPtr<nsIWindowsRegKey> regKey =
    do_CreateInstance("@mozilla.org/windows-registry-key;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = regKey->Create(nsIWindowsRegKey::ROOT_KEY_CURRENT_USER,
                      NS_LITERAL_STRING("Control Panel\\Colors"),
                      nsIWindowsRegKey::ACCESS_SET_VALUE);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUnichar rgb[12];
  _snwprintf(rgb, 12, L"%u %u %u", r, g, b);

  rv = regKey->WriteStringValue(NS_LITERAL_STRING("Background"),
                                nsDependentString(rgb));
  NS_ENSURE_SUCCESS(rv, rv);

  return regKey->Close();
}

nsWindowsShellService::nsWindowsShellService() : 
  mCheckedThisSession(false) 
{
}

nsWindowsShellService::~nsWindowsShellService()
{
}

NS_IMETHODIMP
nsWindowsShellService::OpenApplicationWithURI(nsIFile* aApplication,
                                              const nsACString& aURI)
{
  nsresult rv;
  nsCOMPtr<nsIProcess> process = 
    do_CreateInstance("@mozilla.org/process/util;1", &rv);
  if (NS_FAILED(rv))
    return rv;
  
  rv = process->Init(aApplication);
  if (NS_FAILED(rv))
    return rv;
  
  const nsCString spec(aURI);
  const char* specStr = spec.get();
  return process->Run(false, &specStr, 1);
}

NS_IMETHODIMP
nsWindowsShellService::GetDefaultFeedReader(nsIFile** _retval)
{
  *_retval = nullptr;

  nsresult rv;
  nsCOMPtr<nsIWindowsRegKey> regKey =
    do_CreateInstance("@mozilla.org/windows-registry-key;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = regKey->Open(nsIWindowsRegKey::ROOT_KEY_CLASSES_ROOT,
                    NS_LITERAL_STRING("feed\\shell\\open\\command"),
                    nsIWindowsRegKey::ACCESS_READ);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString path;
  rv = regKey->ReadStringValue(EmptyString(), path);
  NS_ENSURE_SUCCESS(rv, rv);
  if (path.IsEmpty())
    return NS_ERROR_FAILURE;

  if (path.First() == '"') {
    
    path = Substring(path, 1, path.FindChar('"', 1) - 1);
  }
  else {
    
    path = Substring(path, 0, path.FindChar(' '));
  }

  nsCOMPtr<nsIFile> defaultReader =
    do_CreateInstance("@mozilla.org/file/local;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = defaultReader->InitWithPath(path);
  NS_ENSURE_SUCCESS(rv, rv);

  bool exists;
  rv = defaultReader->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!exists)
    return NS_ERROR_FAILURE;

  NS_ADDREF(*_retval = defaultReader);
  return NS_OK;
}
