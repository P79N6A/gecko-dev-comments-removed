










































#include "gfxIImageFrame.h"
#include "imgIContainer.h"
#include "imgIRequest.h"
#include "nsIDOMDocument.h"
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

#include "windows.h"
#include "shellapi.h"

#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#define _WIN32_WINNT 0x0600
#define INITGUID
#include <shlobj.h>

#include <mbstring.h>

#ifndef MAX_BUF
#define MAX_BUF 4096
#endif

#define REG_SUCCEEDED(val) \
  (val == ERROR_SUCCESS)

#define REG_FAILED(val) \
  (val != ERROR_SUCCESS)

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
  char* keyName;
  char* valueName;
  char* valueData;
} SETTING;

#define APP_REG_NAME L"Firefox"
#define DI "\\DefaultIcon"
#define SOP "\\shell\\open\\command"

#define CLS_HTML "FirefoxHTML"
#define CLS_URL "FirefoxURL"
#define VAL_FILE_ICON "%APPPATH%,1"
#define VAL_OPEN "\"%APPPATH%\" -requestPending -osint -url \"%1\""

#define MAKE_KEY_NAME1(PREFIX, MID) \
  PREFIX MID





static SETTING gSettings[] = {
  
  
  
  { MAKE_KEY_NAME1(CLS_HTML, SOP), "", VAL_OPEN },

  
  { MAKE_KEY_NAME1(CLS_URL, SOP), "", VAL_OPEN },

  
  { MAKE_KEY_NAME1("HTTP", DI),    "", VAL_FILE_ICON },
  { MAKE_KEY_NAME1("HTTP", SOP),   "", VAL_OPEN },
  { MAKE_KEY_NAME1("HTTPS", DI),   "", VAL_FILE_ICON },
  { MAKE_KEY_NAME1("HTTPS", SOP),  "", VAL_OPEN }
};

PRBool
nsWindowsShellService::IsDefaultBrowserVista(PRBool* aIsDefaultBrowser)
{
#if !defined(MOZ_DISABLE_VISTA_SDK_REQUIREMENTS)
  IApplicationAssociationRegistration* pAAR;
  
  HRESULT hr = CoCreateInstance(CLSID_ApplicationAssociationRegistration,
                                NULL,
                                CLSCTX_INPROC,
                                IID_IApplicationAssociationRegistration,
                                (void**)&pAAR);

  if (SUCCEEDED(hr)) {
    hr = pAAR->QueryAppIsDefaultAll(AL_EFFECTIVE,
                                    APP_REG_NAME,
                                    aIsDefaultBrowser);

    pAAR->Release();
    return PR_TRUE;
  }
#endif  
  return PR_FALSE;
}

NS_IMETHODIMP
nsWindowsShellService::IsDefaultBrowser(PRBool aStartupCheck,
                                        PRBool* aIsDefaultBrowser)
{
  
  
  
  if (aStartupCheck)
    mCheckedThisSession = PR_TRUE;

  SETTING* settings;
  SETTING* end = gSettings + sizeof(gSettings)/sizeof(SETTING);

  *aIsDefaultBrowser = PR_TRUE;

  PRUnichar exePath[MAX_BUF];
  if (!::GetModuleFileNameW(0, exePath, MAX_BUF))
    return NS_ERROR_FAILURE;

#ifndef WINCE
  
  
  if (!::GetLongPathNameW(exePath, exePath, MAX_BUF))
    return NS_ERROR_FAILURE;
#endif

  nsAutoString appLongPath(exePath);

  nsresult rv;
  PRUnichar currValue[MAX_BUF];
  for (settings = gSettings; settings < end; ++settings) {
    NS_ConvertUTF8toUTF16 dataLongPath(settings->valueData);
    NS_ConvertUTF8toUTF16 key(settings->keyName);
    NS_ConvertUTF8toUTF16 value(settings->valueName);
    PRInt32 offset = dataLongPath.Find("%APPPATH%");
    dataLongPath.Replace(offset, 9, appLongPath);

    ::ZeroMemory(currValue, sizeof(currValue));
    HKEY theKey;
    rv = OpenKeyForReading(HKEY_CLASSES_ROOT, key, &theKey);
    if (NS_FAILED(rv)) {
      *aIsDefaultBrowser = PR_FALSE;
      return NS_OK;
    }

    DWORD len = sizeof currValue;
    DWORD res = ::RegQueryValueExW(theKey, PromiseFlatString(value).get(),
                                   NULL, NULL, (LPBYTE)currValue, &len);
    
    ::RegCloseKey(theKey);
    if (REG_FAILED(res) ||
        !dataLongPath.Equals(currValue, CaseInsensitiveCompare)) {
      
      *aIsDefaultBrowser = PR_FALSE;
      return NS_OK;
    }
  }

  
  
  if (*aIsDefaultBrowser)
    IsDefaultBrowserVista(aIsDefaultBrowser);

  return NS_OK;
}

