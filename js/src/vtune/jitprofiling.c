
























































#include "vtune/ittnotify_config.h"

#if ITT_PLATFORM==ITT_PLATFORM_WIN
#include <windows.h>
#pragma optimize("", off)
#else  
#include <pthread.h>
#include <dlfcn.h>
#endif 
#include <malloc.h>
#include <stdlib.h>

#include "vtune/jitprofiling.h"

static const char rcsid[] = "\n@(#) $Revision: 294150 $\n";

#define DLL_ENVIRONMENT_VAR             "VS_PROFILER"

#ifndef NEW_DLL_ENVIRONMENT_VAR
#if ITT_ARCH==ITT_ARCH_IA32
#define NEW_DLL_ENVIRONMENT_VAR	        "INTEL_JIT_PROFILER32"
#else
#define NEW_DLL_ENVIRONMENT_VAR	        "INTEL_JIT_PROFILER64"
#endif
#endif 

#if ITT_PLATFORM==ITT_PLATFORM_WIN
#define DEFAULT_DLLNAME                 "JitPI.dll"
HINSTANCE m_libHandle = NULL;
#else  
#define DEFAULT_DLLNAME                 "libJitPI.so"
void* m_libHandle = NULL;
#endif 


#define ANDROID_JIT_AGENT_PATH  "/data/intel/libittnotify.so"


typedef unsigned int(*TPInitialize)(void);
static TPInitialize FUNC_Initialize=NULL;

typedef unsigned int(*TPNotify)(unsigned int, void*);
static TPNotify FUNC_NotifyEvent=NULL;

static iJIT_IsProfilingActiveFlags executionMode = iJIT_NOTHING_RUNNING;








 
static int loadiJIT_Funcs(void);


static int iJIT_DLL_is_missing = 0;









#if ITT_PLATFORM==ITT_PLATFORM_WIN
static DWORD threadLocalStorageHandle = 0;
#else  
static pthread_key_t threadLocalStorageHandle = (pthread_key_t)0;
#endif 

#define INIT_TOP_Stack 10000

typedef struct 
{
    unsigned int TopStack;
    unsigned int CurrentStack;
} ThreadStack, *pThreadStack;













ITT_EXTERN_C int JITAPI 
iJIT_NotifyEvent(iJIT_JVM_EVENT event_type, void *EventSpecificData)
{
    int ReturnValue;

    
















    



    if (!FUNC_NotifyEvent) 
    {
        if (iJIT_DLL_is_missing) 
            return 0;

        
        if (!loadiJIT_Funcs()) 
            return 0;

        
    }

    


    if ((event_type == iJVM_EVENT_TYPE_ENTER_NIDS || 
         event_type == iJVM_EVENT_TYPE_LEAVE_NIDS) &&
        (executionMode != iJIT_CALLGRAPH_ON))
    {
        return 0;
    }
    



    if (event_type == iJVM_EVENT_TYPE_ENTER_NIDS)
    {
#if ITT_PLATFORM==ITT_PLATFORM_WIN
        pThreadStack threadStack = 
            (pThreadStack)TlsGetValue (threadLocalStorageHandle);
#else  
        pThreadStack threadStack = 
            (pThreadStack)pthread_getspecific(threadLocalStorageHandle);
#endif 

        
        if ( ((piJIT_Method_NIDS) EventSpecificData)->method_id <= 999 )
            return 0;

        if (!threadStack)
        {
            
            threadStack = (pThreadStack) calloc (sizeof(ThreadStack), 1);
            if (!threadStack)
                return 0;
            threadStack->TopStack = INIT_TOP_Stack;
            threadStack->CurrentStack = INIT_TOP_Stack;
#if ITT_PLATFORM==ITT_PLATFORM_WIN
            TlsSetValue(threadLocalStorageHandle,(void*)threadStack);
#else  
            pthread_setspecific(threadLocalStorageHandle,(void*)threadStack);
#endif 
        }

        
        ((piJIT_Method_NIDS) EventSpecificData)->stack_id = 
            (threadStack->CurrentStack)--;
    }

    





    if (event_type == iJVM_EVENT_TYPE_LEAVE_NIDS)
    {
#if ITT_PLATFORM==ITT_PLATFORM_WIN
        pThreadStack threadStack = 
           (pThreadStack)TlsGetValue (threadLocalStorageHandle);
#else  
        pThreadStack threadStack = 
            (pThreadStack)pthread_getspecific(threadLocalStorageHandle);
#endif 

        
        if ( ((piJIT_Method_NIDS) EventSpecificData)->method_id <= 999 )
            return 0;

        if (!threadStack)
        {
            
            exit (1);
        }

        ((piJIT_Method_NIDS) EventSpecificData)->stack_id = 
            ++(threadStack->CurrentStack) + 1;

        if (((piJIT_Method_NIDS) EventSpecificData)->stack_id 
               > threadStack->TopStack)
            ((piJIT_Method_NIDS) EventSpecificData)->stack_id = 
                (unsigned int)-1;
    }

    if (event_type == iJVM_EVENT_TYPE_METHOD_LOAD_FINISHED)
    {
        
        if ( ((piJIT_Method_Load) EventSpecificData)->method_id <= 999 )
            return 0;
    }
    else if (event_type == iJVM_EVENT_TYPE_METHOD_LOAD_FINISHED_V2)
    {
        
        if ( ((piJIT_Method_Load_V2) EventSpecificData)->method_id <= 999 )
            return 0;
    }

    ReturnValue = (int)FUNC_NotifyEvent(event_type, EventSpecificData);

    return ReturnValue;
}


