










































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
#include "nsIWindowsRegKey.h"
#include "nsUnicharUtils.h"

#include "windows.h"
#include "shellapi.h"
#include "shlobj.h"

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
OpenUserKeyForReading(HKEY aStartKey, const nsAString& aKeyName, HKEY* aKey)
{
  const nsString &flatName = PromiseFlatString(aKeyName);

  DWORD res = ::RegOpenKeyExW(aStartKey, flatName.get(), 0, KEY_READ, aKey);
  switch (res) {
  case ERROR_SUCCESS:
    break;
  case ERROR_ACCESS_DENIED:
    return NS_ERROR_FILE_ACCESS_DENIED;
  case ERROR_FILE_NOT_FOUND:
    if (aStartKey == HKEY_LOCAL_MACHINE) {
      
      
      return NS_ERROR_NOT_AVAILABLE;
    }
    return OpenUserKeyForReading(HKEY_LOCAL_MACHINE, aKeyName, aKey);
  }

  return NS_OK;
}





static nsresult
OpenKeyForWriting(HKEY aStartKey, const nsAString& aKeyName, HKEY* aKey,
                  PRBool aHKLMOnly)
{
  const nsString &flatName = PromiseFlatString(aKeyName);

  DWORD dwDisp = 0;
  DWORD res = ::RegCreateKeyExW(aStartKey, flatName.get(), 0, NULL,
                                0, KEY_READ | KEY_WRITE, NULL, aKey,
                                &dwDisp);
  switch (res) {
  case ERROR_SUCCESS:
    break;
  case ERROR_ACCESS_DENIED:
    if (aHKLMOnly || aStartKey == HKEY_CURRENT_USER)
      return NS_ERROR_FILE_ACCESS_DENIED;
    
    
    return OpenKeyForWriting(HKEY_CURRENT_USER, aKeyName, aKey, aHKLMOnly);
  case ERROR_FILE_NOT_FOUND:
    res = ::RegCreateKeyExW(aStartKey, flatName.get(), 0, NULL,
                            0, KEY_READ | KEY_WRITE, NULL, aKey,
                            NULL);
    if (res != ERROR_SUCCESS) {
      if (aHKLMOnly || aStartKey == HKEY_CURRENT_USER) {
        
        
        return NS_ERROR_FILE_ACCESS_DENIED;
      }
      return OpenKeyForWriting(HKEY_CURRENT_USER, aKeyName, aKey, aHKLMOnly);
    }
  }

  return NS_OK;
}



































































typedef enum {
  NO_SUBSTITUTION           = 0x00,
  APP_PATH_SUBSTITUTION     = 0x01,
  EXE_NAME_SUBSTITUTION     = 0x02,
  UNINST_PATH_SUBSTITUTION  = 0x04,
  HKLM_ONLY                 = 0x08,
  NON_ESSENTIAL             = 0x10
} SettingFlags;

typedef struct {
  char* keyName;
  char* valueName;
  char* valueData;

  PRInt32 flags;
} SETTING;

#define APP_REG_NAME L"Firefox"
#define SMI "SOFTWARE\\Clients\\StartMenuInternet\\"
#define CLS "SOFTWARE\\Classes\\"
#define DI "\\DefaultIcon"
#define II "\\InstallInfo"
#define SOP "\\shell\\open\\command"
#define DDE "\\shell\\open\\ddeexec\\"
#define DDE_NAME "Firefox" // This must be kept in sync with ID_DDE_APPLICATION_NAME as defined in splash.rc
#define DDE_COMMAND "\"%1\",,0,0,,,,"


#define UNINSTALL_EXE "\\uninstall\\helper.exe"

#define CLS_HTML "FirefoxHTML"
#define CLS_URL "FirefoxURL"
#define VAL_FILE_ICON "%APPPATH%,1"
#define VAL_OPEN "\"%APPPATH%\" -requestPending -osint -url \"%1\""

#define MAKE_KEY_NAME1(PREFIX, MID) \
  PREFIX MID

#define MAKE_KEY_NAME2(PREFIX, MID, SUFFIX) \
  PREFIX MID SUFFIX

#define MAKE_KEY_NAME3(PREFIX, MID, MID2, SUFFIX) \
  PREFIX MID MID2 SUFFIX






