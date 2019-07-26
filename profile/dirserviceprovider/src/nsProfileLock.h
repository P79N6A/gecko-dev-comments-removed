




#ifndef __nsProfileLock_h___
#define __nsProfileLock_h___

#include "nsILocalFile.h"

class nsIProfileUnlocker;

#if defined (XP_WIN)
#include <windows.h>
#endif

#if defined (XP_OS2)
#define INCL_DOSERRORS
#define INCL_DOSFILEMGR
#include <os2.h>
#endif

#if defined (XP_UNIX)
#include <signal.h>
#include "prclist.h"
#endif

class nsProfileLock
#if defined (XP_UNIX)
  : public PRCList
#endif
{
public:
                            nsProfileLock();
                            nsProfileLock(nsProfileLock& src);

                            ~nsProfileLock();
 
    nsProfileLock&          operator=(nsProfileLock& rhs);
                       
    








    nsresult                Lock(nsILocalFile* aProfileDir, nsIProfileUnlocker* *aUnlocker);

    




    nsresult                Unlock(bool aFatalSignal = false);

    


    nsresult                GetReplacedLockTime(PRInt64* aResult);

private:
    bool                    mHaveLock;
    PRInt64                 mReplacedLockTime;

#if defined (XP_WIN)
    HANDLE                  mLockFileHandle;
#elif defined (XP_OS2)
    LHANDLE                 mLockFileHandle;
#elif defined (XP_UNIX)

    struct RemovePidLockFilesExiting {
        RemovePidLockFilesExiting() {}
        ~RemovePidLockFilesExiting() {
            RemovePidLockFiles(false);
        }
    };

    static void             RemovePidLockFiles(bool aFatalSignal);
    static void             FatalSignalHandler(int signo
#ifdef SA_SIGINFO
                                               , siginfo_t *info, void *context
#endif
                                               );
    static PRCList          mPidLockList;

    nsresult                LockWithFcntl(nsILocalFile *aLockFile);

    



    nsresult                LockWithSymlink(nsILocalFile *aLockFile, bool aHaveFcntlLock);

    char*                   mPidLockFileName;
    int                     mLockFileDesc;
#endif

};

#endif 