NS_IMETHODIMP
nsWindowsShellService::SetDefaultBrowser(PRBool aClaimAllTypes, PRBool aForAllUsers)
{
  nsresult rv;
  nsCOMPtr<nsIProperties> directoryService = 
    do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsILocalFile> appHelper;
  rv = directoryService->Get(NS_XPCOM_CURRENT_PROCESS_DIR, NS_GET_IID(nsILocalFile), getter_AddRefs(appHelper));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = appHelper->AppendNative(NS_LITERAL_CSTRING("uninstall"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = appHelper->AppendNative(NS_LITERAL_CSTRING("helper.exe"));
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString appHelperPath;
  rv = appHelper->GetPath(appHelperPath);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aForAllUsers) {
    appHelperPath.AppendLiteral(" /SetAsDefaultAppGlobal");
  } else {
    appHelperPath.AppendLiteral(" /SetAsDefaultAppUser");
  }

  STARTUPINFOW si = {sizeof(si), 0};
  PROCESS_INFORMATION pi = {0};

  BOOL ok = CreateProcessW(NULL, (LPWSTR)appHelperPath.get(), NULL, NULL,
                           FALSE, 0, NULL, NULL, &si, &pi);

  if (!ok)
    return NS_ERROR_FAILURE;

  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);

  return NS_OK;
}

NS_IMETHODIMP
nsWindowsShellService::GetShouldCheckDefaultBrowser(PRBool* aResult)
{
  
  
  if (mCheckedThisSession) {
    *aResult = PR_FALSE;
    return NS_OK;
  }

  nsCOMPtr<nsIPrefBranch> prefs;
  nsCOMPtr<nsIPrefService> pserve(do_GetService(NS_PREFSERVICE_CONTRACTID));
  if (pserve)
    pserve->GetBranch("", getter_AddRefs(prefs));

  prefs->GetBoolPref(PREF_CHECKDEFAULTBROWSER, aResult);

  return NS_OK;
}

NS_IMETHODIMP
nsWindowsShellService::SetShouldCheckDefaultBrowser(PRBool aShouldCheck)
{
  nsCOMPtr<nsIPrefBranch> prefs;
  nsCOMPtr<nsIPrefService> pserve(do_GetService(NS_PREFSERVICE_CONTRACTID));
  if (pserve)
    pserve->GetBranch("", getter_AddRefs(prefs));

  prefs->SetBoolPref(PREF_CHECKDEFAULTBROWSER, aShouldCheck);

  return NS_OK;
}