static SETTING gSettings[] = {
  
  { MAKE_KEY_NAME1(CLS, ".htm"),    "", CLS_HTML, NO_SUBSTITUTION | NON_ESSENTIAL },
  { MAKE_KEY_NAME1(CLS, ".html"),   "", CLS_HTML, NO_SUBSTITUTION | NON_ESSENTIAL },
  { MAKE_KEY_NAME1(CLS, ".shtml"),  "", CLS_HTML, NO_SUBSTITUTION | NON_ESSENTIAL },
  { MAKE_KEY_NAME1(CLS, ".xht"),    "", CLS_HTML, NO_SUBSTITUTION | NON_ESSENTIAL },
  { MAKE_KEY_NAME1(CLS, ".xhtml"),  "", CLS_HTML, NO_SUBSTITUTION | NON_ESSENTIAL },

  
  
  
  { MAKE_KEY_NAME2(CLS, CLS_HTML, DI),  "", VAL_FILE_ICON, APP_PATH_SUBSTITUTION | NON_ESSENTIAL },
  { MAKE_KEY_NAME2(CLS, CLS_HTML, SOP), "", VAL_OPEN, APP_PATH_SUBSTITUTION },

  
  { MAKE_KEY_NAME2(CLS, CLS_URL, DI),  "", VAL_FILE_ICON, APP_PATH_SUBSTITUTION | NON_ESSENTIAL },
  { MAKE_KEY_NAME2(CLS, CLS_URL, SOP), "", VAL_OPEN, APP_PATH_SUBSTITUTION },

  
  { MAKE_KEY_NAME2(CLS, "HTTP", DI),    "", VAL_FILE_ICON, APP_PATH_SUBSTITUTION },
  { MAKE_KEY_NAME2(CLS, "HTTP", SOP),   "", VAL_OPEN, APP_PATH_SUBSTITUTION },
  { MAKE_KEY_NAME2(CLS, "HTTPS", DI),   "", VAL_FILE_ICON, APP_PATH_SUBSTITUTION },
  { MAKE_KEY_NAME2(CLS, "HTTPS", SOP),  "", VAL_OPEN, APP_PATH_SUBSTITUTION },
  { MAKE_KEY_NAME2(CLS, "FTP", DI),     "", VAL_FILE_ICON, APP_PATH_SUBSTITUTION | NON_ESSENTIAL },
  { MAKE_KEY_NAME2(CLS, "FTP", SOP),    "", VAL_OPEN, APP_PATH_SUBSTITUTION | NON_ESSENTIAL },

  
  { MAKE_KEY_NAME2(CLS, CLS_HTML, DDE), "", DDE_COMMAND, NO_SUBSTITUTION | NON_ESSENTIAL },
  { MAKE_KEY_NAME3(CLS, CLS_HTML, DDE, "Application"), "", DDE_NAME, NO_SUBSTITUTION | NON_ESSENTIAL },
  { MAKE_KEY_NAME3(CLS, CLS_HTML, DDE, "Topic"), "", "WWW_OpenURL", NO_SUBSTITUTION | NON_ESSENTIAL },
  { MAKE_KEY_NAME2(CLS, CLS_URL, DDE), "", DDE_COMMAND, NO_SUBSTITUTION | NON_ESSENTIAL },
  { MAKE_KEY_NAME3(CLS, CLS_URL, DDE, "Application"), "", DDE_NAME, NO_SUBSTITUTION | NON_ESSENTIAL },
  { MAKE_KEY_NAME3(CLS, CLS_URL, DDE, "Topic"), "", "WWW_OpenURL", NO_SUBSTITUTION | NON_ESSENTIAL },
  { MAKE_KEY_NAME2(CLS, "HTTP", DDE), "", DDE_COMMAND, NO_SUBSTITUTION | NON_ESSENTIAL },
  { MAKE_KEY_NAME3(CLS, "HTTP", DDE, "Application"), "", DDE_NAME, NO_SUBSTITUTION | NON_ESSENTIAL },
  { MAKE_KEY_NAME3(CLS, "HTTP", DDE, "Topic"), "", "WWW_OpenURL", NO_SUBSTITUTION | NON_ESSENTIAL },
  { MAKE_KEY_NAME2(CLS, "HTTPS", DDE), "", DDE_COMMAND, NO_SUBSTITUTION | NON_ESSENTIAL },
  { MAKE_KEY_NAME3(CLS, "HTTPS", DDE, "Application"), "", DDE_NAME, NO_SUBSTITUTION | NON_ESSENTIAL },
  { MAKE_KEY_NAME3(CLS, "HTTPS", DDE, "Topic"), "", "WWW_OpenURL", NO_SUBSTITUTION | NON_ESSENTIAL },
  { MAKE_KEY_NAME2(CLS, "FTP", DDE), "", DDE_COMMAND, NO_SUBSTITUTION | NON_ESSENTIAL },
  { MAKE_KEY_NAME3(CLS, "FTP", DDE, "Application"), "", DDE_NAME, NO_SUBSTITUTION | NON_ESSENTIAL },
  { MAKE_KEY_NAME3(CLS, "FTP", DDE, "Topic"), "", "WWW_OpenURL", NO_SUBSTITUTION | NON_ESSENTIAL },

  
  { MAKE_KEY_NAME2(SMI, "%APPEXE%", DI),  
    "", 
    "%APPPATH%,0", 
    APP_PATH_SUBSTITUTION | EXE_NAME_SUBSTITUTION | HKLM_ONLY | NON_ESSENTIAL },
  { MAKE_KEY_NAME2(SMI, "%APPEXE%", II),
    "HideIconsCommand",
    "\"%UNINSTPATH%\" /HideShortcuts",
    UNINST_PATH_SUBSTITUTION | EXE_NAME_SUBSTITUTION | HKLM_ONLY | NON_ESSENTIAL },
  { MAKE_KEY_NAME2(SMI, "%APPEXE%", II),
    "ReinstallCommand",
    "\"%UNINSTPATH%\" /SetAsDefaultAppGlobal",
    UNINST_PATH_SUBSTITUTION | EXE_NAME_SUBSTITUTION | HKLM_ONLY | NON_ESSENTIAL },
  { MAKE_KEY_NAME2(SMI, "%APPEXE%", II),
    "ShowIconsCommand",
    "\"%UNINSTPATH%\" /ShowShortcuts",
    UNINST_PATH_SUBSTITUTION | EXE_NAME_SUBSTITUTION | HKLM_ONLY | NON_ESSENTIAL },
  { MAKE_KEY_NAME2(SMI, "%APPEXE%", SOP), 
    "", 
    "%APPPATH%",   
    APP_PATH_SUBSTITUTION | EXE_NAME_SUBSTITUTION | HKLM_ONLY | NON_ESSENTIAL },
  { MAKE_KEY_NAME1(SMI, "%APPEXE%\\shell\\properties\\command"),
    "", 
    "\"%APPPATH%\" -preferences",
    APP_PATH_SUBSTITUTION | EXE_NAME_SUBSTITUTION | HKLM_ONLY | NON_ESSENTIAL },
  { MAKE_KEY_NAME1(SMI, "%APPEXE%\\shell\\safemode\\command"),
    "", 
    "\"%APPPATH%\" -safe-mode",
    APP_PATH_SUBSTITUTION | EXE_NAME_SUBSTITUTION | HKLM_ONLY | NON_ESSENTIAL }

  
  
  
};



