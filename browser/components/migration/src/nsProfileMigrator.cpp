




































#include "nsProfileMigrator.h"

#include "nsIBrowserProfileMigrator.h"
#include "nsIComponentManager.h"
#include "nsIDOMWindow.h"
#include "nsILocalFile.h"
#include "nsIObserverService.h"
#include "nsIProperties.h"
#include "nsIServiceManager.h"
#include "nsISupportsPrimitives.h"
#include "nsISupportsArray.h"
#include "nsIToolkitProfile.h"
#include "nsIToolkitProfileService.h"
#include "nsIWindowWatcher.h"

#include "nsCOMPtr.h"
#include "nsBrowserCompsCID.h"
#include "nsComponentManagerUtils.h"
#include "nsDirectoryServiceDefs.h"
#include "nsServiceManagerUtils.h"

#include "nsStringAPI.h"
#include "nsUnicharUtils.h"
#ifdef XP_WIN
#include <windows.h>
#include "nsIWindowsRegKey.h"
#include "nsILocalFileWin.h"
#else
#include <limits.h>
#endif

#include "nsAutoPtr.h"




#define MIGRATION_WIZARD_FE_URL "chrome://browser/content/migration/migration.xul"
#define MIGRATION_WIZARD_FE_FEATURES "chrome,dialog,modal,centerscreen,titlebar"

NS_IMETHODIMP
nsProfileMigrator::Migrate(nsIProfileStartup* aStartup)
{
  nsresult rv;

  nsCAutoString key;
  nsCOMPtr<nsIBrowserProfileMigrator> bpm;

  rv = GetDefaultBrowserMigratorKey(key, bpm);
  if (NS_FAILED(rv)) return rv;

  if (!bpm) {
    nsCAutoString contractID(NS_BROWSERPROFILEMIGRATOR_CONTRACTID_PREFIX);
    contractID.Append(key);

    bpm = do_CreateInstance(contractID.get());
    if (!bpm) return NS_ERROR_FAILURE;
  }

  PRBool sourceExists;
  bpm->GetSourceExists(&sourceExists);
  if (!sourceExists) {
#ifdef XP_WIN
    
    
    
    bpm = do_CreateInstance(NS_BROWSERPROFILEMIGRATOR_CONTRACTID_PREFIX "ie");
#else
    return NS_ERROR_FAILURE;
#endif
  }

  nsCOMPtr<nsISupportsCString> cstr
    (do_CreateInstance("@mozilla.org/supports-cstring;1"));
  if (!cstr) return NS_ERROR_OUT_OF_MEMORY;
  cstr->SetData(key);

  
  
  nsCOMPtr<nsIWindowWatcher> ww(do_GetService(NS_WINDOWWATCHER_CONTRACTID));
  nsCOMPtr<nsISupportsArray> params = 
    do_CreateInstance(NS_SUPPORTSARRAY_CONTRACTID);
  if (!ww || !params) return NS_ERROR_FAILURE;

  params->AppendElement(cstr);
  params->AppendElement(bpm);
  params->AppendElement(aStartup);

  nsCOMPtr<nsIDOMWindow> migrateWizard;
  return ww->OpenWindow(nsnull, 
                        MIGRATION_WIZARD_FE_URL,
                        "_blank",
                        MIGRATION_WIZARD_FE_FEATURES,
                        params,
                        getter_AddRefs(migrateWizard));
}




NS_IMPL_ISUPPORTS1(nsProfileMigrator, nsIProfileMigrator)

#ifdef XP_WIN

#define INTERNAL_NAME_IEXPLORE        "iexplore"
#define INTERNAL_NAME_MOZILLA_SUITE   "apprunner"
#define INTERNAL_NAME_OPERA           "opera"
#endif

nsresult
nsProfileMigrator::GetDefaultBrowserMigratorKey(nsACString& aKey,
                                                nsCOMPtr<nsIBrowserProfileMigrator>& bpm)
{
#if XP_WIN

  nsCOMPtr<nsIWindowsRegKey> regKey = 
    do_CreateInstance("@mozilla.org/windows-registry-key;1");
  if (!regKey)
    return NS_ERROR_FAILURE;

  NS_NAMED_LITERAL_STRING(kCommandKey,
                          "SOFTWARE\\Classes\\HTTP\\shell\\open\\command");

  if (NS_FAILED(regKey->Open(nsIWindowsRegKey::ROOT_KEY_LOCAL_MACHINE,
                             kCommandKey, nsIWindowsRegKey::ACCESS_READ)))
    return NS_ERROR_FAILURE;

  nsAutoString value;
  if (NS_FAILED(regKey->ReadStringValue(EmptyString(), value)))
    return NS_ERROR_FAILURE;

  PRInt32 len = value.Find(NS_LITERAL_STRING(".exe"), CaseInsensitiveCompare);
  if (len == -1)
    return NS_ERROR_FAILURE;

  
  len += 4;

  PRUint32 start = 0;
  
  if (value.get()[1] != ':') {
    start = 1;
    --len;
  }

  const nsDependentSubstring filePath(Substring(value, start, len)); 

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  nsCOMPtr<nsILocalFile> lf;
  NS_NewLocalFile(filePath, PR_TRUE, getter_AddRefs(lf));
  if (!lf)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsILocalFileWin> lfw = do_QueryInterface(lf); 
  if (!lfw)
    return NS_ERROR_FAILURE;

  nsAutoString internalName;
  if (NS_FAILED(lfw->GetVersionInfoField("InternalName", internalName)))
    return NS_ERROR_FAILURE;

  if (internalName.LowerCaseEqualsLiteral(INTERNAL_NAME_IEXPLORE)) {
    aKey = "ie";
    return NS_OK;
  }
  else if (internalName.LowerCaseEqualsLiteral(INTERNAL_NAME_OPERA)) {
    aKey = "opera";
    return NS_OK;
  }

#else
  PRBool exists = PR_FALSE;
#define CHECK_MIGRATOR(browser) do {\
  bpm = do_CreateInstance(NS_BROWSERPROFILEMIGRATOR_CONTRACTID_PREFIX browser);\
  if (bpm)\
    bpm->GetSourceExists(&exists);\
  if (exists) {\
    aKey = browser;\
    return NS_OK;\
  }} while(0)

#if defined(XP_MACOSX)
  CHECK_MIGRATOR("safari");
#endif
  CHECK_MIGRATOR("opera");

#undef CHECK_MIGRATOR
#endif
  return NS_ERROR_FAILURE;
}
