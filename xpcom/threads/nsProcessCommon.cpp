














































#include "nsCOMPtr.h"
#include "nsMemory.h"
#include "nsProcess.h"
#include "prtypes.h"
#include "prio.h"
#include "prenv.h"
#include "nsCRT.h"

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




NS_IMPL_ISUPPORTS1(nsProcess, nsIProcess)


nsProcess::nsProcess()
    : mExitValue(-1),
      mProcess(nsnull)
{
}


nsProcess::~nsProcess()
{
#if defined(PROCESSMODEL_WINAPI)
    if (mProcess)
        CloseHandle(mProcess);
#else
    if (mProcess) 
        PR_DetachProcess(mProcess);
#endif
}

NS_IMETHODIMP
nsProcess::Init(nsIFile* executable)
{
    
#if defined(PROCESSMODEL_WINAPI)
    if (mProcess)
        return NS_ERROR_ALREADY_INITIALIZED;
#else
    if (mProcess)
        return NS_ERROR_ALREADY_INITIALIZED;
#endif    

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


NS_IMETHODIMP  
nsProcess::Run(PRBool blocking, const char **args, PRUint32 count)
{
    NS_ENSURE_TRUE(mExecutable, NS_ERROR_NOT_INITIALIZED);
    PRStatus status = PR_SUCCESS;
    mExitValue = -1;

    
    
    
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
    mProcess = sinfo.hProcess;

    PR_Free(wideFile);
    if (count > 0)
        PR_Free( cmdLine );

    if (blocking) {

        
        
        

        if ( retVal == TRUE ) {
            DWORD dwRetVal;
            unsigned long exitCode;

            dwRetVal = WaitForSingleObject(mProcess, INFINITE);
            if (dwRetVal == WAIT_FAILED) {
                nsMemory::Free(my_argv);
                return NS_ERROR_FAILURE;
            }
            if (GetExitCodeProcess(mProcess, &exitCode) == FALSE) {
                mExitValue = exitCode;
                nsMemory::Free(my_argv);
                return NS_ERROR_FAILURE;
            }
            mExitValue = exitCode;
            CloseHandle(mProcess);
            mProcess = NULL;
        }
        else
            status = PR_FAILURE;
    } 
    else {

        

        if (retVal == TRUE) 
            status = PR_SUCCESS;
        else
            status = PR_FAILURE;
    }

#else 
    
    mProcess = PR_CreateProcess(mTargetPath.get(), my_argv, NULL, NULL);
    if (mProcess) {
        status = PR_SUCCESS;
        if (blocking) {
            status = PR_WaitProcess(mProcess, &mExitValue);
            mProcess = nsnull;
        } 
    }
#endif

    
    nsMemory::Free(my_argv);

    if (status != PR_SUCCESS)
        return NS_ERROR_FILE_EXECUTION_FAILED;

    return NS_OK;
}

NS_IMETHODIMP nsProcess::GetIsRunning(PRBool *aIsRunning)
{
#if defined(PROCESSMODEL_WINAPI)
    if (!mProcess) {
        *aIsRunning = PR_FALSE;
        return NS_OK;
    }
    DWORD ec = 0;
    BOOL br = GetExitCodeProcess(mProcess, &ec);
    if (!br)
        return NS_ERROR_FAILURE;
    if (ec == STILL_ACTIVE) {
        *aIsRunning = PR_TRUE;
    }
    else {
        *aIsRunning = PR_FALSE;
        mExitValue = ec;
        CloseHandle(mProcess);
        mProcess = NULL;
    }
    return NS_OK;
#elif defined WINCE
    return NS_ERROR_NOT_IMPLEMENTED;   
#else
    if (!mProcess) {
        *aIsRunning = PR_FALSE;
        return NS_OK;
    }
    PRUint32 pid;
    nsresult rv = GetPid(&pid);
    NS_ENSURE_SUCCESS(rv, rv);
    if (pid)
        *aIsRunning = (kill(pid, 0) != -1) ? PR_TRUE : PR_FALSE;
    return NS_OK;        
#endif
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
#if defined(PROCESSMODEL_WINAPI)
    if (!mProcess)
        return NS_ERROR_FAILURE;
    HMODULE kernelDLL = ::LoadLibraryW(L"kernel32.dll");
    if (!kernelDLL)
        return NS_ERROR_NOT_IMPLEMENTED;
    GetProcessIdPtr getProcessId = (GetProcessIdPtr)GetProcAddress(kernelDLL, "GetProcessId");
    if (!getProcessId) {
        FreeLibrary(kernelDLL);
        return NS_ERROR_NOT_IMPLEMENTED;
    }
    *aPid = getProcessId(mProcess);
    FreeLibrary(kernelDLL);
    return NS_OK;
#elif defined WINCE
    return NS_ERROR_NOT_IMPLEMENTED;
#else
    if (!mProcess)
        return NS_ERROR_FAILURE;

    struct MYProcess {
        PRUint32 pid;
    };
    MYProcess* ptrProc = (MYProcess *) mProcess;
    *aPid = ptrProc->pid;
    return NS_OK;
#endif
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
#if defined(PROCESSMODEL_WINAPI)
    if (!mProcess)
        return NS_ERROR_NOT_INITIALIZED;

    if ( TerminateProcess(mProcess, NULL) == 0 )
        return NS_ERROR_FAILURE;

    CloseHandle( mProcess );
    mProcess = NULL;
#else
    if (!mProcess)
        return NS_ERROR_NOT_INITIALIZED;

    if (PR_KillProcess(mProcess) != PR_SUCCESS)
        return NS_ERROR_FAILURE;

    mProcess = nsnull;
#endif  
    return NS_OK;
}

NS_IMETHODIMP
nsProcess::GetExitValue(PRInt32 *aExitValue)
{
#if defined(PROCESSMODEL_WINAPI)
    if (mProcess) {
        DWORD ec = 0;
        BOOL br = GetExitCodeProcess(mProcess, &ec);
        if (!br)
            return NS_ERROR_FAILURE;
        
        if (ec != STILL_ACTIVE) {
            mExitValue = ec;
            CloseHandle(mProcess);
            mProcess = NULL;
        }
    }
#endif
    *aExitValue = mExitValue;
    
    return NS_OK;
}