#if !defined(IApplicationAssociationRegistration)

typedef enum tagASSOCIATIONLEVEL {
  AL_MACHINE,
  AL_EFFECTIVE,
  AL_USER
} ASSOCIATIONLEVEL;

typedef enum tagASSOCIATIONTYPE {
  AT_FILEEXTENSION,
  AT_URLPROTOCOL,
  AT_STARTMENUCLIENT,
  AT_MIMETYPE
} ASSOCIATIONTYPE;

MIDL_INTERFACE("4e530b0a-e611-4c77-a3ac-9031d022281b")
IApplicationAssociationRegistration : public IUnknown
{
 public:
  virtual HRESULT STDMETHODCALLTYPE QueryCurrentDefault(LPCWSTR pszQuery,
                                                        ASSOCIATIONTYPE atQueryType,
                                                        ASSOCIATIONLEVEL alQueryLevel,
                                                        LPWSTR *ppszAssociation) = 0;
  virtual HRESULT STDMETHODCALLTYPE QueryAppIsDefault(LPCWSTR pszQuery,
                                                      ASSOCIATIONTYPE atQueryType,
                                                      ASSOCIATIONLEVEL alQueryLevel,
                                                      LPCWSTR pszAppRegistryName,
                                                      BOOL *pfDefault) = 0;
  virtual HRESULT STDMETHODCALLTYPE QueryAppIsDefaultAll(ASSOCIATIONLEVEL alQueryLevel,
                                                         LPCWSTR pszAppRegistryName,
                                                         BOOL *pfDefault) = 0;
  virtual HRESULT STDMETHODCALLTYPE SetAppAsDefault(LPCWSTR pszAppRegistryName,
                                                    LPCWSTR pszSet,
                                                    ASSOCIATIONTYPE atSetType) = 0;
  virtual HRESULT STDMETHODCALLTYPE SetAppAsDefaultAll(LPCWSTR pszAppRegistryName) = 0;
  virtual HRESULT STDMETHODCALLTYPE ClearUserAssociations(void) = 0;
};
#endif