static nsresult
WriteBitmap(nsIFile* aFile, gfxIImageFrame* aImage)
{
  PRInt32 width, height;
  aImage->GetWidth(&width);
  aImage->GetHeight(&height);

  PRUint8* bits;
  PRUint32 length;
  aImage->LockImageData();
  aImage->GetImageData(&bits, &length);
  if (!bits) {
      aImage->UnlockImageData();
      return NS_ERROR_FAILURE;
  }

  PRUint32 bpr;
  aImage->GetImageBytesPerRow(&bpr);
  PRInt32 bitCount = bpr/width;

  
  
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
  nsresult rv = NS_NewLocalFileOutputStream(getter_AddRefs(stream), aFile);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = NS_ERROR_FAILURE;
  if (stream) {
    PRUint32 written;
    stream->Write((const char*)&bf, sizeof(BITMAPFILEHEADER), &written);
    if (written == sizeof(BITMAPFILEHEADER)) {
      stream->Write((const char*)&bmi, sizeof(BITMAPINFOHEADER), &written);
      if (written == sizeof(BITMAPINFOHEADER)) {
        
        
        PRUint32 i = length;
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

  aImage->UnlockImageData();
  return rv;
}

NS_IMETHODIMP
nsWindowsShellService::SetDesktopBackground(nsIDOMElement* aElement, 
                                            PRInt32 aPosition)
{
  nsresult rv;

  nsCOMPtr<gfxIImageFrame> gfxFrame;

  nsCOMPtr<nsIDOMHTMLImageElement> imgElement(do_QueryInterface(aElement));
  if (!imgElement) {
    
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
    nsCOMPtr<imgIContainer> container;
    rv = request->GetImage(getter_AddRefs(container));
    if (!container)
      return NS_ERROR_FAILURE;

    
    container->GetCurrentFrame(getter_AddRefs(gfxFrame));
  }

  if (!gfxFrame)
    return NS_ERROR_FAILURE;

  
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

  
  rv = WriteBitmap(file, gfxFrame);

  
  if (NS_SUCCEEDED(rv)) {
     PRBool result = PR_FALSE;
     DWORD  dwDisp = 0;
     HKEY   key;
     
     DWORD res = ::RegCreateKeyExW(HKEY_CURRENT_USER,
                                   L"Control Panel\\Desktop",
                                   0, NULL, REG_OPTION_NON_VOLATILE,
                                   KEY_WRITE, NULL, &key, &dwDisp);
    if (REG_SUCCEEDED(res)) {
      PRUnichar tile[2], style[2];
      switch (aPosition) {
        case BACKGROUND_TILE:
          tile[0] = '1';
          style[0] = '1';
          break;
        case BACKGROUND_CENTER:
          tile[0] = '0';
          style[0] = '0';
          break;
        case BACKGROUND_STRETCH:
          tile[0] = '0';
          style[0] = '2';
          break;
      }
      tile[1] = '\0';
      style[1] = '\0';

      
      PRInt32 size = 3 * sizeof(PRUnichar);
      ::RegSetValueExW(key, L"TileWallpaper",
                       0, REG_SZ, (const BYTE *)tile, size);
      ::RegSetValueExW(key, L"WallpaperStyle",
                       0, REG_SZ, (const BYTE *)style, size);
      ::SystemParametersInfoW(SPI_SETDESKWALLPAPER, 0, (PVOID)path.get(),
                              SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
      
      ::RegCloseKey(key);
    }
  }
  return rv;
}

NS_IMETHODIMP
nsWindowsShellService::OpenApplication(PRInt32 aApplication)
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
  PRInt32 end = path.Length();
  PRInt32 cursor = 0, temp = 0;
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

  BOOL success = ::CreateProcessW(NULL, (LPWSTR)path.get(), NULL,
                                  NULL, FALSE, 0, NULL,  NULL,
                                  &si, &pi);
  if (!success)
    return NS_ERROR_FAILURE;

  return NS_OK;
}

NS_IMETHODIMP
nsWindowsShellService::GetDesktopBackgroundColor(PRUint32* aColor)
{
  PRUint32 color = ::GetSysColor(COLOR_DESKTOP);
  *aColor = (GetRValue(color) << 16) | (GetGValue(color) << 8) | GetBValue(color);
  return NS_OK;
}

NS_IMETHODIMP
nsWindowsShellService::SetDesktopBackgroundColor(PRUint32 aColor)
{
  int aParameters[2] = { COLOR_BACKGROUND, COLOR_DESKTOP };
  BYTE r = (aColor >> 16);
  BYTE g = (aColor << 16) >> 24;
  BYTE b = (aColor << 24) >> 24;
  COLORREF colors[2] = { RGB(r,g,b), RGB(r,g,b) };

  ::SetSysColors(sizeof(aParameters) / sizeof(int), aParameters, colors);

  PRBool result = PR_FALSE;
  DWORD  dwDisp = 0;
  HKEY   key;
  
  DWORD rv = ::RegCreateKeyExW(HKEY_CURRENT_USER,
                               L"Control Panel\\Colors", 0, NULL,
                               REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                               &key, &dwDisp);

  if (REG_SUCCEEDED(rv)) {
    char rgb[12];
    sprintf((char*)rgb, "%u %u %u\0", r, g, b);
    NS_ConvertUTF8toUTF16 backColor(rgb);

    ::RegSetValueExW(key, L"Background",
                     0, REG_SZ, (const BYTE *)backColor.get(),
                     (backColor.Length() + 1) * sizeof(PRUnichar));
  }
  
  
  ::RegCloseKey(key);
  return NS_OK;
}

NS_IMETHODIMP
nsWindowsShellService::GetUnreadMailCount(PRUint32* aCount)
{
  *aCount = 0;

  HKEY accountKey;
  if (GetMailAccountKey(&accountKey)) {
    DWORD type, unreadCount;
    DWORD len = sizeof unreadCount;
    DWORD res = ::RegQueryValueExW(accountKey, L"MessageCount", 0,
                                   &type, (LPBYTE)&unreadCount, &len);
    if (REG_SUCCEEDED(res))
      *aCount = unreadCount;

    
    ::RegCloseKey(accountKey);
  }

  return NS_OK;
}

PRBool
nsWindowsShellService::GetMailAccountKey(HKEY* aResult)
{
  NS_NAMED_LITERAL_STRING(unread,
    "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\UnreadMail\\");

  HKEY mailKey;
  DWORD res = ::RegOpenKeyExW(HKEY_CURRENT_USER, unread.get(), 0,
                              KEY_ENUMERATE_SUB_KEYS, &mailKey);

  PRInt32 i = 0;
  do {
    PRUnichar subkeyName[MAX_BUF];
    DWORD len = sizeof subkeyName;
    res = ::RegEnumKeyExW(mailKey, i++, subkeyName, &len, NULL, NULL,
                          NULL, NULL);
    if (REG_SUCCEEDED(res)) {
      HKEY accountKey;
      res = ::RegOpenKeyExW(mailKey, PromiseFlatString(subkeyName).get(),
                            0, KEY_READ, &accountKey);
      if (REG_SUCCEEDED(res)) {
        *aResult = accountKey;
    
        
        ::RegCloseKey(mailKey);
	 
        return PR_TRUE;
      }
    }
    else
      break;
  }
  while (1);

  
  ::RegCloseKey(mailKey);
  return PR_FALSE;
}

NS_IMETHODIMP
nsWindowsShellService::OpenApplicationWithURI(nsILocalFile* aApplication,
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
  PRUint32 pid;
  return process->Run(PR_FALSE, &specStr, 1, &pid);
}

NS_IMETHODIMP
nsWindowsShellService::GetDefaultFeedReader(nsILocalFile** _retval)
{
  *_retval = nsnull;

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

  nsCOMPtr<nsILocalFile> defaultReader =
    do_CreateInstance("@mozilla.org/file/local;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = defaultReader->InitWithPath(path);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool exists;
  rv = defaultReader->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!exists)
    return NS_ERROR_FAILURE;

  NS_ADDREF(*_retval = defaultReader);
  return NS_OK;
}
