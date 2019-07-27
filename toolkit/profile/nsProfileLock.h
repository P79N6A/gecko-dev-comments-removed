




#ifndef __nsProfileLock_h___
#define __nsProfileLock_h___

#include "nsIFile.h"

class nsIProfileUnlocker;

#if defined (XP_WIN)
#include <windows.h>
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
                       
    








    nsresult                Lock(nsIFile* aProfileDir, nsIProfileUnlocker* *aUnlocker);

    




    nsresult                Unlock(bool aFatalSignal = false);

    


    nsresult                GetReplacedLockTime(PRTime* aResult);

private:
    bool                    mHaveLock;
    PRTime                  mReplacedLockTime;

#if defined (XP_WIN)
    HANDLE                  mLockFileHandle;
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

    nsresult                LockWithFcntl(nsIFile *aLockFile);

    



    nsresult                LockWithSymlink(nsIFile *aLockFile, bool aHaveFcntlLock);

    char*                   mPidLockFileName;
    int                     mLockFileDesc;
#endif

};

#endif 