static const CLSID CLSID_ApplicationAssociationReg = {0x591209C7,0x767B,0x42B2,{0x9F,0xBA,0x44,0xEE,0x46,0x15,0xF2,0xC7}};
static const IID   IID_IApplicationAssociationReg  = {0x4e530b0a,0xe611,0x4c77,{0xa3,0xac,0x90,0x31,0xd0,0x22,0x28,0x1b}};


PRBool
nsWindowsShellService::IsDefaultBrowserVista(PRBool aStartupCheck, PRBool* aIsDefaultBrowser)
{
  IApplicationAssociationRegistration* pAAR;
  
  HRESULT hr = CoCreateInstance(CLSID_ApplicationAssociationReg,
                                NULL,
                                CLSCTX_INPROC,
                                IID_IApplicationAssociationReg,
                                (void**)&pAAR);
  
  if (SUCCEEDED(hr)) {
    hr = pAAR->QueryAppIsDefaultAll(AL_EFFECTIVE,
                                    APP_REG_NAME,
                                    aIsDefaultBrowser);
    
    
    
    
    if (aStartupCheck)
      mCheckedThisSession = PR_TRUE;
    
    pAAR->Release();
    return PR_TRUE;
  }
  
  return PR_FALSE;
}

PRBool
nsWindowsShellService::SetDefaultBrowserVista()
{
  IApplicationAssociationRegistration* pAAR;
  
  HRESULT hr = CoCreateInstance(CLSID_ApplicationAssociationReg,
                                NULL,
                                CLSCTX_INPROC,
                                IID_IApplicationAssociationReg,
                                (void**)&pAAR);
  
  if (SUCCEEDED(hr)) {
    hr = pAAR->SetAppAsDefaultAll(APP_REG_NAME);
    
    pAAR->Release();
    return PR_TRUE;
  }
  
  return PR_FALSE;
}

