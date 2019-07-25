




































#include "nsProfileMigrator.h"

#include "nsIBrowserProfileMigrator.h"
#include "nsIComponentManager.h"
#include "nsIDOMWindowInternal.h"
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

#include "NSReg.h"
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

#ifndef MAXPATHLEN
#ifdef PATH_MAX
#define MAXPATHLEN PATH_MAX
#elif defined(_MAX_PATH)
#define MAXPATHLEN _MAX_PATH
#elif defined(CCHMAXPATH)
#define MAXPATHLEN CCHMAXPATH
#else
#define MAXPATHLEN 1024
#endif
#endif




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

NS_IMETHODIMP
nsProfileMigrator::Import()
{
  if (ImportRegistryProfiles(NS_LITERAL_CSTRING("Firefox")))
    return NS_OK;

  return NS_ERROR_FAILURE;
}




NS_IMPL_ISUPPORTS1(nsProfileMigrator, nsIProfileMigrator)

#ifdef XP_WIN

#define INTERNAL_NAME_IEXPLORE        "iexplore"
#define INTERNAL_NAME_MOZILLA_SUITE   "apprunner"
#define INTERNAL_NAME_SEAMONKEY       "seamonkey"
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
  if (internalName.LowerCaseEqualsLiteral(INTERNAL_NAME_MOZILLA_SUITE) ||
      internalName.LowerCaseEqualsLiteral(INTERNAL_NAME_SEAMONKEY)) {
    aKey = "seamonkey";
    return NS_OK;
  }
  if (internalName.LowerCaseEqualsLiteral(INTERNAL_NAME_OPERA)) {
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
  CHECK_MIGRATOR("seamonkey");
  CHECK_MIGRATOR("opera");

#undef CHECK_MIGRATOR
#endif
  return NS_ERROR_FAILURE;
}

PRBool
nsProfileMigrator::ImportRegistryProfiles(const nsACString& aAppName)
{
  nsresult rv;

  nsCOMPtr<nsIToolkitProfileService> profileSvc
    (do_GetService(NS_PROFILESERVICE_CONTRACTID));
  NS_ENSURE_TRUE(profileSvc, NS_ERROR_FAILURE);

  nsCOMPtr<nsIProperties> dirService
    (do_GetService("@mozilla.org/file/directory_service;1"));
  NS_ENSURE_TRUE(dirService, NS_ERROR_FAILURE);

  nsCOMPtr<nsILocalFile> regFile;
#ifdef XP_WIN
  rv = dirService->Get(NS_WIN_APPDATA_DIR, NS_GET_IID(nsILocalFile),
                       getter_AddRefs(regFile));
  NS_ENSURE_SUCCESS(rv, PR_FALSE);
  regFile->AppendNative(aAppName);
  regFile->AppendNative(NS_LITERAL_CSTRING("registry.dat"));
#elif defined(XP_MACOSX)
  rv = dirService->Get(NS_MAC_USER_LIB_DIR, NS_GET_IID(nsILocalFile),
                       getter_AddRefs(regFile));
  NS_ENSURE_SUCCESS(rv, PR_FALSE);
  regFile->AppendNative(aAppName);
  regFile->AppendNative(NS_LITERAL_CSTRING("Application Registry"));
#elif defined(XP_OS2)
  rv = dirService->Get(NS_OS2_HOME_DIR, NS_GET_IID(nsILocalFile),
                       getter_AddRefs(regFile));
  NS_ENSURE_SUCCESS(rv, PR_FALSE);
  regFile->AppendNative(aAppName);
  regFile->AppendNative(NS_LITERAL_CSTRING("registry.dat"));
#else
  rv = dirService->Get(NS_UNIX_HOME_DIR, NS_GET_IID(nsILocalFile),
                       getter_AddRefs(regFile));
  NS_ENSURE_SUCCESS(rv, PR_FALSE);
  nsCAutoString dotAppName;
  ToLowerCase(aAppName, dotAppName);
  dotAppName.Insert('.', 0);
  
  regFile->AppendNative(dotAppName);
  regFile->AppendNative(NS_LITERAL_CSTRING("appreg"));
#endif

  nsCAutoString path;
  rv = regFile->GetNativePath(path);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  if (NR_StartupRegistry())
    return PR_FALSE;

  PRBool migrated = PR_FALSE;
  HREG reg = nsnull;
  RKEY profiles = 0;
  REGENUM enumstate = 0;
  char profileName[MAXREGNAMELEN];

  if (NR_RegOpen(path.get(), &reg))
    goto cleanup;

  if (NR_RegGetKey(reg, ROOTKEY_COMMON, "Profiles", &profiles))
    goto cleanup;

  while (!NR_RegEnumSubkeys(reg, profiles, &enumstate,
                            profileName, MAXREGNAMELEN, REGENUM_CHILDREN)) {
#ifdef DEBUG_bsmedberg
    printf("Found profile %s.\n", profileName);
#endif

    RKEY profile = 0;
    if (NR_RegGetKey(reg, profiles, profileName, &profile)) {
      NS_ERROR("Could not get the key that was enumerated.");
      continue;
    }

    char profilePath[MAXPATHLEN];
    if (NR_RegGetEntryString(reg, profile, "directory",
                             profilePath, MAXPATHLEN))
      continue;

    nsCOMPtr<nsILocalFile> profileFile
      (do_CreateInstance("@mozilla.org/file/local;1"));
    if (!profileFile)
      continue;

#if defined (XP_MACOSX)
    rv = profileFile->SetPersistentDescriptor(nsDependentCString(profilePath));
#else
    NS_ConvertUTF8toUTF16 widePath(profilePath);
    rv = profileFile->InitWithPath(widePath);
#endif
    if (NS_FAILED(rv)) continue;

    nsCOMPtr<nsIToolkitProfile> tprofile;
    profileSvc->CreateProfile(profileFile, nsnull,
                              nsDependentCString(profileName),
                              getter_AddRefs(tprofile));
    migrated = PR_TRUE;
  }

cleanup:
  if (reg)
    NR_RegClose(reg);
  NR_ShutdownRegistry();
  return migrated;
}
