




#include "nsIToolkitProfileService.h"
#include "nsIFile.h"
#include "nsThreadUtils.h"

static bool gProfileResetCleanupCompleted = false;
static const char kResetProgressURL[] = "chrome://global/content/resetProfileProgress.xul";

nsresult CreateResetProfile(nsIToolkitProfileService* aProfileSvc,
                            nsIToolkitProfile* *aNewProfile);

nsresult ProfileResetCleanup(nsIToolkitProfile* aOldProfile);

class ProfileResetCleanupResultTask : public nsRunnable
{
public:
  ProfileResetCleanupResultTask()
    : mWorkerThread(do_GetCurrentThread())
  {
    MOZ_ASSERT(!NS_IsMainThread());
  }

  NS_IMETHOD Run() {
    MOZ_ASSERT(NS_IsMainThread());
    mWorkerThread->Shutdown();
    return NS_OK;
  }

private:
  nsCOMPtr<nsIThread> mWorkerThread;
};

class ProfileResetCleanupAsyncTask : public nsRunnable
{
public:
  ProfileResetCleanupAsyncTask(nsIFile* aProfileDir, nsIFile* aProfileLocalDir,
                               nsIFile* aTargetDir, const nsAString &aLeafName)
    : mProfileDir(aProfileDir)
    , mProfileLocalDir(aProfileLocalDir)
    , mTargetDir(aTargetDir)
    , mLeafName(aLeafName)
  { }




  NS_IMETHOD Run()
  {
    
    nsresult rv = mProfileDir->CopyToFollowingLinks(mTargetDir, mLeafName);
    if (NS_SUCCEEDED(rv))
      rv = mProfileDir->Remove(true);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      NS_WARNING("Could not backup the root profile directory");
    }

    
    
    bool sameDir;
    nsresult rvLocal = mProfileDir->Equals(mProfileLocalDir, &sameDir);
    if (NS_SUCCEEDED(rvLocal) && !sameDir) {
      rvLocal = mProfileLocalDir->Remove(true);
      if (NS_FAILED(rvLocal)) NS_WARNING("Could not remove the old local profile directory (cache)");
    }
    gProfileResetCleanupCompleted = true;

    nsCOMPtr<nsIRunnable> resultRunnable = new ProfileResetCleanupResultTask();
    NS_DispatchToMainThread(resultRunnable);
    return NS_OK;
  }

private:
  nsCOMPtr<nsIFile> mProfileDir;
  nsCOMPtr<nsIFile> mProfileLocalDir;
  nsCOMPtr<nsIFile> mTargetDir;
  nsAutoString mLeafName;
};
