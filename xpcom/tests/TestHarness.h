










































#ifndef TestHarness_h__
#define TestHarness_h__

#include "nsComponentManagerUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsStringGlue.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

static PRUint32 gFailCount = 0;






void fail(const char* msg, ...)
{
  va_list ap;

  printf("TEST-UNEXPECTED-FAIL | ");

  va_start(ap, msg);
  vprintf(msg, ap);
  va_end(ap);

  putchar('\n');
  ++gFailCount;
}






void passed(const char* test)
{
  printf("TEST-PASS | %s\n", test);
}




static const char* gCurrentProfile;
static PRBool gProfilerTriedInit = PR_FALSE;
static PRBool gProfilerInited = PR_FALSE;





static PRBool _PlatformInitProfiler();
static PRBool _PlatformStartProfile(const char* profileName);
static PRBool _PlatformStopProfile(const char* profileName);
static PRBool _PlatformDeinitProfiler();















static PRBool
StartProfiling(const char* profileName)
{
    if (!gProfilerTriedInit) {
        gProfilerTriedInit = PR_TRUE;
        gProfilerInited = _PlatformInitProfiler();
    }
    if (!gProfilerInited)
        return PR_FALSE;

    NS_ASSERTION(profileName, "need a name for this profile");
    NS_PRECONDITION(!gCurrentProfile, "started a new profile before stopping another");

    PRBool rv = _PlatformStartProfile(profileName);
    gCurrentProfile = profileName;
    return rv;
}












static PRBool
StopProfiling()
{
    NS_ASSERTION(gProfilerTriedInit, "tried to stop profile before starting one");
    if (!gProfilerInited)
        return PR_FALSE;

    NS_PRECONDITION(gCurrentProfile, "tried to stop profile before starting one");
    
    const char* profileName = gCurrentProfile;
    gCurrentProfile = 0;
    return _PlatformStopProfile(profileName);
}



#if defined(MOZ_SHARK)
#include <CHUD/CHUD.h>

static PRBool
_PlatformInitProfiler()
{
    if (chudSuccess != chudInitialize())
        return PR_FALSE;
    if (chudSuccess != chudAcquireRemoteAccess()) {
        NS_WARNING("Couldn't connect to Shark.  Is it running and in Programmatic mode (Shift-Cmd-R)?");
        return PR_FALSE;
    }
   return PR_TRUE;
}

static PRBool
_PlatformStartProfile(const char* profileName)
{
    return (chudSuccess == chudStartRemotePerfMonitor(profileName)) ?
        PR_TRUE : PR_FALSE;
}

static PRBool
_PlatformStopProfile(const char* profileName)
{
    return (chudSuccess == chudStopRemotePerfMonitor()) ?
        PR_TRUE : PR_FALSE;
}

static PRBool
_PlatformDeinitProfiler()
{
    return (chudIsRemoteAccessAcquired() 
            && chudSuccess == chudReleaseRemoteAccess()) ?
        PR_TRUE : PR_FALSE;
}



#else 

static PRBool
_PlatformInitProfiler()
{
    NS_WARNING("Profiling is not available/configured for your platform.");
    return PR_FALSE;
}
static PRBool
_PlatformStartProfile(const char* profileName)
{
    NS_WARNING("Profiling is not available/configured for your platform.");
    return PR_FALSE;
}
static PRBool
_PlatformStopProfile(const char* profileName)
{
    NS_WARNING("Profiling is not available/configured for your platform.");
    return PR_FALSE;
}
static PRBool
_PlatformDeinitProfiler()
{
    NS_WARNING("Profiling is not available/configured for your platform.");
    return PR_FALSE;
}

#endif



class ScopedXPCOM
{
  public:
    ScopedXPCOM(const char* testName,
                nsIDirectoryServiceProvider *dirSvcProvider = NULL)
    {
      mTestName = testName;
      printf("Running %s tests...\n", mTestName);

      nsresult rv = NS_InitXPCOM2(&mServMgr, NULL, dirSvcProvider);
      if (NS_FAILED(rv))
      {
        fail("NS_InitXPCOM2 returned failure code 0x%x", rv);
        mServMgr = NULL;
      }
    }

    ~ScopedXPCOM()
    {
      if (gProfilerInited)
        if (!_PlatformDeinitProfiler())
          NS_WARNING("Problem shutting down profiler");

      if (mServMgr)
      {
        NS_RELEASE(mServMgr);
        nsresult rv = NS_ShutdownXPCOM(NULL);
        if (NS_FAILED(rv))
        {
          fail("XPCOM shutdown failed with code 0x%x", rv);
          exit(1);
        }
      }

      printf("Finished running %s tests.\n", mTestName);
    }

    PRBool failed()
    {
      return mServMgr == NULL;
    }

  private:
    const char* mTestName;
    nsIServiceManager* mServMgr;
};

#endif  
