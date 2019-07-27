





#if defined(hpux)
# ifndef _INCLUDE_POSIX_SOURCE
#  define _INCLUDE_POSIX_SOURCE
# endif
#endif


#ifndef __EXTENSIONS__
#define __EXTENSIONS__
#endif



#include "uposixdefs.h"

#include "simplethread.h"

#include "unicode/utypes.h"
#include "unicode/ustring.h"
#include "umutex.h"
#include "cmemory.h"
#include "cstring.h"
#include "uparse.h"
#include "unicode/resbund.h"
#include "unicode/udata.h"
#include "unicode/uloc.h"
#include "unicode/locid.h"
#include "putilimp.h"
#include "intltest.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>    

#if U_PLATFORM_USES_ONLY_WIN32_API
    
#   undef POSIX
#elif U_PLATFORM_IMPLEMENTS_POSIX
#   define POSIX
#else
#   undef POSIX
#endif


#if U_PLATFORM == U_PF_OS390
#define __DOT1 1
#ifndef __UU
#   define __UU
#endif
#ifndef _XPG4_2
#   define _XPG4_2
#endif
#include <unistd.h>
#endif

#if defined(POSIX)
#define HAVE_IMP

#if (ICU_USE_THREADS == 1)
#include <pthread.h>
#endif

#if defined(__hpux) && defined(HPUX_CMA)
# if defined(read)  
#  undef read
# endif
#endif

#if U_PLATFORM == U_PF_OS390
#include <sys/types.h>
#endif

#if U_PLATFORM != U_PF_OS390
#include <signal.h>
#endif


#ifndef _XPG4_2
#define _XPG4_2
#endif


#ifndef __USE_XOPEN_EXTENDED
#define __USE_XOPEN_EXTENDED 
#endif


#ifndef _INCLUDE_XOPEN_SOURCE_EXTENDED
#define _INCLUDE_XOPEN_SOURCE_EXTENDED
#endif

#include <unistd.h>

#endif

#ifdef sleep
#undef sleep
#endif


#if (ICU_USE_THREADS==0)
    SimpleThread::SimpleThread()
    {}

    SimpleThread::~SimpleThread()
    {}

    int32_t 
    SimpleThread::start()
    { return -1; }

    void 
    SimpleThread::run()
    {}

    void 
    SimpleThread::sleep(int32_t millis)
    {}

    UBool  
    SimpleThread::isRunning() {
        return FALSE;
    }
#else

#include "unicode/putil.h"


#include "unicode/numfmt.h"
#include "unicode/choicfmt.h"
#include "unicode/msgfmt.h"
#include "unicode/locid.h"
#include "unicode/ucol.h"
#include "unicode/calendar.h"
#include "ucaconf.h"

#if U_PLATFORM_USES_ONLY_WIN32_API
#define HAVE_IMP

#   define VC_EXTRALEAN
#   define WIN32_LEAN_AND_MEAN
#   define NOUSER
#   define NOSERVICE
#   define NOIME
#   define NOMCX
#include <windows.h>
#include <process.h>






struct Win32ThreadImplementation
{
    HANDLE         fHandle;
    unsigned int   fThreadID;
};


extern "C" unsigned int __stdcall SimpleThreadProc(void *arg)
{
    ((SimpleThread*)arg)->run();
    return 0;
}

SimpleThread::SimpleThread()
:fImplementation(0)
{
    Win32ThreadImplementation *imp = new Win32ThreadImplementation;
    imp->fHandle = 0;
    fImplementation = imp;
}

SimpleThread::~SimpleThread()
{
    
    
    
    Win32ThreadImplementation *imp = (Win32ThreadImplementation*)fImplementation;
    if (imp != 0) {
        if (imp->fHandle != 0) {
            CloseHandle(imp->fHandle);
            imp->fHandle = 0;
        }
    }
    delete (Win32ThreadImplementation*)fImplementation;
}

int32_t SimpleThread::start()
{
    Win32ThreadImplementation *imp = (Win32ThreadImplementation*)fImplementation;
    if(imp->fHandle != NULL) {
        
        
        return -1;
    }

    imp->fHandle = (HANDLE) _beginthreadex(
        NULL,                                 
        0x20000,                              
        SimpleThreadProc,                     
        (void *)this,                         
        0,                                    
        &imp->fThreadID                       
        );

    if (imp->fHandle == 0) {
        
        int err = errno;
        if (err == 0) {
            err = -1;
        }
        return err;
    }
    return 0;
}


