








































#ifdef WINCE_WINDOWS_MOBILE
#include <windows.h>
#include <phone.h>
#endif

#if (MOZ_PLATFORM_MAEMO == 5)
#include <dbus/dbus.h>
#endif

#ifdef MOZ_WIDGET_QT
#include <QtGui/QApplication>
#include <QtGui/QWidget>
#endif

#include "nsPhoneSupport.h"
#include "nsString.h"

NS_IMPL_ISUPPORTS1(nsPhoneSupport, nsIPhoneSupport)

NS_IMETHODIMP
nsPhoneSupport::MakeCall(const PRUnichar *telephoneNumber, const PRUnichar *telephoneDescription, PRBool aPrompt)
{
  long result = -1;

#ifdef WINCE_WINDOWS_MOBILE
  typedef LONG (*__PhoneMakeCall)(PHONEMAKECALLINFO *ppmci);

  HMODULE hPhoneDLL = LoadLibraryW(L"phone.dll");
  if (hPhoneDLL) {
    __PhoneMakeCall MakeCall = (__PhoneMakeCall) GetProcAddress(hPhoneDLL,
                                                                "PhoneMakeCall");
    if (MakeCall) {
      PHONEMAKECALLINFO callInfo;
      callInfo.cbSize          = sizeof(PHONEMAKECALLINFO);
      callInfo.dwFlags         = aPrompt ? PMCF_PROMPTBEFORECALLING : PMCF_DEFAULT;
      callInfo.pszDestAddress  = telephoneNumber;
      callInfo.pszAppName      = nsnull;
      callInfo.pszCalledParty  = telephoneDescription;
      callInfo.pszComment      = nsnull;
      result = MakeCall(&callInfo);
    }
    FreeLibrary(hPhoneDLL);
  }
#endif

  return (result == 0) ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsPhoneSupport::SwitchTask()
{
#if (MOZ_PLATFORM_MAEMO == 5)
  DBusError error;
  dbus_error_init(&error);

  DBusConnection *conn = dbus_bus_get(DBUS_BUS_SESSION, &error);

  DBusMessage *msg = dbus_message_new_signal("/com/nokia/hildon_desktop",
                                             "com.nokia.hildon_desktop",
                                             "exit_app_view");

  if (msg) {
      dbus_connection_send(conn, msg, NULL);
      dbus_message_unref(msg);
      dbus_connection_flush(conn);
  }
  return NS_OK;
#elif MOZ_WIDGET_QT
  QWidget * window = QApplication::activeWindow();
  if (window)
      window->showMinimized();
  return NS_OK;
#else
  return NS_ERROR_NOT_IMPLEMENTED;
#endif
}

#ifdef WINCE



























#define MAX_BUF 1024

#define REG_SUCCEEDED(val) \
  (val == ERROR_SUCCESS)

#define REG_FAILED(val) \
  (val != ERROR_SUCCESS)

typedef struct {
  HKEY keyRoot;
  char* keyName;
  char* valueName;
  char* valueData;
  char* backupValueName;
} SETTING;

#define APP_REG_NAME "Fennec"
#define BACKUP_KEY_ROOT HKEY_LOCAL_MACHINE
#define BACKUP_KEY "Software\\Mozilla\\Fennec\\DefaultBrowserBackup"
#define DEFBROWSERREG "Software\\Microsoft\\Shell\\Rai\\:DEFBROWSER"
#define VAL_OPEN "\"%APPPATH%\" \"%1\""
#define SHELL_OPEN "\\Shell\\Open\\Command"

static SETTING gSettings[] = {
  { HKEY_LOCAL_MACHINE, DEFBROWSERREG,         "0", APP_REG_NAME,    "Rai_DEFBROWSER_0" },
  { HKEY_LOCAL_MACHINE, DEFBROWSERREG,         "1", "\"%APPPATH%\"", "Rai_DEFBROWSER_1" },
  { HKEY_CLASSES_ROOT, "http" SHELL_OPEN,      "",  VAL_OPEN,        "http" },
  { HKEY_CLASSES_ROOT, "https" SHELL_OPEN,     "",  VAL_OPEN,        "https" },
  
  { HKEY_CLASSES_ROOT, "file" SHELL_OPEN,      "",  VAL_OPEN,        "file" },
  { HKEY_CLASSES_ROOT, "ftp" SHELL_OPEN,       "",  VAL_OPEN,        "ftp" },
  { HKEY_CLASSES_ROOT, "htmlfile" SHELL_OPEN,  "",  VAL_OPEN,        "htmlfile" },
  { HKEY_CLASSES_ROOT, "xhtmlfile" SHELL_OPEN, "",  VAL_OPEN,        "xhtmlfile" },
  { HKEY_CLASSES_ROOT, "xmlfile" SHELL_OPEN,   "",  VAL_OPEN,        "xmlfile" },
  { HKEY_CLASSES_ROOT, "xslfile" SHELL_OPEN,   "",  VAL_OPEN,        "xslfile" },
  
  { HKEY_CLASSES_ROOT, ".htm",                 "",  "htmlfile",      ".htm" },
  { HKEY_CLASSES_ROOT, ".html",                "",  "htmlfile",      ".html" },
  { HKEY_CLASSES_ROOT, ".xml",                 "",  "xmlfile",       ".xml" },
  { HKEY_CLASSES_ROOT, ".xsl",                 "",  "xslfile",       ".xsl" },
};

const PRInt32 gSettingsCount = sizeof(gSettings)/sizeof(SETTING);

static nsresult
OpenKeyForReading(HKEY aKeyRoot, const nsAString& aKeyName, HKEY* aKey)
{
  const nsString &flatName = PromiseFlatString(aKeyName);

  DWORD res = ::RegOpenKeyExW(aKeyRoot, flatName.get(), 0, KEY_READ, aKey);
  switch (res) {
  case ERROR_ACCESS_DENIED:
    return NS_ERROR_FILE_ACCESS_DENIED;
  case ERROR_FILE_NOT_FOUND:
    return NS_ERROR_NOT_AVAILABLE;
  }

  return NS_OK;
}

static nsresult
OpenKeyForWriting(HKEY aStartKey, const nsAString& aKeyName, HKEY* aKey)
{
  const nsString &flatName = PromiseFlatString(aKeyName);

  DWORD dwDisp = 0;
  DWORD res = ::RegCreateKeyExW(aStartKey, flatName.get(), 0, NULL,
                                0, KEY_READ | KEY_WRITE, NULL, aKey,
                                &dwDisp);
  switch (res) {
  case ERROR_ACCESS_DENIED:
    return NS_ERROR_FILE_ACCESS_DENIED;
  case ERROR_FILE_NOT_FOUND:
    res = ::RegCreateKeyExW(aStartKey, flatName.get(), 0, NULL,
                            0, KEY_READ | KEY_WRITE, NULL, aKey,
                            NULL);
    if (res != ERROR_SUCCESS)
      return NS_ERROR_FILE_ACCESS_DENIED;
  }

  return NS_OK;
}

void
SetRegValue(HKEY aKeyRoot, const nsString& aKeyName, const nsString& aValueName, const nsString& aValue)
{
  PRUnichar buf[MAX_BUF];
  DWORD len = sizeof buf;

  HKEY theKey;
  nsresult rv = OpenKeyForWriting(aKeyRoot, aKeyName, &theKey);
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

DWORD
GetRegValue(HKEY aKeyRoot, const nsString& aKeyName, const nsString& aValueName, nsString& aValue)
{
  aValue = L"";
  PRUnichar currValue[MAX_BUF];
  ::ZeroMemory(currValue, sizeof(currValue));
  HKEY theKey;
  DWORD res = OpenKeyForReading(aKeyRoot, aKeyName, &theKey);
  if (REG_FAILED(res))
    return res;

  DWORD len = sizeof currValue;
  res = ::RegQueryValueExW(theKey, PromiseFlatString(aValueName).get(),
                                 NULL, NULL, (LPBYTE)currValue, &len);
  
  ::RegCloseKey(theKey);

  if (REG_SUCCEEDED(res))
    aValue = currValue;

  return res;
}
#endif 

NS_IMETHODIMP
nsPhoneSupport::IsDefaultBrowser(PRBool* aIsDefaultBrowser)
{
#ifdef WINCE
  *aIsDefaultBrowser = PR_TRUE;

  SETTING* settings;
  SETTING* end = gSettings + gSettingsCount;

  PRUnichar exePath[MAX_BUF];
  if (!::GetModuleFileNameW(0, exePath, MAX_BUF))
    return NS_ERROR_FAILURE;

  nsAutoString appPath(exePath);

  nsresult rv;
  PRUnichar currValue[MAX_BUF];
  for (settings = gSettings; settings < end; ++settings) {
    NS_ConvertUTF8toUTF16 dataPath(settings->valueData);
    NS_ConvertUTF8toUTF16 key(settings->keyName);
    NS_ConvertUTF8toUTF16 value(settings->valueName);
    PRInt32 offset = dataPath.Find("%APPPATH%");
    if (offset >= 0)
      dataPath.Replace(offset, 9, appPath);

    ::ZeroMemory(currValue, sizeof(currValue));
    HKEY theKey;
    rv = OpenKeyForReading(settings->keyRoot, key, &theKey);
    if (NS_FAILED(rv)) {
      *aIsDefaultBrowser = PR_FALSE;
      return NS_OK;
    }

    DWORD len = sizeof currValue;
    DWORD res = ::RegQueryValueExW(theKey, PromiseFlatString(value).get(),
                                   NULL, NULL, (LPBYTE)currValue, &len);
    
    ::RegCloseKey(theKey);
    if (REG_FAILED(res) ||
        !dataPath.Equals(currValue)) {
      
      *aIsDefaultBrowser = PR_FALSE;
      return NS_OK;
    }
  }

  return NS_OK;
#else
  return NS_ERROR_NOT_IMPLEMENTED;
#endif
}

NS_IMETHODIMP
nsPhoneSupport::SetDefaultBrowser()
{
#ifdef WINCE
  PRBool defaultBrowser = PR_FALSE;
  nsresult result = IsDefaultBrowser(&defaultBrowser);
  if (defaultBrowser || result != NS_OK)
  {
    
    return result;
  }

  SETTING* settings;
  SETTING* end = gSettings + gSettingsCount;

  PRUnichar exePath[MAX_BUF];
  if (!::GetModuleFileNameW(0, exePath, MAX_BUF))
    return NS_ERROR_FAILURE;

  nsAutoString appPath(exePath);

  for (settings = gSettings; settings < end; ++settings) {
    NS_ConvertUTF8toUTF16 dataPath(settings->valueData);
    NS_ConvertUTF8toUTF16 key(settings->keyName);
    NS_ConvertUTF8toUTF16 value(settings->valueName);
    NS_ConvertUTF8toUTF16 backupKey(BACKUP_KEY);
    NS_ConvertUTF8toUTF16 backupValue(settings->backupValueName);
    PRInt32 offset = dataPath.Find("%APPPATH%");
    if (offset >= 0)
      dataPath.Replace(offset, 9, appPath);

    nsString dataOld;
    if (REG_SUCCEEDED(GetRegValue(settings->keyRoot, key, value, dataOld))) {
      SetRegValue(BACKUP_KEY_ROOT, backupKey, backupValue, dataOld);
      SetRegValue(settings->keyRoot, key, value, dataPath);
    }
  }
  
  
  
  
  ::RegFlushKey(HKEY_CLASSES_ROOT);
  ::RegFlushKey(HKEY_LOCAL_MACHINE);

  return NS_OK;
#else
  return NS_ERROR_NOT_IMPLEMENTED;
#endif
}

NS_IMETHODIMP
nsPhoneSupport::RestoreDefaultBrowser()
{
#ifdef WINCE
  PRBool defaultBrowser = PR_FALSE;
  nsresult result = IsDefaultBrowser(&defaultBrowser);
  if (!defaultBrowser || result != NS_OK)
  {
    
    return result;
  }

  SETTING* settings;
  SETTING* end = gSettings + gSettingsCount;

  for (settings = gSettings; settings < end; ++settings) {
    NS_ConvertUTF8toUTF16 key(settings->keyName);
    NS_ConvertUTF8toUTF16 value(settings->valueName);
    NS_ConvertUTF8toUTF16 backupKey(BACKUP_KEY);
    NS_ConvertUTF8toUTF16 backupValue(settings->backupValueName);

    nsString dataOld;
    if (REG_SUCCEEDED(GetRegValue(BACKUP_KEY_ROOT, backupKey, backupValue, dataOld)))
      SetRegValue(settings->keyRoot, key, value, dataOld);
  }
  ::RegFlushKey(HKEY_CLASSES_ROOT);
  ::RegFlushKey(HKEY_LOCAL_MACHINE);

  return NS_OK;
#else
  return NS_ERROR_NOT_IMPLEMENTED;
#endif
}
