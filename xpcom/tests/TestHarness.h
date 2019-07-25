










































#ifndef TestHarness_h__
#define TestHarness_h__

#if defined(_MSC_VER) && defined(MOZ_STATIC_JS)




#define STATIC_JS_API
#endif

#include "nsComponentManagerUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsStringGlue.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsDirectoryServiceDefs.h"
#include "nsDirectoryServiceUtils.h"
#include "nsIDirectoryService.h"
#include "nsIFile.h"
#include "nsIProperties.h"
#include "nsXULAppAPI.h"
#include "jsdbgapi.h"
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















inline bool
StartProfiling(const char* profileName)
{
    NS_ASSERTION(profileName, "need a name for this profile");
    NS_PRECONDITION(!gCurrentProfile, "started a new profile before stopping another");

    JSBool ok = JS_StartProfiling(profileName);
    gCurrentProfile = profileName;
    return ok ? PR_TRUE : PR_FALSE;
}












inline bool
StopProfiling()
{
    NS_PRECONDITION(gCurrentProfile, "tried to stop profile before starting one");

    const char* profileName = gCurrentProfile;
    gCurrentProfile = 0;
    return JS_StopProfiling(profileName) ? PR_TRUE : PR_FALSE;
}



class ScopedLogging
{
public:
    ScopedLogging()
    {
        NS_LogInit();
    }

    ~ScopedLogging()
    {
        NS_LogTerm();
    }
};

class ScopedXPCOM : public nsIDirectoryServiceProvider2
{
  public:
    NS_DECL_ISUPPORTS

    ScopedXPCOM(const char* testName,
                nsIDirectoryServiceProvider *dirSvcProvider = NULL)
    : mDirSvcProvider(dirSvcProvider)
    {
      mTestName = testName;
      printf("Running %s tests...\n", mTestName);

      nsresult rv = NS_InitXPCOM2(&mServMgr, NULL, this);
      if (NS_FAILED(rv))
      {
        fail("NS_InitXPCOM2 returned failure code 0x%x", rv);
        mServMgr = NULL;
        return;
      }
    }

    ~ScopedXPCOM()
    {
      
      if (mProfD) {
        if (NS_FAILED(mProfD->Remove(PR_TRUE)))
          NS_WARNING("Problem removing profile direrctory");

        mProfD = nsnull;
      }

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

    bool failed()
    {
      return mServMgr == NULL;
    }

    already_AddRefed<nsIFile> GetProfileDirectory()
    {
      if (mProfD) {
        nsCOMPtr<nsIFile> copy = mProfD;
        return copy.forget();
      }

      
      nsCOMPtr<nsIFile> profD;
      nsresult rv = NS_GetSpecialDirectory(NS_OS_TEMP_DIR,
                                           getter_AddRefs(profD));
      NS_ENSURE_SUCCESS(rv, nsnull);

      rv = profD->Append(NS_LITERAL_STRING("cpp-unit-profd"));
      NS_ENSURE_SUCCESS(rv, nsnull);

      rv = profD->CreateUnique(nsIFile::DIRECTORY_TYPE, 0755);
      NS_ENSURE_SUCCESS(rv, nsnull);

      mProfD = profD;
      return profD.forget();
    }

    
    

    NS_IMETHODIMP GetFile(const char *aProperty, bool *_persistent,
                          nsIFile **_result)
    {
      
      if (mDirSvcProvider &&
          NS_SUCCEEDED(mDirSvcProvider->GetFile(aProperty, _persistent,
                                                _result))) {
        return NS_OK;
      }

      
      if (0 == strcmp(aProperty, NS_APP_USER_PROFILE_50_DIR) ||
          0 == strcmp(aProperty, NS_APP_USER_PROFILE_LOCAL_50_DIR) ||
          0 == strcmp(aProperty, NS_APP_PROFILE_LOCAL_DIR_STARTUP)) {
        nsCOMPtr<nsIFile> profD = GetProfileDirectory();
        NS_ENSURE_TRUE(profD, NS_ERROR_FAILURE);

        nsCOMPtr<nsIFile> clone;
        nsresult rv = profD->Clone(getter_AddRefs(clone));
        NS_ENSURE_SUCCESS(rv, rv);

        *_persistent = PR_TRUE;
        clone.forget(_result);
        return NS_OK;
      }

      return NS_ERROR_FAILURE;
    }

    
    

    NS_IMETHODIMP GetFiles(const char *aProperty, nsISimpleEnumerator **_enum)
    {
      
      nsCOMPtr<nsIDirectoryServiceProvider2> provider =
        do_QueryInterface(mDirSvcProvider);
      if (provider && NS_SUCCEEDED(provider->GetFiles(aProperty, _enum))) {
        return NS_OK;
      }

     return NS_ERROR_FAILURE;
   }

  private:
    const char* mTestName;
    nsIServiceManager* mServMgr;
    nsCOMPtr<nsIDirectoryServiceProvider> mDirSvcProvider;
    nsCOMPtr<nsIFile> mProfD;
};

NS_IMPL_QUERY_INTERFACE2(
  ScopedXPCOM,
  nsIDirectoryServiceProvider,
  nsIDirectoryServiceProvider2
)

NS_IMETHODIMP_(nsrefcnt)
ScopedXPCOM::AddRef()
{
  return 2;
}

NS_IMETHODIMP_(nsrefcnt)
ScopedXPCOM::Release()
{
  return 1;
}

#endif  