NS_IMETHODIMP
nsWindowsShellService::IsDefaultBrowser(PRBool aStartupCheck,
                                        PRBool* aIsDefaultBrowser)
{
  
  
  
  
  
  
  if (IsDefaultBrowserVista(aStartupCheck, aIsDefaultBrowser))
    return NS_OK;

  SETTING* settings;
  SETTING* end = gSettings + sizeof(gSettings)/sizeof(SETTING);

  *aIsDefaultBrowser = PR_TRUE;

  PRUnichar exePath[MAX_BUF];
  if (!::GetModuleFileNameW(0, exePath, MAX_BUF))
    return NS_ERROR_FAILURE;

  nsAutoString appLongPath(exePath);

  
  
  if (!::GetShortPathNameW(exePath, exePath, sizeof(exePath)))
    return NS_ERROR_FAILURE;

  nsAutoString appShortPath(exePath);
  ToUpperCase(appShortPath);

  nsCOMPtr<nsILocalFile> lf;
  nsresult rv = NS_NewLocalFile(appShortPath, PR_TRUE,
                                getter_AddRefs(lf));
  if (NS_FAILED(rv))
    return rv;

  nsAutoString exeName;
  rv = lf->GetLeafName(exeName);
  if (NS_FAILED(rv))
    return rv;
  ToUpperCase(exeName);

  PRUnichar currValue[MAX_BUF];
  for (settings = gSettings; settings < end; ++settings) {
    if (settings->flags & NON_ESSENTIAL)
      continue; 
                
    NS_ConvertUTF8toUTF16 dataLongPath(settings->valueData);
    NS_ConvertUTF8toUTF16 dataShortPath(settings->valueData);
    NS_ConvertUTF8toUTF16 key(settings->keyName);
    NS_ConvertUTF8toUTF16 value(settings->valueName);
    if (settings->flags & APP_PATH_SUBSTITUTION) {
      PRInt32 offset = dataLongPath.Find("%APPPATH%");
      dataLongPath.Replace(offset, 9, appLongPath);
      
      PRInt32 offsetQuoted = dataShortPath.Find("\"%APPPATH%\"");
      if (offsetQuoted != -1)
        dataShortPath.Replace(offsetQuoted, 11, appShortPath);
      else
        dataShortPath.Replace(offset, 9, appShortPath);
    }
    if (settings->flags & EXE_NAME_SUBSTITUTION) {
      PRInt32 offset = key.Find("%APPEXE%");
      key.Replace(offset, 8, exeName);
    }

    ::ZeroMemory(currValue, sizeof(currValue));
    HKEY theKey;
    rv = OpenUserKeyForReading(HKEY_CURRENT_USER, key, &theKey);
    if (NS_SUCCEEDED(rv)) {
      DWORD len = sizeof currValue;
      DWORD res = ::RegQueryValueExW(theKey, PromiseFlatString(value).get(),
                                     NULL, NULL, (LPBYTE)currValue, &len);
      
      ::RegCloseKey(theKey);
      if (REG_FAILED(res) ||
          !dataLongPath.Equals(currValue, CaseInsensitiveCompare) &&
          !dataShortPath.Equals(currValue, CaseInsensitiveCompare)) {
        
        *aIsDefaultBrowser = PR_FALSE;
        break;
      }
    }
  }

  
  
  
  if (aStartupCheck)
    mCheckedThisSession = PR_TRUE;

  return NS_OK;
}

DWORD
nsWindowsShellService::DeleteRegKeyDefaultValue(HKEY baseKey,
                                                const nsString& keyName)
{
  HKEY key;
  DWORD res = ::RegOpenKeyExW(baseKey, keyName.get(),
                              0, KEY_WRITE, &key);
  if (res == ERROR_SUCCESS) {
    res = ::RegDeleteValueW(key, EmptyString().get());
    ::RegCloseKey(key);
  }

  return res;
}

