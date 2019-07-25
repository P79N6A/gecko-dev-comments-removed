





#ifndef nsUpdateDriver_h__
#define nsUpdateDriver_h__

#include "nscore.h"
#ifdef MOZ_UPDATER
#include "nsIUpdateService.h"
#include "nsIThread.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "mozilla/Attributes.h"
#endif

class nsIFile;

#if defined(XP_WIN)
#include <windows.h>
  typedef HANDLE     ProcessType;
#elif defined(XP_MACOSX)
  typedef pid_t      ProcessType;
#else
#include "prproces.h"
  typedef PRProcess* ProcessType;
#endif





















NS_HIDDEN_(nsresult) ProcessUpdates(nsIFile *greDir, nsIFile *appDir,
                                    nsIFile *updRootDir,
                                    int argc, char **argv,
                                    const char *appVersion,
                                    bool restart = true,
                                    ProcessType *pid = nullptr);

#ifdef MOZ_UPDATER




class nsUpdateProcessor MOZ_FINAL : public nsIUpdateProcessor
{
public:
  nsUpdateProcessor();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIUPDATEPROCESSOR

private:
  struct BackgroundUpdateInfo {
    BackgroundUpdateInfo()
      : mArgc(0),
        mArgv(nullptr)
    {}
    ~BackgroundUpdateInfo() {
      for (int i = 0; i < mArgc; ++i) {
        delete[] mArgv[i];
      }
      delete[] mArgv;
    }

    nsCOMPtr<nsIFile> mGREDir;
    nsCOMPtr<nsIFile> mAppDir;
    nsCOMPtr<nsIFile> mUpdateRoot;
    int mArgc;
    char **mArgv;
    nsAutoCString mAppVersion;
  };

private:
  void StartBackgroundUpdate();
  void WaitForProcess();
  void UpdateDone();
  void ShutdownWatcherThread();

private:
  ProcessType mUpdaterPID;
  nsCOMPtr<nsIThread> mProcessWatcher;
  nsCOMPtr<nsIUpdate> mUpdate;
  BackgroundUpdateInfo mInfo;
};
#endif

#endif  
