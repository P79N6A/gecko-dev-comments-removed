






































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
    nsresult                Unlock();
        
private:
    PRPackedBool            mHaveLock;

#if defined (XP_WIN)
    HANDLE                  mLockFileHandle;
#elif defined (XP_OS2)
    LHANDLE                 mLockFileHandle;
#elif defined (XP_UNIX)
    static void             RemovePidLockFiles();
    static void             FatalSignalHandler(int signo, siginfo_t *info,
                                               void *context);
    static PRCList          mPidLockList;

    nsresult                LockWithFcntl(const nsACString& lockFilePath);

    



    nsresult                LockWithSymlink(const nsACString& lockFilePath, PRBool aHaveFcntlLock);

    char*                   mPidLockFileName;
    int                     mLockFileDesc;
#endif

};

#endif 
