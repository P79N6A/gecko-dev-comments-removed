
















#ifndef __UCLN_IMP_H__
#define __UCLN_IMP_H__

#include "ucln.h"
#include <stdlib.h>

















#if !UCLN_NO_AUTO_CLEANUP

















#ifdef UCLN_TYPE_IS_COMMON
#   define UCLN_CLEAN_ME_UP u_cleanup()
#else
#   define UCLN_CLEAN_ME_UP ucln_cleanupOne(UCLN_TYPE)
#endif


#if defined(UCLN_AUTO_LOCAL)






#include "ucln_local_hook.c"

#elif defined(UCLN_AUTO_ATEXIT)




static UBool gAutoCleanRegistered = FALSE;

static void ucln_atexit_handler()
{
    UCLN_CLEAN_ME_UP;
}

static void ucln_registerAutomaticCleanup()
{
    if(!gAutoCleanRegistered) {
        gAutoCleanRegistered = TRUE;
        atexit(&ucln_atexit_handler);
    }
}

static void ucln_unRegisterAutomaticCleanup () {
}


#elif defined (UCLN_FINI)





U_CAPI void U_EXPORT2 UCLN_FINI (void);

U_CAPI void U_EXPORT2 UCLN_FINI ()
{
    
     UCLN_CLEAN_ME_UP;
}


#elif U_PLATFORM_HAS_WIN32_API








#   define WIN32_LEAN_AND_MEAN
#   define VC_EXTRALEAN
#   define NOUSER
#   define NOSERVICE
#   define NOIME
#   define NOMCX
#   include <windows.h>



BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    BOOL status = TRUE;

    switch(fdwReason) {
        case DLL_PROCESS_ATTACH:
             
            
            break;

        case DLL_PROCESS_DETACH:
            

            UCLN_CLEAN_ME_UP;

            break;

        case DLL_THREAD_ATTACH:
            
            
            break;

        case DLL_THREAD_DETACH:
            
            
            break;

    }
    return status;
}

#elif defined(__GNUC__)

static void ucln_destructor()   __attribute__((destructor)) ;

static void ucln_destructor() 
{
    UCLN_CLEAN_ME_UP;
}

#endif

#endif 

#else
#error This file can only be included once.
#endif
