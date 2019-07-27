




#include "nsIAppStartup.h"
#include "nsIDOMWindow.h"
#include "nsIFile.h"
#include "nsIStringBundle.h"
#include "nsIToolkitProfile.h"
#include "nsIWindowWatcher.h"

#include "ProfileReset.h"

#include "nsDirectoryServiceDefs.h"
#include "nsDirectoryServiceUtils.h"
#include "nsPrintfCString.h"
#include "nsToolkitCompsCID.h"
#include "nsXPCOMCIDInternal.h"
#include "nsXREAppData.h"

#include "mozilla/Services.h"
#include "prtime.h"

extern const nsXREAppData* gAppData;

static const char kProfileProperties[] =
  "chrome://mozapps/locale/profile/profileSelection.properties";




nsresult
CreateResetProfile(nsIToolkitProfileService* aProfileSvc, nsIToolkitProfile* *aNewProfile)
{
  MOZ_ASSERT(aProfileSvc, "NULL profile service");

  nsCOMPtr<nsIToolkitProfile> newProfile;
  
  nsAutoCString newProfileName("default-");
  newProfileName.Append(nsPrintfCString("%lld", PR_Now() / 1000));
  nsresult rv = aProfileSvc->CreateProfile(nullptr, 
                                           newProfileName,
                                           getter_AddRefs(newProfile));
  if (NS_FAILED(rv)) return rv;

  rv = aProfileSvc->Flush();
  if (NS_FAILED(rv)) return rv;

  newProfile.swap(*aNewProfile);

  return NS_OK;
}




nsresult
ProfileResetCleanup(nsIToolkitProfile* aOldProfile)
{
  nsresult rv;
  nsCOMPtr<nsIFile> profileDir;
  rv = aOldProfile->GetRootDir(getter_AddRefs(profileDir));
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsIFile> profileLocalDir;
  rv = aOldProfile->GetLocalDir(getter_AddRefs(profileLocalDir));
  if (NS_FAILED(rv)) return rv;

  
  nsCOMPtr<nsIStringBundleService> sbs = mozilla::services::GetStringBundleService();
  if (!sbs) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIStringBundle> sb;
  rv = sbs->CreateBundle(kProfileProperties, getter_AddRefs(sb));
  if (!sb) return NS_ERROR_FAILURE;

  NS_ConvertUTF8toUTF16 appName(gAppData->name);
  const char16_t* params[] = {appName.get(), appName.get()};

  nsXPIDLString resetBackupDirectoryName;

  static const char16_t* kResetBackupDirectory = MOZ_UTF16("resetBackupDirectory");
  rv = sb->FormatStringFromName(kResetBackupDirectory, params, 2,
                                getter_Copies(resetBackupDirectoryName));

  
  nsCOMPtr<nsIFile> backupDest, containerDest, profileDest;
  rv = NS_GetSpecialDirectory(NS_OS_DESKTOP_DIR, getter_AddRefs(backupDest));
  if (NS_FAILED(rv)) {
    
    rv = NS_GetSpecialDirectory(NS_OS_HOME_DIR, getter_AddRefs(backupDest));
    if (NS_FAILED(rv)) return rv;
  }

  
  backupDest->Clone(getter_AddRefs(containerDest));
  containerDest->Append(resetBackupDirectoryName);
  rv = containerDest->Create(nsIFile::DIRECTORY_TYPE, 0700);
  
  if (rv == NS_ERROR_FILE_ALREADY_EXISTS) {
    bool containerIsDir;
    rv = containerDest->IsDirectory(&containerIsDir);
    if (NS_FAILED(rv) || !containerIsDir) {
      return rv;
    }
  } else if (NS_FAILED(rv)) {
    return rv;
  }

  
  nsAutoString leafName;
  rv = profileDir->GetLeafName(leafName);
  if (NS_FAILED(rv)) return rv;

  
  containerDest->Clone(getter_AddRefs(profileDest));
  profileDest->Append(leafName);
  rv = profileDest->CreateUnique(nsIFile::DIRECTORY_TYPE, 0700);
  if (NS_FAILED(rv)) return rv;

  
  rv = profileDest->GetLeafName(leafName);
  if (NS_FAILED(rv)) return rv;

  
  rv = profileDest->Remove(false);
  if (NS_FAILED(rv)) return rv;

  
  nsCOMPtr<nsIWindowWatcher> windowWatcher(do_GetService(NS_WINDOWWATCHER_CONTRACTID));
  if (!windowWatcher) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIAppStartup> appStartup(do_GetService(NS_APPSTARTUP_CONTRACTID));
  if (!appStartup) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMWindow> progressWindow;
  rv = windowWatcher->OpenWindow(nullptr,
                                 kResetProgressURL,
                                 "_blank",
                                 "centerscreen,chrome,titlebar",
                                 nullptr,
                                 getter_AddRefs(progressWindow));
  if (NS_FAILED(rv)) return rv;

  
  nsCOMPtr<nsIThreadManager> tm = do_GetService(NS_THREADMANAGER_CONTRACTID);
  nsCOMPtr<nsIThread> cleanupThread;
  rv = tm->NewThread(0, 0, getter_AddRefs(cleanupThread));
  if (NS_SUCCEEDED(rv)) {
    nsCOMPtr<nsIRunnable> runnable = new ProfileResetCleanupAsyncTask(profileDir, profileLocalDir,
                                                                      containerDest, leafName);
    cleanupThread->Dispatch(runnable, nsIThread::DISPATCH_NORMAL);
    

    nsIThread *thread = NS_GetCurrentThread();
    
    while(!gProfileResetCleanupCompleted) {
      NS_ProcessNextEvent(thread);
    }
  } else {
    gProfileResetCleanupCompleted = true;
    NS_WARNING("Cleanup thread creation failed");
    return rv;
  }
  
  progressWindow->Close();

  
  rv = aOldProfile->Remove(false);
  if (NS_FAILED(rv)) NS_WARNING("Could not remove the profile");

  return rv;
}