UBool  SimpleThread::isRunning() {
    
    
    
    
    
    
    
    
    
    
    
    
    
    Win32ThreadImplementation *imp = (Win32ThreadImplementation*)fImplementation;
    
    bool      success;
    DWORD     threadExitCode;

    if (imp->fHandle == 0) {
        
        return FALSE;
    }
    success = GetExitCodeThread(imp->fHandle,   &threadExitCode) != 0;
    if (! success) {
        
        return FALSE;
    }
    return (threadExitCode == STILL_ACTIVE);
}


void SimpleThread::sleep(int32_t millis)
{
    ::Sleep(millis);
}






#elif U_PLATFORM == U_PF_CLASSIC_MACOS





#define HAVE_IMP

SimpleThread::SimpleThread()
{}

SimpleThread::~SimpleThread()
{}

int32_t 
SimpleThread::start()
{ return 0; }

void 
SimpleThread::run()
{}

void 
SimpleThread::sleep(int32_t millis)
{}

UBool  
SimpleThread::isRunning() {
    return FALSE;
}

#endif























#if defined(POSIX)
#define HAVE_IMP

struct PosixThreadImplementation
{
    pthread_t        fThread;
    UBool            fRunning;
    UBool            fRan;          
};

extern "C" void* SimpleThreadProc(void *arg)
{
    
    SimpleThread *This = (SimpleThread *)arg;
    This->run();      

    
    
    
    PosixThreadImplementation *imp = (PosixThreadImplementation*)This->fImplementation;
    umtx_lock(NULL);
    imp->fRunning = FALSE;
    umtx_unlock(NULL);
    return 0;
}

SimpleThread::SimpleThread() 
{
    PosixThreadImplementation *imp = new PosixThreadImplementation;
    imp->fRunning   = FALSE;
    imp->fRan       = FALSE;
    fImplementation = imp;
}

SimpleThread::~SimpleThread()
{
    PosixThreadImplementation *imp = (PosixThreadImplementation*)fImplementation;
    if (imp->fRan) {
        pthread_join(imp->fThread, NULL);
    }
    delete imp;
    fImplementation = (void *)0xdeadbeef;
}

int32_t SimpleThread::start()
{
    int32_t        rc;
    static pthread_attr_t attr;
    static UBool attrIsInitialized = FALSE;

    PosixThreadImplementation *imp = (PosixThreadImplementation*)fImplementation;
    imp->fRunning = TRUE;
    imp->fRan     = TRUE;

#ifdef HPUX_CMA
    if (attrIsInitialized == FALSE) {
        rc = pthread_attr_create(&attr);
        attrIsInitialized = TRUE;
    }
    rc = pthread_create(&(imp->fThread),attr,&SimpleThreadProc,(void*)this);
#else
    if (attrIsInitialized == FALSE) {
        rc = pthread_attr_init(&attr);
#if U_PLATFORM == U_PF_OS390
        {
            int detachstate = 0;  
                                  
                                  
                                  
                                  
            pthread_attr_setdetachstate(&attr, &detachstate);
        }
#else
        
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
#endif
        attrIsInitialized = TRUE;
    }
    rc = pthread_create(&(imp->fThread),&attr,&SimpleThreadProc,(void*)this);
#endif
    
    if (rc != 0) {
        
        imp->fRan     = FALSE;
        imp->fRunning = FALSE;
    }

    return rc;
}


UBool  
SimpleThread::isRunning() {
    
    
    
    
    PosixThreadImplementation *imp = (PosixThreadImplementation*)fImplementation;
    umtx_lock(NULL);
    UBool retVal = imp->fRunning;
    umtx_unlock(NULL);
    return retVal;
}


void SimpleThread::sleep(int32_t millis)
{
#if U_PLATFORM == U_PF_SOLARIS
    sigignore(SIGALRM);
#endif

#ifdef HPUX_CMA
    cma_sleep(millis/100);
#elif U_PLATFORM == U_PF_HPUX || U_PLATFORM == U_PF_OS390
    millis *= 1000;
    while(millis >= 1000000) {
        usleep(999999);
        millis -= 1000000;
    }
    if(millis > 0) {
        usleep(millis);
    }
#else
    usleep(millis * 1000);
#endif
}

#endif



#ifndef HAVE_IMP
#error  No implementation for threads! Cannot test.
0 = 216; 
#endif






class ThreadWithStatus : public SimpleThread
{
public:
    UBool  getError() { return (fErrors > 0); } 
    UBool  getError(UnicodeString& fillinError) { fillinError = fErrorString; return (fErrors > 0); } 
    virtual ~ThreadWithStatus(){}
protected:
    ThreadWithStatus() :  fErrors(0) {}
    void error(const UnicodeString &error) { 
        fErrors++; fErrorString = error; 
        SimpleThread::errorFunc();  
    }
    void error() { error("An error occured."); }
private:
    int32_t fErrors;
    UnicodeString fErrorString;
};

#endif 
