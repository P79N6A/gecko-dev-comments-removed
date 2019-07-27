





#ifndef nsUpdateDriver_h__
#define nsUpdateDriver_h__

#include "nscore.h"
#include "nsIUpdateService.h"
#include "nsIThread.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "mozilla/Attributes.h"

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





















nsresult ProcessUpdates(nsIFile *greDir, nsIFile *appDir,
                                    nsIFile *updRootDir,
                                    int argc, char **argv,
                                    const char *appVersion,
                                    bool restart = true,
                                    bool isOSUpdate = false,
                                    nsIFile *osApplyToDir = nullptr,
                                    ProcessType *pid = nullptr);





class nsUpdateProcessor final : public nsIUpdateProcessor
{
public:
  nsUpdateProcessor();

  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIUPDATEPROCESSOR

private:
  ~nsUpdateProcessor();

  struct StagedUpdateInfo {
    StagedUpdateInfo()
      : mArgc(0),
        mArgv(nullptr),
        mIsOSUpdate(false)
    {}
    ~StagedUpdateInfo() {
      for (int i = 0; i < mArgc; ++i) {
        delete[] mArgv[i];
      }
      delete[] mArgv;
    }

    nsCOMPtr<nsIFile> mGREDir;
    nsCOMPtr<nsIFile> mAppDir;
    nsCOMPtr<nsIFile> mUpdateRoot;
    nsCOMPtr<nsIFile> mOSApplyToDir;
    int mArgc;
    char **mArgv;
    nsAutoCString mAppVersion;
    bool mIsOSUpdate;
  };

private:
  void StartStagedUpdate();
  void WaitForProcess();
  void UpdateDone();
  void ShutdownWatcherThread();

private:
  ProcessType mUpdaterPID;
  nsCOMPtr<nsIThread> mProcessWatcher;
  StagedUpdateInfo mInfo;
};
#endif  
