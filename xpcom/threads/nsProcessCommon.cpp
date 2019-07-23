














































#include "nsCOMPtr.h"
#include "nsMemory.h"
#include "nsProcess.h"
#include "prtypes.h"
#include "prio.h"
#include "prenv.h"
#include "nsCRT.h"

#include <stdlib.h>

#if defined(XP_WIN)
#include "prmem.h"
#include "nsString.h"
#include "nsLiteralString.h"
#include "nsReadableUtils.h"
#include <windows.h>
#else
#include <sys/types.h>
#include <signal.h>
#endif




NS_IMPL_ISUPPORTS1(nsProcess, nsIProcess)


nsProcess::nsProcess()
    : mExitValue(-1),
      mProcess(nsnull)
{
#if defined(PROCESSMODEL_WINAPI)
    procInfo.dwProcessId = nsnull;
#endif
}


nsProcess::~nsProcess()
{
#if defined(PROCESSMODEL_WINAPI)
    if (procInfo.dwProcessId) {
        CloseHandle( procInfo.hProcess );
        CloseHandle( procInfo.hThread );
    }
#else
    if (mProcess) 
        PR_DetachProcess(mProcess);
#endif
}

NS_IMETHODIMP
nsProcess::Init(nsIFile* executable)
{
    
#if defined(PROCESSMODEL_WINAPI)
    if (procInfo.dwProcessId)
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
    STARTUPINFOW startupInfo;
    BOOL retVal;
    PRUnichar *cmdLine;

    if (assembleCmdLine(my_argv, &cmdLine) == -1) {
        nsMemory::Free(my_argv);
        return NS_ERROR_FILE_EXECUTION_FAILED;    
    }

    ZeroMemory(&startupInfo, sizeof(startupInfo));
    startupInfo.cb = sizeof(startupInfo);

    





    retVal = CreateProcessW(NULL,
                            
                            cmdLine,
                            NULL,  

                            NULL,  

                            FALSE,  
                            CREATE_NO_WINDOW, 
                            NULL,  
                            NULL,  
                            &startupInfo,
                            &procInfo
                           );
    PR_Free( cmdLine );
    if (blocking) {
 
        
        
        

        if ( retVal == TRUE ) {
            DWORD dwRetVal;
            unsigned long exitCode;

            dwRetVal = WaitForSingleObject(procInfo.hProcess, INFINITE);
            if (dwRetVal == WAIT_FAILED) {
                nsMemory::Free(my_argv);
                return NS_ERROR_FAILURE;
            }
            if (GetExitCodeProcess(procInfo.hProcess, &exitCode) == FALSE) {
                mExitValue = exitCode;
                nsMemory::Free(my_argv);
               return NS_ERROR_FAILURE;
            }
            mExitValue = exitCode;
            CloseHandle(procInfo.hProcess);
            CloseHandle(procInfo.hThread);
            procInfo.dwProcessId = nsnull;
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

NS_IMETHODIMP nsProcess::GetIsRunning(PRUint32 *aIsRunning)
{
#if defined(PROCESSMODEL_WINAPI)
    DWORD ec = 0;
    BOOL br = GetExitCodeProcess(procInfo.hProcess, &ec);
    if (!br) {
        aIsRunning = 0;
        return NS_OK;
    }
    *aIsRunning = (ec == STILL_ACTIVE ? 1 : 0); 
    return NS_OK;
#elif defined WINCE
    return NS_ERROR_NOT_IMPLEMENTED;   
#else
    PRUint32 pid;
    GetPid(&pid);
    if (pid)
        *aIsRunning = (kill(pid, 0) != -1) ? 1 : 0;
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
    if (!procInfo.dwProcessId)
        return NS_ERROR_FAILURE;
    *aPid = procInfo.dwProcessId;
    return NS_OK;
#elif defined WINCE
    return NS_ERROR_NOT_IMPLEMENTED;
#else
    if (!mProcess) {
        *aPid = 0;
        return NS_ERROR_FAILURE;
    }
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
    nsresult rv = NS_OK;
#if defined(PROCESSMODEL_WINAPI)
    if ( TerminateProcess(procInfo.hProcess, NULL) == 0 ) {
        rv = NS_ERROR_FAILURE;
    }
    else {
        CloseHandle( procInfo.hProcess );
        procInfo.dwProcessId = nsnull;
    }
#else
    if (mProcess)
        rv = PR_KillProcess(mProcess) == PR_SUCCESS ? NS_OK : NS_ERROR_FAILURE;
    if (rv == NS_OK)
        mProcess = nsnull;
#endif  
    return rv;
}

NS_IMETHODIMP
nsProcess::GetExitValue(PRInt32 *aExitValue)
{
    *aExitValue = mExitValue;
    
    return NS_OK;
}