NS_IMETHODIMP
nsWindowsShellService::SetDefaultBrowser(PRBool aClaimAllTypes, PRBool aForAllUsers)
{
  
  
  
  (void)DeleteRegKey(HKEY_CURRENT_USER,
    NS_LITERAL_STRING("Software\\Classes\\http\\shell\\open"));
  (void)DeleteRegKey(HKEY_CURRENT_USER,
    NS_LITERAL_STRING("Software\\Classes\\http\\DefaultIcon"));
  (void)DeleteRegKey(HKEY_CURRENT_USER,
    NS_LITERAL_STRING("Software\\Classes\\https\\shell\\open"));
  (void)DeleteRegKey(HKEY_CURRENT_USER,
     NS_LITERAL_STRING("Software\\Classes\\https\\DefaultIcon"));
  (void)DeleteRegKey(HKEY_CURRENT_USER,
   NS_LITERAL_STRING("Software\\Classes\\ftp\\shell\\open"));
  (void)DeleteRegKey(HKEY_CURRENT_USER,
     NS_LITERAL_STRING("Software\\Classes\\ftp\\DefaultIcon"));
  (void)DeleteRegKey(HKEY_CURRENT_USER,
     NS_LITERAL_STRING("Software\\Classes\\FirefoxURL"));
  (void)DeleteRegKey(HKEY_CURRENT_USER,
     NS_LITERAL_STRING("Software\\Classes\\FirefoxHTML"));

  (void)DeleteRegKeyDefaultValue(HKEY_CURRENT_USER,
     NS_LITERAL_STRING("Software\\Classes\\.htm"));
  (void)DeleteRegKeyDefaultValue(HKEY_CURRENT_USER,
     NS_LITERAL_STRING("Software\\Classes\\.html"));
  (void)DeleteRegKeyDefaultValue(HKEY_CURRENT_USER,
     NS_LITERAL_STRING("Software\\Classes\\.shtml"));
  (void)DeleteRegKeyDefaultValue(HKEY_CURRENT_USER,
     NS_LITERAL_STRING("Software\\Classes\\.xht"));
  (void)DeleteRegKeyDefaultValue(HKEY_CURRENT_USER,
     NS_LITERAL_STRING("Software\\Classes\\.xhtml"));

  if (!aForAllUsers && SetDefaultBrowserVista())
    return NS_OK;

  SETTING* settings;
  SETTING* end = gSettings + sizeof(gSettings)/sizeof(SETTING);

  PRUnichar exePath[MAX_BUF];
  if (!::GetModuleFileNameW(0, exePath, MAX_BUF))
    return NS_ERROR_FAILURE;

  nsAutoString appLongPath(exePath);

  nsCOMPtr<nsILocalFile> lf;
  nsresult rv = NS_NewLocalFile(nsDependentString(exePath), PR_TRUE,
                                getter_AddRefs(lf));
  if (NS_FAILED(rv))
    return rv;

  nsAutoString exeName;
  rv = lf->GetLeafName(exeName);
  if (NS_FAILED(rv))
    return rv;
  ToUpperCase(exeName);

  nsCOMPtr<nsIFile> appDir;
  rv = lf->GetParent(getter_AddRefs(appDir));
  if (NS_FAILED(rv))
    return rv;

  nsAutoString uninstLongPath;
  appDir->GetPath(uninstLongPath);
  uninstLongPath.AppendLiteral(UNINSTALL_EXE);

  for (settings = gSettings; settings < end; ++settings) {
    NS_ConvertUTF8toUTF16 dataLongPath(settings->valueData);
    NS_ConvertUTF8toUTF16 key(settings->keyName);
    NS_ConvertUTF8toUTF16 value(settings->valueName);
    if (settings->flags & APP_PATH_SUBSTITUTION) {
      PRInt32 offset = dataLongPath.Find("%APPPATH%");
      dataLongPath.Replace(offset, 9, appLongPath);
    }
    if (settings->flags & UNINST_PATH_SUBSTITUTION) {
      PRInt32 offset = dataLongPath.Find("%UNINSTPATH%");
      dataLongPath.Replace(offset, 12, uninstLongPath);
    }
    if (settings->flags & EXE_NAME_SUBSTITUTION) {
      PRInt32 offset = key.Find("%APPEXE%");
      key.Replace(offset, 8, exeName);
    }

    SetRegKey(key, value, dataLongPath,
              (settings->flags & HKLM_ONLY));
  }

  
  SetRegKey(NS_LITERAL_STRING(SMI), EmptyString(), exeName, PR_TRUE);

  nsCOMPtr<nsIStringBundleService>
    bundleService(do_GetService("@mozilla.org/intl/stringbundle;1"));
  if (!bundleService)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIStringBundle> bundle, brandBundle;
  rv = bundleService->CreateBundle(SHELLSERVICE_PROPERTIES, getter_AddRefs(bundle));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = bundleService->CreateBundle(BRAND_PROPERTIES, getter_AddRefs(brandBundle));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsString brandFullName;
  brandBundle->GetStringFromName(NS_LITERAL_STRING("brandFullName").get(),
                                 getter_Copies(brandFullName));

  nsAutoString key1(NS_LITERAL_STRING(SMI));
  key1.Append(exeName);
  key1.AppendLiteral("\\");
  SetRegKey(key1, EmptyString(), brandFullName, PR_TRUE);

  
  nsAutoString optionsKey(NS_LITERAL_STRING(SMI));
  optionsKey.Append(exeName);
  optionsKey.AppendLiteral("\\shell\\properties");

  nsAutoString safeModeKey(NS_LITERAL_STRING(SMI));
  safeModeKey.Append(exeName);
  safeModeKey.AppendLiteral("\\shell\\safemode");

  nsString brandShortName;
  brandBundle->GetStringFromName(NS_LITERAL_STRING("brandShortName").get(),
                                 getter_Copies(brandShortName));

  const PRUnichar* brandNameStrings[] = { brandShortName.get() };

  
  nsString optionsTitle;
  bundle->FormatStringFromName(NS_LITERAL_STRING("optionsLabel").get(),
                               brandNameStrings, 1,
                               getter_Copies(optionsTitle));
  
  nsString safeModeTitle;
  bundle->FormatStringFromName(NS_LITERAL_STRING("safeModeLabel").get(),
                               brandNameStrings, 1,
                               getter_Copies(safeModeTitle));

  
  SetRegKey(optionsKey, EmptyString(), optionsTitle, PR_TRUE);
  SetRegKey(safeModeKey, EmptyString(), safeModeTitle, PR_TRUE);

  
  SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, 0, 0);
  return NS_OK;
}


