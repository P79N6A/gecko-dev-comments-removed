














































#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsMemory.h"
#include "nsProcess.h"
#include "prtypes.h"
#include "prio.h"
#include "prenv.h"
#include "nsCRT.h"
#include "nsAutoLock.h"
#include "nsThreadUtils.h"
#include "nsIObserverService.h"

#include <stdlib.h>

#if defined(PROCESSMODEL_WINAPI)
#include "prmem.h"
#include "nsString.h"
#include "nsLiteralString.h"
#include "nsReadableUtils.h"
#else
#include <sys/types.h>
#include <signal.h>
#endif

#ifdef WINCE
#include <windows.h> 
#include "prmem.h"
#endif




NS_IMPL_THREADSAFE_ISUPPORTS3(nsProcess, nsIProcess,
                                         nsIProcess2,
                                         nsIObserver)


nsProcess::nsProcess()
    : mThread(nsnull),
      mLock(PR_NewLock()),
      mShutdown(PR_FALSE),
      mPid(-1),
      mObserver(nsnull),
      mWeakObserver(nsnull),
      mExitValue(-1),
      mProcess(nsnull)
{
}


nsProcess::~nsProcess()
{
    PR_DestroyLock(mLock);
}

NS_IMETHODIMP
nsProcess::Init(nsIFile* executable)
{
    if (mExecutable)
        return NS_ERROR_ALREADY_INITIALIZED;

    NS_ENSURE_ARG_POINTER(executable);
    PRBool isFile;

    
    nsresult rv = executable->IsFile(&isFile);
    if (NS_FAILED(rv)) return rv;
    if (!isFile)
        return NS_ERROR_FAILURE;

    
    mExecutable = executable;
    
#ifdef XP_WIN 
    rv = mExecutable->GetNativeTarget(mTargetPath);
    if (NS_FAILED(rv) || mTargetPath.IsEmpty() )
#endif
        rv = mExecutable->GetNativePath(mTargetPath);

    return rv;
}


#if defined(XP_WIN)

static int assembleCmdLine(char *const *argv, PRUnichar **wideCmdLine)
{
    char *const *arg;
    char *p, *q, *cmdLine;
    int cmdLineSize;
    int numBackslashes;
    int i;
    int argNeedQuotes;

    


    cmdLineSize = 0;
    for (arg = argv; *arg; arg++) {
        






        cmdLineSize += 2 * strlen(*arg)  
                + 2                      
                + 1;                     
    }
    p = cmdLine = (char *) PR_MALLOC(cmdLineSize*sizeof(char));
    if (p == NULL) {
        return -1;
    }

    for (arg = argv; *arg; arg++) {
        
        if (arg != argv) {
            *p++ = ' '; 
        }
        q = *arg;
        numBackslashes = 0;
        argNeedQuotes = 0;

        
        if (strpbrk(*arg, " \f\n\r\t\v")) {
            argNeedQuotes = 1;
        }

        if (argNeedQuotes) {
            *p++ = '"';
        }
        while (*q) {
            if (*q == '\\') {
                numBackslashes++;
                q++;
            } else if (*q == '"') {
                if (numBackslashes) {
                    



                    for (i = 0; i < 2 * numBackslashes; i++) {
                        *p++ = '\\';
                    }
                    numBackslashes = 0;
                }
                
                *p++ = '\\';
                *p++ = *q++;
            } else {
                if (numBackslashes) {
                    



                    for (i = 0; i < numBackslashes; i++) {
                        *p++ = '\\';
                    }
                    numBackslashes = 0;
                }
                *p++ = *q++;
            }
        }

        
        if (numBackslashes) {
            



            if (argNeedQuotes) {
                numBackslashes *= 2;
            }
            for (i = 0; i < numBackslashes; i++) {
                *p++ = '\\';
            }
        }
        if (argNeedQuotes) {
            *p++ = '"';
        }
    } 

    *p = '\0';
    PRInt32 numChars = MultiByteToWideChar(CP_ACP, 0, cmdLine, -1, NULL, 0); 
    *wideCmdLine = (PRUnichar *) PR_MALLOC(numChars*sizeof(PRUnichar));
    MultiByteToWideChar(CP_ACP, 0, cmdLine, -1, *wideCmdLine, numChars); 
    PR_Free(cmdLine);
    return 0;
}
#endif

