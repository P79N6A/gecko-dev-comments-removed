









































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
#include "shlobj.h"
#include "nsIWindowsRegKey.h"

#include "windows.h"
#include "shellapi.h"

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
OpenUserKeyForReading(HKEY aStartKey, const char* aKeyName, HKEY* aKey)
{
  DWORD result = ::RegOpenKeyEx(aStartKey, aKeyName, 0, KEY_READ, aKey);

  switch (result) {
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
OpenKeyForWriting(HKEY aStartKey, const char* aKeyName, HKEY* aKey,
                  PRBool aHKLMOnly)
{
  DWORD dwDisp = 0;
  DWORD rv = ::RegCreateKeyEx(aStartKey, aKeyName, 0, NULL, 0,
                              KEY_READ | KEY_WRITE, NULL, aKey, &dwDisp);

  switch (rv) {
  case ERROR_SUCCESS:
    break;
  case ERROR_ACCESS_DENIED:
    if (aHKLMOnly || aStartKey == HKEY_CURRENT_USER)
      return NS_ERROR_FILE_ACCESS_DENIED;
    
    
    return OpenKeyForWriting(HKEY_CURRENT_USER, aKeyName, aKey, aHKLMOnly);
  case ERROR_FILE_NOT_FOUND:
    rv = ::RegCreateKey(aStartKey, aKeyName, aKey);
    if (rv != ERROR_SUCCESS) {
      if (aHKLMOnly || aStartKey == HKEY_CURRENT_USER) {
        
        
        return NS_ERROR_FILE_ACCESS_DENIED;
      }
      return OpenKeyForWriting(HKEY_CURRENT_USER, aKeyName, aKey, aHKLMOnly);
    }
  }

  return NS_OK;
}



































































typedef enum { NO_SUBSTITUTION           = 0x00,
               APP_PATH_SUBSTITUTION     = 0x01,
               EXE_NAME_SUBSTITUTION     = 0x02,
               UNINST_PATH_SUBSTITUTION  = 0x04,
               HKLM_ONLY                 = 0x08,
               NON_ESSENTIAL             = 0x10 } SettingFlags;
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
  { MAKE_KEY_NAME2(CLS, "GOPHER", DI),  "", VAL_FILE_ICON, APP_PATH_SUBSTITUTION | NON_ESSENTIAL },
  { MAKE_KEY_NAME2(CLS, "GOPHER", SOP), "", VAL_OPEN, APP_PATH_SUBSTITUTION | NON_ESSENTIAL },

  
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
  { MAKE_KEY_NAME2(CLS, "GOPHER", DDE), "", DDE_COMMAND, NO_SUBSTITUTION | NON_ESSENTIAL },
  { MAKE_KEY_NAME3(CLS, "GOPHER", DDE, "Application"), "", DDE_NAME, NO_SUBSTITUTION | NON_ESSENTIAL },
  { MAKE_KEY_NAME3(CLS, "GOPHER", DDE, "Topic"), "", "WWW_OpenURL", NO_SUBSTITUTION | NON_ESSENTIAL },

  
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

typedef enum tagASSOCIATIONLEVEL
{
  AL_MACHINE,
  AL_EFFECTIVE,
  AL_USER
} ASSOCIATIONLEVEL;

typedef enum tagASSOCIATIONTYPE
{
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
  virtual HRESULT STDMETHODCALLTYPE ClearUserAssociations( void) = 0;
};
#endif

static const CLSID CLSID_ApplicationAssociationReg = {0x591209C7,0x767B,0x42B2,{0x9F,0xBA,0x44,0xEE,0x46,0x15,0xF2,0xC7}};
static const IID   IID_IApplicationAssociationReg  = {0x4e530b0a,0xe611,0x4c77,{0xa3,0xac,0x90,0x31,0xd0,0x22,0x28,0x1b}};


PRBool
nsWindowsShellService::IsDefaultBrowserVista(PRBool aStartupCheck, PRBool* aIsDefaultBrowser)
{
  IApplicationAssociationRegistration* pAAR;
  
  HRESULT hr = CoCreateInstance (CLSID_ApplicationAssociationReg,
                                 NULL,
                                 CLSCTX_INPROC,
                                 IID_IApplicationAssociationReg,
                                 (void**)&pAAR);
  
  if (SUCCEEDED(hr))
  {
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
  
  HRESULT hr = CoCreateInstance (CLSID_ApplicationAssociationReg,
                                 NULL,
                                 CLSCTX_INPROC,
                                 IID_IApplicationAssociationReg,
                                 (void**)&pAAR);
  
  if (SUCCEEDED(hr))
  {
    hr = pAAR->SetAppAsDefaultAll(APP_REG_NAME);
    
    pAAR->Release();
    return PR_TRUE;
  }
  
  return PR_FALSE;
}

NS_IMETHODIMP
nsWindowsShellService::IsDefaultBrowser(PRBool aStartupCheck, PRBool* aIsDefaultBrowser)
{
  
  
  
  
  
  
  if (IsDefaultBrowserVista(aStartupCheck, aIsDefaultBrowser))
    return NS_OK;

  SETTING* settings;
  SETTING* end = gSettings + sizeof(gSettings)/sizeof(SETTING);

  *aIsDefaultBrowser = PR_TRUE;

  char exePath[MAX_BUF];
  if (!::GetModuleFileName(0, exePath, MAX_BUF))
    return NS_ERROR_FAILURE;

  nsCAutoString appLongPath(exePath);

  
  
  if (!::GetShortPathName(exePath, exePath, sizeof(exePath)))
    return NS_ERROR_FAILURE;

  nsCAutoString appShortPath;
  ToUpperCase(appShortPath = exePath);

  nsCOMPtr<nsILocalFile> lf;
  nsresult rv = NS_NewNativeLocalFile(nsDependentCString(exePath), PR_TRUE,
                                      getter_AddRefs(lf));
  if (NS_FAILED(rv))
    return rv;

  nsCAutoString exeName;
  rv = lf->GetNativeLeafName(exeName);
  if (NS_FAILED(rv))
    return rv;
  ToUpperCase(exeName);

  char currValue[MAX_BUF];
  for (settings = gSettings; settings < end; ++settings) {
    if (settings->flags & NON_ESSENTIAL)
      continue; 
                

    nsCAutoString dataLongPath(settings->valueData);
    nsCAutoString dataShortPath(settings->valueData);
    nsCAutoString key(settings->keyName);
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
    nsresult rv = OpenUserKeyForReading(HKEY_CURRENT_USER, key.get(), &theKey);
    if (NS_SUCCEEDED(rv)) {
      DWORD len = sizeof currValue;
      DWORD result = ::RegQueryValueEx(theKey, settings->valueName, NULL, NULL, (LPBYTE)currValue, &len);
      
      ::RegCloseKey(theKey);
      if (REG_FAILED(result) ||
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
nsWindowsShellService::DeleteRegKeyDefaultValue(HKEY baseKey, const char *keyName)
{
  HKEY key;
  DWORD rc = ::RegOpenKeyEx(baseKey, keyName, 0, KEY_WRITE, &key);
  if (rc == ERROR_SUCCESS) {
    rc = ::RegDeleteValue(key, "");
    ::RegCloseKey(key);
  }
  return rc;
}

NS_IMETHODIMP
nsWindowsShellService::SetDefaultBrowser(PRBool aClaimAllTypes, PRBool aForAllUsers)
{
  
  
  
  (void)DeleteRegKey(HKEY_CURRENT_USER, "Software\\Classes\\http\\shell\\open");
  (void)DeleteRegKey(HKEY_CURRENT_USER, "Software\\Classes\\http\\DefaultIcon");
  (void)DeleteRegKey(HKEY_CURRENT_USER, "Software\\Classes\\https\\shell\\open");
  (void)DeleteRegKey(HKEY_CURRENT_USER, "Software\\Classes\\https\\DefaultIcon");
  (void)DeleteRegKey(HKEY_CURRENT_USER, "Software\\Classes\\ftp\\shell\\open");
  (void)DeleteRegKey(HKEY_CURRENT_USER, "Software\\Classes\\ftp\\DefaultIcon");
  (void)DeleteRegKey(HKEY_CURRENT_USER, "Software\\Classes\\gopher\\shell\\open");
  (void)DeleteRegKey(HKEY_CURRENT_USER, "Software\\Classes\\gopher\\DefaultIcon");
  (void)DeleteRegKey(HKEY_CURRENT_USER, "Software\\Classes\\FirefoxURL");
  (void)DeleteRegKey(HKEY_CURRENT_USER, "Software\\Classes\\FirefoxHTML");

  (void)DeleteRegKeyDefaultValue(HKEY_CURRENT_USER, "Software\\Classes\\.htm");
  (void)DeleteRegKeyDefaultValue(HKEY_CURRENT_USER, "Software\\Classes\\.html");
  (void)DeleteRegKeyDefaultValue(HKEY_CURRENT_USER, "Software\\Classes\\.shtml");
  (void)DeleteRegKeyDefaultValue(HKEY_CURRENT_USER, "Software\\Classes\\.xht");
  (void)DeleteRegKeyDefaultValue(HKEY_CURRENT_USER, "Software\\Classes\\.xhtml");

  if (!aForAllUsers && SetDefaultBrowserVista())
    return NS_OK;

  SETTING* settings;
  SETTING* end = gSettings + sizeof(gSettings)/sizeof(SETTING);

  char exePath[MAX_BUF];
  if (!::GetModuleFileName(0, exePath, MAX_BUF))
    return NS_ERROR_FAILURE;

  nsCAutoString appLongPath(exePath);

  nsCOMPtr<nsILocalFile> lf;
  nsresult rv = NS_NewNativeLocalFile(nsDependentCString(exePath), PR_TRUE,
                                      getter_AddRefs(lf));
  if (NS_FAILED(rv))
    return rv;

  nsCAutoString exeName;
  rv = lf->GetNativeLeafName(exeName);
  if (NS_FAILED(rv))
    return rv;
  ToUpperCase(exeName);

  nsCOMPtr<nsIFile> appDir;
  rv = lf->GetParent(getter_AddRefs(appDir));
  if (NS_FAILED(rv))
    return rv;

  nsCAutoString parentPath;
  appDir->GetNativePath(parentPath);

  nsCAutoString uninstLongPath(parentPath.get());
  uninstLongPath.Append(UNINSTALL_EXE);

  for (settings = gSettings; settings < end; ++settings) {
    nsCAutoString dataLongPath(settings->valueData);
    nsCAutoString key(settings->keyName);
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

    SetRegKey(key.get(), settings->valueName, dataLongPath.get(),
              (settings->flags & HKLM_ONLY));
  }

  
  SetRegKey(NS_LITERAL_CSTRING(SMI).get(), "", exeName.get(), PR_TRUE);

  nsCOMPtr<nsIStringBundleService> bundleService(do_GetService("@mozilla.org/intl/stringbundle;1"));
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
  nsCAutoString nativeFullName;
  
  NS_UTF16ToCString(brandFullName, NS_CSTRING_ENCODING_NATIVE_FILESYSTEM,
                    nativeFullName);

  nsCAutoString key1(NS_LITERAL_CSTRING(SMI));
  key1.Append(exeName);
  key1.Append("\\");
  SetRegKey(key1.get(), "", nativeFullName.get(), PR_TRUE);

  
  nsCAutoString optionsKey(SMI);
  optionsKey.Append(exeName);
  optionsKey.Append("\\shell\\properties");

  nsCAutoString safeModeKey(SMI);
  safeModeKey.Append(exeName);
  safeModeKey.Append("\\shell\\safemode");

  nsString brandShortName;
  brandBundle->GetStringFromName(NS_LITERAL_STRING("brandShortName").get(),
                                 getter_Copies(brandShortName));

  const PRUnichar* brandNameStrings[] = { brandShortName.get() };

  
  nsString optionsTitle;
  bundle->FormatStringFromName(NS_LITERAL_STRING("optionsLabel").get(),
                               brandNameStrings, 1, getter_Copies(optionsTitle));
  
  nsString safeModeTitle;
  bundle->FormatStringFromName(NS_LITERAL_STRING("safeModeLabel").get(),
                               brandNameStrings, 1, getter_Copies(safeModeTitle));

  
  nsCAutoString nativeTitle;
  
  NS_UTF16ToCString(optionsTitle, NS_CSTRING_ENCODING_NATIVE_FILESYSTEM,
                    nativeTitle);
  SetRegKey(optionsKey.get(), "", nativeTitle.get(), PR_TRUE);
  
  NS_UTF16ToCString(safeModeTitle, NS_CSTRING_ENCODING_NATIVE_FILESYSTEM,
                    nativeTitle);
  SetRegKey(safeModeKey.get(), "", nativeTitle.get(), PR_TRUE);

  
  SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, 0, 0);
  return NS_OK;
}


DWORD
nsWindowsShellService::DeleteRegKey(HKEY baseKey, const char *keyName)
{
 
 if (!keyName || !::strlen(keyName))
   return ERROR_BADKEY;

 DWORD rc;
 
 HKEY key;
 rc = ::RegOpenKeyEx(baseKey, keyName, 0, KEY_ENUMERATE_SUB_KEYS | DELETE, &key);
 
 
 while (rc == ERROR_SUCCESS) {
   char subkeyName[_MAX_PATH];
   DWORD len = sizeof subkeyName;
   
   
   
   rc = ::RegEnumKeyEx(key, 0, subkeyName, &len, 0, 0, 0, 0);
   if (rc == ERROR_NO_MORE_ITEMS) {
     
     rc = ::RegDeleteKey(baseKey, keyName);
     break;
   } 
   if (rc == ERROR_SUCCESS) {
     
     rc = DeleteRegKey(key, subkeyName);
   }
 }
 
 
 ::RegCloseKey(key);
 return rc;
}

void
nsWindowsShellService::SetRegKey(const char* aKeyName, const char* aValueName, 
                                 const char* aValue, PRBool aHKLMOnly)
{
  char buf[MAX_BUF];
  DWORD len = sizeof buf;

  HKEY theKey;
  nsresult rv = OpenKeyForWriting(HKEY_LOCAL_MACHINE, aKeyName, &theKey, aHKLMOnly);
  if (NS_FAILED(rv))
    return;

  
  DWORD result = ::RegQueryValueEx(theKey, aValueName, NULL, NULL, (LPBYTE)buf, &len);

  
  if (REG_FAILED(result) || strcmp(buf, aValue) != 0)
    ::RegSetValueEx(theKey, aValueName, 0, REG_SZ, 
                    (LPBYTE)aValue, nsDependentCString(aValue).Length());
  
  
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

  
  nsresult rv;

  nsCOMPtr<nsIOutputStream> stream;
  rv = NS_NewLocalFileOutputStream(getter_AddRefs(stream), aFile);
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
    nsCOMPtr<nsIImageLoadingContent> imageContent = do_QueryInterface(aElement, &rv);
    if (!imageContent) return rv;

    
    nsCOMPtr<imgIRequest> request;
    rv = imageContent->GetRequest(nsIImageLoadingContent::CURRENT_REQUEST,
                                  getter_AddRefs(request));
    if (!request) return rv;
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

  nsCAutoString nativePath;
  rv = file->GetNativePath(nativePath);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = WriteBitmap(file, gfxFrame);

  
  if (NS_SUCCEEDED(rv)) {
     char   subKey[] = "Control Panel\\Desktop";
     PRBool result = PR_FALSE;
     DWORD  dwDisp = 0;
     HKEY   key;
     
     DWORD rc = ::RegCreateKeyEx( HKEY_CURRENT_USER,
                                  subKey,
                                  0,
                                  NULL,
                                  REG_OPTION_NON_VOLATILE,
                                  KEY_WRITE,
                                  NULL,
                                  &key,
                                  &dwDisp );
    if (REG_SUCCEEDED(rc)) {
      unsigned char tile[2];
      unsigned char style[2];
      if (aPosition == BACKGROUND_TILE) {
        tile[0] = '1';
        style[0] = '1';
      }
      else if (aPosition == BACKGROUND_CENTER) {
        tile[0] = '0';
        style[0] = '0';
      }
      else if (aPosition == BACKGROUND_STRETCH) {
        tile[0] = '0';
        style[0] = '2';
      }
      tile[1] = '\0';
      style[1] = '\0';
      ::RegSetValueEx(key, "TileWallpaper", 0, REG_SZ, tile, sizeof(tile));
      ::RegSetValueEx(key, "WallpaperStyle", 0, REG_SZ, style, sizeof(style));
      ::SystemParametersInfo(SPI_SETDESKWALLPAPER, 0, (PVOID) nativePath.get(),
                             SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);
      
      ::RegCloseKey(key);
    }
  }
  return rv;
}

NS_IMETHODIMP
nsWindowsShellService::OpenApplication(PRInt32 aApplication)
{
  nsCAutoString application;
  switch (aApplication) {
  case nsIShellService::APPLICATION_MAIL:
    application = NS_LITERAL_CSTRING("Mail");
    break;
  case nsIShellService::APPLICATION_NEWS:
    application = NS_LITERAL_CSTRING("News");
    break;
  }

  
  
  

  
  
  
  
  
  nsCAutoString clientKey(NS_LITERAL_CSTRING("SOFTWARE\\Clients\\"));
  clientKey += application;

  
  HKEY theKey;
  nsresult rv = OpenUserKeyForReading(HKEY_CURRENT_USER, clientKey.get(), &theKey);
  if (NS_FAILED(rv)) return rv;

  char buf[MAX_BUF];
  DWORD type, len = sizeof buf;
  DWORD result = ::RegQueryValueEx(theKey, "", 0, &type, (LPBYTE)&buf, &len);
  if (REG_FAILED(result) || nsDependentCString(buf).IsEmpty()) 
    return NS_OK;

  
  ::RegCloseKey(theKey);

  
  clientKey.Append("\\");
  clientKey.Append(buf);
  clientKey.Append("\\shell\\open\\command");

  rv = OpenUserKeyForReading(HKEY_CURRENT_USER, clientKey.get(), &theKey);
  if (NS_FAILED(rv)) return rv;

  ::ZeroMemory(buf, sizeof(buf));
  len = sizeof buf;
  result = ::RegQueryValueEx(theKey, "", 0, &type, (LPBYTE)&buf, &len);
  if (REG_FAILED(result) || nsDependentCString(buf).IsEmpty()) 
    return NS_ERROR_FAILURE;

  
  ::RegCloseKey(theKey);

  nsCAutoString path(buf);

  
  
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
    ::GetEnvironmentVariable(nsCAutoString(Substring(path, cursor, temp - cursor)).get(), 
                             buf, sizeof(buf));
    
    
    
    path.Replace((cursor - 1), temp - cursor + 2, nsDependentCString(buf));

    ++cursor;
  }
  while (cursor < end);

  STARTUPINFO si;
  PROCESS_INFORMATION pi;

  ::ZeroMemory(&si, sizeof(STARTUPINFO));
  ::ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

  char *pathCStr = ToNewCString(path);
  BOOL success = ::CreateProcess(NULL, pathCStr, NULL, NULL, FALSE, 0, NULL, 
                                 NULL, &si, &pi);
  nsMemory::Free(pathCStr);
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

  char   subKey[] = "Control Panel\\Colors";
  PRBool result = PR_FALSE;
  DWORD  dwDisp = 0;
  HKEY   key;
  
  DWORD rc = ::RegCreateKeyEx(HKEY_CURRENT_USER, subKey, 0, NULL, 
                              REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &key,
                              &dwDisp);
  if (REG_SUCCEEDED(rc)) {
    unsigned char rgb[12];
    sprintf((char*)rgb, "%u %u %u\0", r, g, b);
    ::RegSetValueEx(key, "Background", 0, REG_SZ, (const unsigned char*)rgb, strlen((char*)rgb));
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
    DWORD result = ::RegQueryValueEx(accountKey, "MessageCount", 0, &type, 
                                     (LPBYTE)&unreadCount, &len);
    if (REG_SUCCEEDED(result)) {
      *aCount = unreadCount;
    }

  
  ::RegCloseKey(accountKey);
  }

  return NS_OK;
}

PRBool
nsWindowsShellService::GetMailAccountKey(HKEY* aResult)
{
  HKEY mailKey;
  DWORD result = ::RegOpenKeyEx(HKEY_CURRENT_USER, 
                                "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\UnreadMail\\",
                                0, KEY_ENUMERATE_SUB_KEYS, &mailKey);

  PRInt32 i = 0;
  do {
    char subkeyName[MAX_BUF];
    DWORD len = sizeof subkeyName;
    result = ::RegEnumKeyEx(mailKey, i++, subkeyName, &len, 0, 0, 0, 0);
    if (REG_SUCCEEDED(result)) {
      HKEY accountKey;
      result = ::RegOpenKeyEx(mailKey, subkeyName, 0, KEY_READ, &accountKey);
      if (REG_SUCCEEDED(result)) {
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
nsWindowsShellService::OpenApplicationWithURI(nsILocalFile* aApplication, const nsACString& aURI)
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