ITT_EXTERN_C void JITAPI 
iJIT_RegisterCallbackEx(void *userdata, iJIT_ModeChangedEx 
                        NewModeCallBackFuncEx) 
{
    
    if (iJIT_DLL_is_missing || !loadiJIT_Funcs())
    {
        
        NewModeCallBackFuncEx(userdata, iJIT_NO_NOTIFICATIONS);  
        
        return;
    }
    
}





ITT_EXTERN_C iJIT_IsProfilingActiveFlags JITAPI iJIT_IsProfilingActive()
{
    if (!iJIT_DLL_is_missing)
    {
        loadiJIT_Funcs();
    }

    return executionMode;
}





 
static int loadiJIT_Funcs()
{
    static int bDllWasLoaded = 0;
    char *dllName = (char*)rcsid; 
#if ITT_PLATFORM==ITT_PLATFORM_WIN
    DWORD dNameLength = 0;
#endif 

    if(bDllWasLoaded)
    {
        
        return 1;
    }

    
    iJIT_DLL_is_missing = 1;
    FUNC_NotifyEvent = NULL;

    if (m_libHandle) 
    {
#if ITT_PLATFORM==ITT_PLATFORM_WIN
        FreeLibrary(m_libHandle);
#else  
        dlclose(m_libHandle);
#endif 
        m_libHandle = NULL;
    }

    
#if ITT_PLATFORM==ITT_PLATFORM_WIN
    dNameLength = GetEnvironmentVariableA(NEW_DLL_ENVIRONMENT_VAR, NULL, 0);
    if (dNameLength)
    {
        DWORD envret = 0;
        dllName = (char*)malloc(sizeof(char) * (dNameLength + 1));
        envret = GetEnvironmentVariableA(NEW_DLL_ENVIRONMENT_VAR, 
                                         dllName, dNameLength);
        if (envret)
        {
            
            m_libHandle = LoadLibraryExA(dllName, 
                                         NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
        }
        free(dllName);
    } else {
        
        dNameLength = GetEnvironmentVariableA(DLL_ENVIRONMENT_VAR, NULL, 0);
        if (dNameLength)
        {
            DWORD envret = 0;
            dllName = (char*)malloc(sizeof(char) * (dNameLength + 1));
            envret = GetEnvironmentVariableA(DLL_ENVIRONMENT_VAR, 
                                             dllName, dNameLength);
            if (envret)
            {
                
                m_libHandle = LoadLibraryA(dllName);
            }
            free(dllName);
        }
    }
#else  
    dllName = getenv(NEW_DLL_ENVIRONMENT_VAR);
    if (!dllName)
        dllName = getenv(DLL_ENVIRONMENT_VAR);
#ifdef ANDROID
    if (!dllName)
        dllName = ANDROID_JIT_AGENT_PATH;
#endif
    if (dllName)
    {
        
        m_libHandle = dlopen(dllName, RTLD_LAZY);
    }
#endif 

    if (!m_libHandle)
    {
#if ITT_PLATFORM==ITT_PLATFORM_WIN
        m_libHandle = LoadLibraryA(DEFAULT_DLLNAME);
#else  
        m_libHandle = dlopen(DEFAULT_DLLNAME, RTLD_LAZY);
#endif 
    }

    
    if (!m_libHandle)
    {
        iJIT_DLL_is_missing = 1; 


        return 0;
    }

#if ITT_PLATFORM==ITT_PLATFORM_WIN
    FUNC_NotifyEvent = (TPNotify)GetProcAddress(m_libHandle, "NotifyEvent");
#else  
    FUNC_NotifyEvent = (TPNotify)dlsym(m_libHandle, "NotifyEvent");
#endif 
    if (!FUNC_NotifyEvent) 
    {
        FUNC_Initialize = NULL;
        return 0;
    }

#if ITT_PLATFORM==ITT_PLATFORM_WIN
    FUNC_Initialize = (TPInitialize)GetProcAddress(m_libHandle, "Initialize");
#else  
    FUNC_Initialize = (TPInitialize)dlsym(m_libHandle, "Initialize");
#endif 
    if (!FUNC_Initialize) 
    {
        FUNC_NotifyEvent = NULL;
        return 0;
    }

    executionMode = (iJIT_IsProfilingActiveFlags)FUNC_Initialize();

    bDllWasLoaded = 1;
    iJIT_DLL_is_missing = 0; 

    



    if ( executionMode == iJIT_CALLGRAPH_ON )
    {
        
        if (!threadLocalStorageHandle)
#if ITT_PLATFORM==ITT_PLATFORM_WIN
            threadLocalStorageHandle = TlsAlloc();
#else  
        pthread_key_create(&threadLocalStorageHandle, NULL);
#endif 
    }

    return 1;
}





ITT_EXTERN_C void JITAPI FinalizeThread()
{
    if (threadLocalStorageHandle)
    {
#if ITT_PLATFORM==ITT_PLATFORM_WIN
        pThreadStack threadStack = 
            (pThreadStack)TlsGetValue (threadLocalStorageHandle);
#else  
        pThreadStack threadStack = 
            (pThreadStack)pthread_getspecific(threadLocalStorageHandle);
#endif 
        if (threadStack)
        {
            free (threadStack);
            threadStack = NULL;
#if ITT_PLATFORM==ITT_PLATFORM_WIN
            TlsSetValue (threadLocalStorageHandle, threadStack);
#else  
            pthread_setspecific(threadLocalStorageHandle, threadStack);
#endif 
        }
    }
}





ITT_EXTERN_C void JITAPI FinalizeProcess()
{
    if (m_libHandle) 
    {
#if ITT_PLATFORM==ITT_PLATFORM_WIN
        FreeLibrary(m_libHandle);
#else  
        dlclose(m_libHandle);
#endif 
        m_libHandle = NULL;
    }

    if (threadLocalStorageHandle)
#if ITT_PLATFORM==ITT_PLATFORM_WIN
        TlsFree (threadLocalStorageHandle);
#else  
    pthread_key_delete(threadLocalStorageHandle);
#endif 
}






ITT_EXTERN_C unsigned int JITAPI iJIT_GetNewMethodID()
{
    static unsigned int methodID = 0x100000;

    if (methodID == 0)
        return 0;  

    return methodID++;
}
