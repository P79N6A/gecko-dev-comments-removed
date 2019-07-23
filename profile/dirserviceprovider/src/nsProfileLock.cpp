









































#include "nsProfileStringTypes.h"
#include "nsProfileLock.h"
#include "nsCOMPtr.h"

#if defined(XP_MACOSX)
#include <Carbon/Carbon.h>
#include <CoreFoundation/CoreFoundation.h>
#endif

#ifdef XP_UNIX
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include "prnetdb.h"
#include "prsystem.h"
#include "prprf.h"
#include "prenv.h"
#endif

#ifdef VMS
#include <rmsdef.h>
#endif







#if defined (XP_UNIX)
static PRBool sDisableSignalHandling = PR_FALSE;
#endif

nsProfileLock::nsProfileLock() :
    mHaveLock(PR_FALSE)
#if defined (XP_WIN)
    ,mLockFileHandle(INVALID_HANDLE_VALUE)
#elif defined (XP_OS2)
    ,mLockFileHandle(-1)
#elif defined (XP_UNIX)
    ,mPidLockFileName(nsnull)
    ,mLockFileDesc(-1)
#endif
{
#if defined (XP_UNIX)
    next = prev = this;
    sDisableSignalHandling = PR_GetEnv("MOZ_DISABLE_SIG_HANDLER") ? PR_TRUE : PR_FALSE;
#endif
}


nsProfileLock::nsProfileLock(nsProfileLock& src)
{
    *this = src;
}


nsProfileLock& nsProfileLock::operator=(nsProfileLock& rhs)
{
    Unlock();

    mHaveLock = rhs.mHaveLock;
    rhs.mHaveLock = PR_FALSE;

#if defined (XP_WIN)
    mLockFileHandle = rhs.mLockFileHandle;
    rhs.mLockFileHandle = INVALID_HANDLE_VALUE;
#elif defined (XP_OS2)
    mLockFileHandle = rhs.mLockFileHandle;
    rhs.mLockFileHandle = -1;
#elif defined (XP_UNIX)
    mLockFileDesc = rhs.mLockFileDesc;
    rhs.mLockFileDesc = -1;
    mPidLockFileName = rhs.mPidLockFileName;
    rhs.mPidLockFileName = nsnull;
    if (mPidLockFileName)
    {
        
        PR_REMOVE_LINK(&rhs);
        PR_APPEND_LINK(this, &mPidLockList);
    }
#endif

    return *this;
}


nsProfileLock::~nsProfileLock()
{
    Unlock();
}


#if defined (XP_UNIX)

static int setupPidLockCleanup;

PRCList nsProfileLock::mPidLockList =
    PR_INIT_STATIC_CLIST(&nsProfileLock::mPidLockList);

void nsProfileLock::RemovePidLockFiles()
{
    while (!PR_CLIST_IS_EMPTY(&mPidLockList))
    {
        nsProfileLock *lock = static_cast<nsProfileLock*>(mPidLockList.next);
        lock->Unlock();
    }
}

static struct sigaction SIGHUP_oldact;
static struct sigaction SIGINT_oldact;
static struct sigaction SIGQUIT_oldact;
static struct sigaction SIGILL_oldact;
static struct sigaction SIGABRT_oldact;
static struct sigaction SIGSEGV_oldact;
static struct sigaction SIGTERM_oldact;

void nsProfileLock::FatalSignalHandler(int signo, siginfo_t *info,
                                       void *context)
{
    
    RemovePidLockFiles();

    
    struct sigaction *oldact = nsnull;

    switch (signo) {
      case SIGHUP:
        oldact = &SIGHUP_oldact;
        break;
      case SIGINT:
        oldact = &SIGINT_oldact;
        break;
      case SIGQUIT:
        oldact = &SIGQUIT_oldact;
        break;
      case SIGILL:
        oldact = &SIGILL_oldact;
        break;
      case SIGABRT:
        oldact = &SIGABRT_oldact;
        break;
      case SIGSEGV:
        oldact = &SIGSEGV_oldact;
        break;
      case SIGTERM:
        oldact = &SIGTERM_oldact;
        break;
      default:
        NS_NOTREACHED("bad signo");
        break;
    }

    if (oldact) {
        if (oldact->sa_handler == SIG_DFL) {
            
            
            sigaction(signo,oldact,NULL);

            
            

            sigset_t unblock_sigs;
            sigemptyset(&unblock_sigs);
            sigaddset(&unblock_sigs, signo);

            sigprocmask(SIG_UNBLOCK, &unblock_sigs, NULL);

            raise(signo);
        }
        else if (oldact->sa_sigaction &&
                 (oldact->sa_flags & SA_SIGINFO) == SA_SIGINFO) {
            oldact->sa_sigaction(signo, info, context);
        }
        else if (oldact->sa_handler && oldact->sa_handler != SIG_IGN)
        {
            oldact->sa_handler(signo);
        }
    }

    
    _exit(signo);
}