void PR_CALLBACK nsProcess::Monitor(void *arg)
{
    nsRefPtr<nsProcess> process = dont_AddRef(static_cast<nsProcess*>(arg));
#if defined(PROCESSMODEL_WINAPI)
    DWORD dwRetVal;
    unsigned long exitCode = -1;

    dwRetVal = WaitForSingleObject(process->mProcess, INFINITE);
    if (dwRetVal != WAIT_FAILED) {
        if (GetExitCodeProcess(process->mProcess, &exitCode) == FALSE)
            exitCode = -1;
    }

    
    {
        nsAutoLock lock(process->mLock);
        CloseHandle(process->mProcess);
        process->mProcess = NULL;
        process->mExitValue = exitCode;
        if (process->mShutdown)
            return;
    }
#else
    PRInt32 exitCode = -1;
    if (PR_WaitProcess(process->mProcess, &exitCode) != PR_SUCCESS)
        exitCode = -1;

    
    {
        nsAutoLock lock(process->mLock);
        process->mProcess = nsnull;
        process->mExitValue = exitCode;
        if (process->mShutdown)
            return;
    }
#endif

    
    
    if (NS_IsMainThread()) {
        process->ProcessComplete();
    }
    else {
        nsCOMPtr<nsIRunnable> event = new nsRunnableMethod<nsProcess>(process, &nsProcess::ProcessComplete);
        NS_DispatchToMainThread(event);
    }
}

void nsProcess::ProcessComplete()
{
    if (mThread) {
        nsCOMPtr<nsIObserverService> os = do_GetService("@mozilla.org/observer-service;1");
        if (os)
            os->RemoveObserver(this, "xpcom-shutdown");
        PR_JoinThread(mThread);
        mThread = nsnull;
    }

    char* topic;
    if (mExitValue < 0)
        topic = "process-failed";
    else
        topic = "process-finished";

    mPid = -1;
    nsCOMPtr<nsIObserver> observer;
    if (mWeakObserver)
        observer = do_QueryReferent(mWeakObserver);
    else if (mObserver)
        observer = mObserver;
    mObserver = nsnull;
    mWeakObserver = nsnull;

    if (observer)
        observer->Observe(NS_ISUPPORTS_CAST(nsIProcess*, this), topic, nsnull);
}


NS_IMETHODIMP  
nsProcess::Run(PRBool blocking, const char **args, PRUint32 count)
{
    return RunProcess(blocking, args, count, nsnull, PR_FALSE);
}


NS_IMETHODIMP  
nsProcess::RunAsync(const char **args, PRUint32 count,
                    nsIObserver* observer, PRBool holdWeak)
{
    return RunProcess(PR_FALSE, args, count, observer, holdWeak);
}