DWORD
nsWindowsShellService::DeleteRegKey(HKEY baseKey, const nsString& keyName)
{
  
  if (keyName.IsEmpty())
    return ERROR_BADKEY;

  const nsString &flatName = PromiseFlatString(keyName);

  
  HKEY key;
  DWORD res = ::RegOpenKeyExW(baseKey, flatName.get(), 0,
                              KEY_ENUMERATE_SUB_KEYS | DELETE, &key);
  
  while (res == ERROR_SUCCESS) {
    PRUnichar subkeyName[MAX_PATH];
    DWORD len = sizeof subkeyName;
    
    
    
    res = ::RegEnumKeyExW(key, 0, subkeyName, &len, NULL, NULL,
                          NULL, NULL);
    if (res == ERROR_NO_MORE_ITEMS) {
      
      res = ::RegDeleteKeyW(baseKey, flatName.get());
      break;
    }
    
    if (res == ERROR_SUCCESS)
      res = DeleteRegKey(key, nsDependentString(subkeyName));
  }
  
  
  ::RegCloseKey(key);
  return res;
}

void
nsWindowsShellService::SetRegKey(const nsString& aKeyName,
                                 const nsString& aValueName,
                                 const nsString& aValue, PRBool aHKLMOnly)
{
  PRUnichar buf[MAX_BUF];
  DWORD len = sizeof buf;

  HKEY theKey;
  nsresult rv = OpenKeyForWriting(HKEY_LOCAL_MACHINE, aKeyName, &theKey,
                                  aHKLMOnly);
  if (NS_FAILED(rv))
    return;

  
  DWORD res = ::RegQueryValueExW(theKey, PromiseFlatString(aValueName).get(),
                                 NULL, NULL, (LPBYTE)buf, &len);

  
  nsAutoString current(buf);
  if (REG_FAILED(res) || !current.Equals(aValue)) {
    const nsString &flatValue = PromiseFlatString(aValue);

    ::RegSetValueExW(theKey, PromiseFlatString(aValueName).get(),
                     0, REG_SZ, (const BYTE *)flatValue.get(),
                     (flatValue.Length() + 1) * sizeof(PRUnichar));
  }

  
  ::RegCloseKey(theKey);
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
                              SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);
      
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

  
  
  

  
  
  
  
  
  nsAutoString clientKey;
  clientKey.AssignLiteral("SOFTWARE\\Clients\\");
  clientKey.Append(application);

  
  HKEY theKey;
  nsresult rv = OpenUserKeyForReading(HKEY_CURRENT_USER, clientKey, &theKey);
  if (NS_FAILED(rv))
    return rv;

  PRUnichar buf[MAX_BUF];
  DWORD type, len = sizeof buf;
  DWORD res = ::RegQueryValueExW(theKey, EmptyString().get(), 0,
                                 &type, (LPBYTE)&buf, &len);

  if (REG_FAILED(res) || !*buf)
    return NS_OK;

  
  ::RegCloseKey(theKey);

  
  clientKey.AppendLiteral("\\");
  clientKey.Append(buf);
  clientKey.AppendLiteral("\\shell\\open\\command");

  rv = OpenUserKeyForReading(HKEY_CURRENT_USER, clientKey, &theKey);
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