nsresult nsProfileLock::LockWithFcntl(const nsACString& lockFilePath)
{
    nsresult rv = NS_OK;

    mLockFileDesc = open(PromiseFlatCString(lockFilePath).get(),
                          O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (mLockFileDesc != -1)
    {
        struct flock lock;
        lock.l_start = 0;
        lock.l_len = 0; 
        lock.l_type = F_WRLCK;
        lock.l_whence = SEEK_SET;

        
        
        
        struct flock testlock = lock;
        if (fcntl(mLockFileDesc, F_GETLK, &testlock) == -1)
        {
            close(mLockFileDesc);
            mLockFileDesc = -1;
            rv = NS_ERROR_FAILURE;
        }
        else if (fcntl(mLockFileDesc, F_SETLK, &lock) == -1)
        {
            close(mLockFileDesc);
            mLockFileDesc = -1;

            
            
#ifdef DEBUG
            printf("fcntl(F_SETLK) failed. errno = %d\n", errno);
#endif
            if (errno == EAGAIN || errno == EACCES)
                rv = NS_ERROR_FILE_ACCESS_DENIED;
            else
                rv = NS_ERROR_FAILURE;
        }
        else
            mHaveLock = PR_TRUE;
    }
    else
    {
        NS_ERROR("Failed to open lock file.");
        rv = NS_ERROR_FAILURE;
    }
    return rv;
}

static PRBool IsSymlinkStaleLock(struct in_addr* aAddr, const char* aFileName,
                                 PRBool aHaveFcntlLock)
{
    
    
    char buf[1024];
    int len = readlink(aFileName, buf, sizeof buf - 1);
    if (len > 0)
    {
        buf[len] = '\0';
        char *colon = strchr(buf, ':');
        if (colon)
        {
            *colon++ = '\0';
            unsigned long addr = inet_addr(buf);
            if (addr != (unsigned long) -1)
            {
                if (colon[0] == '+' && aHaveFcntlLock) {
                    
                    
                    
                    return PR_TRUE;
                }
                    
                char *after = nsnull;
                pid_t pid = strtol(colon, &after, 0);
                if (pid != 0 && *after == '\0')
                {
                    if (addr != aAddr->s_addr)
                    {
                        
                        return PR_FALSE;
                    }
    
                    
                    
                    if (kill(pid, 0) == 0 || errno != ESRCH)
                    {
                        
                        
                        
                        
                        return PR_FALSE;
                    }
                }
            }
        }
    }
    return PR_TRUE;
}

nsresult nsProfileLock::LockWithSymlink(const nsACString& lockFilePath, PRBool aHaveFcntlLock)
{
    nsresult rv;

    struct in_addr inaddr;
    inaddr.s_addr = htonl(INADDR_LOOPBACK);

    char hostname[256];
    PRStatus status = PR_GetSystemInfo(PR_SI_HOSTNAME, hostname, sizeof hostname);
    if (status == PR_SUCCESS)
    {
        char netdbbuf[PR_NETDB_BUF_SIZE];
        PRHostEnt hostent;
        status = PR_GetHostByName(hostname, netdbbuf, sizeof netdbbuf, &hostent);
        if (status == PR_SUCCESS)
            memcpy(&inaddr, hostent.h_addr, sizeof inaddr);
    }

    char *signature =
        PR_smprintf("%s:%s%lu", inet_ntoa(inaddr), aHaveFcntlLock ? "+" : "",
                    (unsigned long)getpid());
    const nsPromiseFlatCString& flat = PromiseFlatCString(lockFilePath);
    const char *fileName = flat.get();
    int symlink_rv, symlink_errno = 0, tries = 0;

    
    while ((symlink_rv = symlink(signature, fileName)) < 0)
    {
        symlink_errno = errno;
        if (symlink_errno != EEXIST)
            break;

        if (!IsSymlinkStaleLock(&inaddr, fileName, aHaveFcntlLock))
            break;

        
        
        (void) unlink(fileName);
        if (++tries > 100)
            break;
    }

    PR_smprintf_free(signature);
    signature = nsnull;

    if (symlink_rv == 0)
    {
        
        
        rv = NS_OK;
        mHaveLock = PR_TRUE;
        mPidLockFileName = strdup(fileName);
        if (mPidLockFileName)
        {
            PR_APPEND_LINK(this, &mPidLockList);
            if (!setupPidLockCleanup++)
            {
                
                atexit(RemovePidLockFiles);

                
                
                
                if (!sDisableSignalHandling) {
                    struct sigaction act, oldact;
                    act.sa_sigaction = FatalSignalHandler;
                    act.sa_flags = SA_SIGINFO;
                    sigfillset(&act.sa_mask);

#define CATCH_SIGNAL(signame)                                           \
PR_BEGIN_MACRO                                                          \
  if (sigaction(signame, NULL, &oldact) == 0 &&                         \
      oldact.sa_handler != SIG_IGN)                                     \
  {                                                                     \
      sigaction(signame, &act, &signame##_oldact);                      \
  }                                                                     \
  PR_END_MACRO

                    CATCH_SIGNAL(SIGHUP);
                    CATCH_SIGNAL(SIGINT);
                    CATCH_SIGNAL(SIGQUIT);
                    CATCH_SIGNAL(SIGILL);
                    CATCH_SIGNAL(SIGABRT);
                    CATCH_SIGNAL(SIGSEGV);
                    CATCH_SIGNAL(SIGTERM);

#undef CATCH_SIGNAL
                }
            }
        }
    }
    else if (symlink_errno == EEXIST)
        rv = NS_ERROR_FILE_ACCESS_DENIED;
    else
    {
#ifdef DEBUG
        printf("symlink() failed. errno = %d\n", errno);
#endif
        rv = NS_ERROR_FAILURE;
    }
    return rv;
}
#endif 

nsresult nsProfileLock::Lock(nsILocalFile* aProfileDir,
                             nsIProfileUnlocker* *aUnlocker)
{
#if defined (XP_MACOSX)
    NS_NAMED_LITERAL_STRING(LOCKFILE_NAME, ".parentlock");
    NS_NAMED_LITERAL_STRING(OLD_LOCKFILE_NAME, "parent.lock");
#elif defined (XP_UNIX)
    NS_NAMED_LITERAL_STRING(OLD_LOCKFILE_NAME, "lock");
    NS_NAMED_LITERAL_STRING(LOCKFILE_NAME, ".parentlock");
#else
    NS_NAMED_LITERAL_STRING(LOCKFILE_NAME, "parent.lock");
#endif

    nsresult rv;
    if (aUnlocker)
        *aUnlocker = nsnull;

    NS_ENSURE_STATE(!mHaveLock);

    PRBool isDir;
    rv = aProfileDir->IsDirectory(&isDir);
    if (NS_FAILED(rv))
        return rv;
    if (!isDir)
        return NS_ERROR_FILE_NOT_DIRECTORY;

    nsCOMPtr<nsILocalFile> lockFile;
    rv = aProfileDir->Clone((nsIFile **)((void **)getter_AddRefs(lockFile)));
    if (NS_FAILED(rv))
        return rv;

    rv = lockFile->Append(LOCKFILE_NAME);
    if (NS_FAILED(rv))
        return rv;
        
#if defined(XP_MACOSX)
    
    
    nsCAutoString filePath;
    rv = lockFile->GetNativePath(filePath);
    if (NS_FAILED(rv))
        return rv;

    rv = LockWithFcntl(filePath);
    if (NS_FAILED(rv) && (rv != NS_ERROR_FILE_ACCESS_DENIED))
    {
        
        
        rv = LockWithSymlink(filePath, PR_FALSE);
    }
    
    if (NS_SUCCEEDED(rv))
    {
        
        
        
        
        struct LockProcessInfo
        {
            ProcessSerialNumber psn;
            unsigned long launchDate;
        };

        PRFileDesc *fd = nsnull;
        PRInt32 ioBytes;
        ProcessInfoRec processInfo;
        LockProcessInfo lockProcessInfo;

        rv = lockFile->SetLeafName(OLD_LOCKFILE_NAME);
        if (NS_FAILED(rv))
            return rv;
        rv = lockFile->OpenNSPRFileDesc(PR_RDONLY, 0, &fd);
        if (NS_SUCCEEDED(rv))
        {
            ioBytes = PR_Read(fd, &lockProcessInfo, sizeof(LockProcessInfo));
            PR_Close(fd);

            if (ioBytes == sizeof(LockProcessInfo))
            {
#ifdef __LP64__
                processInfo.processAppRef = NULL;
#else
                processInfo.processAppSpec = NULL;
#endif
                processInfo.processName = NULL;
                processInfo.processInfoLength = sizeof(ProcessInfoRec);
                if (::GetProcessInformation(&lockProcessInfo.psn, &processInfo) == noErr &&
                    processInfo.processLaunchDate == lockProcessInfo.launchDate)
                {
                    return NS_ERROR_FILE_ACCESS_DENIED;
                }
            }
            else
            {
                NS_WARNING("Could not read lock file - ignoring lock");
            }
        }
        rv = NS_OK; 
    }
#elif defined(XP_UNIX)
    nsCAutoString filePath;
    rv = lockFile->GetNativePath(filePath);
    if (NS_FAILED(rv))
        return rv;

    
    nsCOMPtr<nsIFile> oldLockFile;
    rv = aProfileDir->Clone(getter_AddRefs(oldLockFile));
    if (NS_FAILED(rv))
        return rv;
    rv = oldLockFile->Append(OLD_LOCKFILE_NAME);
    if (NS_FAILED(rv))
        return rv;
    nsCAutoString oldFilePath;
    rv = oldLockFile->GetNativePath(oldFilePath);
    if (NS_FAILED(rv))
        return rv;

    
    
    rv = LockWithFcntl(filePath);
    if (NS_SUCCEEDED(rv)) {
        
        
        
        
        rv = LockWithSymlink(oldFilePath, PR_TRUE);

        
        
        
        
        
        
        
        if (rv != NS_ERROR_FILE_ACCESS_DENIED)
            rv = NS_OK;
    }
    else if (rv != NS_ERROR_FILE_ACCESS_DENIED)
    {
        
        
        
        rv = LockWithSymlink(oldFilePath, PR_FALSE);
    }

#elif defined(XP_WIN)
    nsAutoString filePath;
    rv = lockFile->GetPath(filePath);
    if (NS_FAILED(rv))
        return rv;
#ifdef WINCE
    
    
    
    DeleteFileW(filePath.get());
#endif

    mLockFileHandle = CreateFileW(filePath.get(),
                                  GENERIC_READ | GENERIC_WRITE,
                                  0, 
                                  nsnull,
                                  OPEN_ALWAYS,
#ifndef WINCE
                                  FILE_FLAG_DELETE_ON_CLOSE,
#else
                                  FILE_ATTRIBUTE_NORMAL,
#endif
                                  nsnull);
    if (mLockFileHandle == INVALID_HANDLE_VALUE) {
        
        return NS_ERROR_FILE_ACCESS_DENIED;
    }
#elif defined(XP_OS2)
    nsCAutoString filePath;
    rv = lockFile->GetNativePath(filePath);
    if (NS_FAILED(rv))
        return rv;

    ULONG   ulAction = 0;
    APIRET  rc;
    rc = DosOpen(filePath.get(),
                  &mLockFileHandle,
                  &ulAction,
                  0,
                  FILE_NORMAL,
                  OPEN_ACTION_CREATE_IF_NEW | OPEN_ACTION_OPEN_IF_EXISTS,
                  OPEN_ACCESS_READWRITE | OPEN_SHARE_DENYREADWRITE | OPEN_FLAGS_NOINHERIT,
                  0 );
    if (rc != NO_ERROR)
    {
        mLockFileHandle = -1;
        return NS_ERROR_FILE_ACCESS_DENIED;
    }
#elif defined(VMS)
    nsCAutoString filePath;
    rv = lockFile->GetNativePath(filePath);
    if (NS_FAILED(rv))
        return rv;

    mLockFileDesc = open_noshr(filePath.get(), O_CREAT, 0666);
    if (mLockFileDesc == -1)
    {
        if ((errno == EVMSERR) && (vaxc$errno == RMS$_FLK))
        {
            return NS_ERROR_FILE_ACCESS_DENIED;
        }
        else
        {
            NS_ERROR("Failed to open lock file.");
            return NS_ERROR_FAILURE;
        }
    }
#endif

    mHaveLock = PR_TRUE;

    return rv;
}


nsresult nsProfileLock::Unlock()
{
    nsresult rv = NS_OK;

    if (mHaveLock)
    {
#if defined (XP_WIN)
        if (mLockFileHandle != INVALID_HANDLE_VALUE)
        {
            CloseHandle(mLockFileHandle);
            mLockFileHandle = INVALID_HANDLE_VALUE;
        }
#elif defined (XP_OS2)
        if (mLockFileHandle != -1)
        {
            DosClose(mLockFileHandle);
            mLockFileHandle = -1;
        }
#elif defined (XP_UNIX)
        if (mPidLockFileName)
        {
            PR_REMOVE_LINK(this);
            (void) unlink(mPidLockFileName);
            free(mPidLockFileName);
            mPidLockFileName = nsnull;
        }
        else if (mLockFileDesc != -1)
        {
            close(mLockFileDesc);
            mLockFileDesc = -1;
            
        }
#endif

        mHaveLock = PR_FALSE;
    }

    return rv;
}