NS_IMETHODIMP  
nsProcess::RunProcess(PRBool blocking, const char **args, PRUint32 count,
                      nsIObserver* observer, PRBool holdWeak)
{
    NS_ENSURE_TRUE(mExecutable, NS_ERROR_NOT_INITIALIZED);
    NS_ENSURE_FALSE(mThread, NS_ERROR_ALREADY_INITIALIZED);

    if (observer) {
        if (holdWeak) {
            mWeakObserver = do_GetWeakReference(observer);
            if (!mWeakObserver)
                return NS_NOINTERFACE;
        }
        else {
            mObserver = observer;
        }
    }

    mExitValue = -1;
    mPid = -1;

    
    
    
    char **my_argv = NULL;
    my_argv = (char **)nsMemory::Alloc(sizeof(char *) * (count + 2) );
    if (!my_argv) {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    
    PRUint32 i;
    for (i=0; i < count; i++) {
        my_argv[i+1] = const_cast<char*>(args[i]);
    }
    
    my_argv[0] = mTargetPath.BeginWriting();
    
    my_argv[count+1] = NULL;

#if defined(PROCESSMODEL_WINAPI)
    BOOL retVal;
    PRUnichar *cmdLine;

    if (count > 0 && assembleCmdLine(my_argv + 1, &cmdLine) == -1) {
        nsMemory::Free(my_argv);
        return NS_ERROR_FILE_EXECUTION_FAILED;    
    }

    



    PRInt32 numChars = MultiByteToWideChar(CP_ACP, 0, my_argv[0], -1, NULL, 0); 
    PRUnichar* wideFile = (PRUnichar *) PR_MALLOC(numChars * sizeof(PRUnichar));
    MultiByteToWideChar(CP_ACP, 0, my_argv[0], -1, wideFile, numChars); 

    nsMemory::Free(my_argv);

    SHELLEXECUTEINFOW sinfo;
    memset(&sinfo, 0, sizeof(SHELLEXECUTEINFOW));
    sinfo.cbSize = sizeof(SHELLEXECUTEINFOW);
    sinfo.hwnd   = NULL;
    sinfo.lpFile = wideFile;
    sinfo.nShow  = SW_SHOWNORMAL;
    sinfo.fMask  = SEE_MASK_FLAG_DDEWAIT |
                   SEE_MASK_NO_CONSOLE |
                   SEE_MASK_NOCLOSEPROCESS;

    if (count > 0)
        sinfo.lpParameters = cmdLine;

    retVal = ShellExecuteExW(&sinfo);
    if (!retVal) {
        return NS_ERROR_FILE_EXECUTION_FAILED;
    }

    mProcess = sinfo.hProcess;

    PR_Free(wideFile);
    if (count > 0)
        PR_Free( cmdLine );

    HMODULE kernelDLL = ::LoadLibraryW(L"kernel32.dll");
    if (kernelDLL) {
        GetProcessIdPtr getProcessId = (GetProcessIdPtr)GetProcAddress(kernelDLL, "GetProcessId");
        if (getProcessId)
            mPid = getProcessId(mProcess);
        FreeLibrary(kernelDLL);
    }

#else 
    
    mProcess = PR_CreateProcess(mTargetPath.get(), my_argv, NULL, NULL);
    nsMemory::Free(my_argv);
    if (!mProcess)
        return NS_ERROR_FAILURE;

#if !defined WINCE
    struct MYProcess {
        PRUint32 pid;
    };
    MYProcess* ptrProc = (MYProcess *) mProcess;
    mPid = ptrProc->pid;
#endif
#endif

    NS_ADDREF_THIS();
    if (blocking) {
        Monitor(this);
        if (mExitValue < 0)
            return NS_ERROR_FILE_EXECUTION_FAILED;
    }
    else {
        mThread = PR_CreateThread(PR_SYSTEM_THREAD, Monitor, this,
                                  PR_PRIORITY_NORMAL, PR_LOCAL_THREAD,
                                  PR_JOINABLE_THREAD, 0);
        if (!mThread) {
            NS_RELEASE_THIS();
            return NS_ERROR_FAILURE;
        }

        
        nsCOMPtr<nsIObserverService> os = do_GetService("@mozilla.org/observer-service;1");
        if (os)
            os->AddObserver(this, "xpcom-shutdown", PR_FALSE);
    }

    return NS_OK;
}

NS_IMETHODIMP nsProcess::GetIsRunning(PRBool *aIsRunning)
{
    if (mThread)
        *aIsRunning = PR_TRUE;
    else
        *aIsRunning = PR_FALSE;

    return NS_OK;
}

NS_IMETHODIMP nsProcess::InitWithPid(PRUint32 pid)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsProcess::GetLocation(nsIFile** aLocation)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsProcess::GetPid(PRUint32 *aPid)
{
    if (!mThread)
        return NS_ERROR_FAILURE;
    if (mPid < 0)
        return NS_ERROR_NOT_IMPLEMENTED;
    *aPid = mPid;
    return NS_OK;
}

NS_IMETHODIMP
nsProcess::GetProcessName(char** aProcessName)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsProcess::GetProcessSignature(PRUint32 *aProcessSignature)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsProcess::Kill()
{
    if (!mThread)
        return NS_ERROR_FAILURE;

    {
        nsAutoLock lock(mLock);
#if defined(PROCESSMODEL_WINAPI)
        if (TerminateProcess(mProcess, NULL) == 0)
            return NS_ERROR_FAILURE;
#else
        if (PR_KillProcess(mProcess) != PR_SUCCESS)
            return NS_ERROR_FAILURE;
#endif
    }

    
    
    nsCOMPtr<nsIObserverService> os = do_GetService("@mozilla.org/observer-service;1");
    if (os)
        os->RemoveObserver(this, "xpcom-shutdown");
    PR_JoinThread(mThread);
    mThread = nsnull;

    return NS_OK;
}

NS_IMETHODIMP
nsProcess::GetExitValue(PRInt32 *aExitValue)
{
    nsAutoLock lock(mLock);

    *aExitValue = mExitValue;
    
    return NS_OK;
}

NS_IMETHODIMP
nsProcess::Observe(nsISupports* subject, const char* topic, const PRUnichar* data)
{
    
    if (mThread) {
        nsCOMPtr<nsIObserverService> os = do_GetService("@mozilla.org/observer-service;1");
        if (os)
            os->RemoveObserver(this, "xpcom-shutdown");
        mThread = nsnull;
    }

    mObserver = nsnull;
    mWeakObserver = nsnull;

    nsAutoLock lock(mLock);
    mShutdown = PR_TRUE;

    return NS_OK;
}
