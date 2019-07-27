










#ifndef TestHarness_h__
#define TestHarness_h__

#if defined(_MSC_VER) && defined(MOZ_STATIC_JS)




#define STATIC_JS_API
#endif

#include "mozilla/ArrayUtils.h"

#include "prenv.h"
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
#include "nsIObserverService.h"
#include "nsXULAppAPI.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

static uint32_t gFailCount = 0;






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






void passed(const char* msg, ...)
{
  va_list ap;

  printf("TEST-PASS | ");

  va_start(ap, msg);
  vprintf(msg, ap);
  va_end(ap);

  putchar('\n');
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

    explicit ScopedXPCOM(const char* testName,
                         nsIDirectoryServiceProvider *dirSvcProvider = nullptr)
    : mDirSvcProvider(dirSvcProvider)
    {
      mTestName = testName;
      printf("Running %s tests...\n", mTestName);

      nsresult rv = NS_InitXPCOM2(&mServMgr, nullptr, this);
      if (NS_FAILED(rv))
      {
        fail("NS_InitXPCOM2 returned failure code 0x%x", rv);
        mServMgr = nullptr;
        return;
      }
    }

    ~ScopedXPCOM()
    {
      
      if (mProfD) {
        nsCOMPtr<nsIObserverService> os =
          do_GetService(NS_OBSERVERSERVICE_CONTRACTID);
        MOZ_ASSERT(os);
        if (os) {
          MOZ_ALWAYS_TRUE(NS_SUCCEEDED(os->NotifyObservers(nullptr, "profile-change-net-teardown", nullptr)));
          MOZ_ALWAYS_TRUE(NS_SUCCEEDED(os->NotifyObservers(nullptr, "profile-change-teardown", nullptr)));
          MOZ_ALWAYS_TRUE(NS_SUCCEEDED(os->NotifyObservers(nullptr, "profile-before-change", nullptr)));
          MOZ_ALWAYS_TRUE(NS_SUCCEEDED(os->NotifyObservers(nullptr, "profile-before-change2", nullptr)));
        }

        if (NS_FAILED(mProfD->Remove(true))) {
          NS_WARNING("Problem removing profile directory");
        }

        mProfD = nullptr;
      }

      if (mServMgr)
      {
        NS_RELEASE(mServMgr);
        nsresult rv = NS_ShutdownXPCOM(nullptr);
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
      return mServMgr == nullptr;
    }

    already_AddRefed<nsIFile> GetProfileDirectory()
    {
      if (mProfD) {
        nsCOMPtr<nsIFile> copy = mProfD;
        return copy.forget();
      }

      
      
      
      nsCOMPtr<nsIFile> profD;
      nsresult rv = NS_GetSpecialDirectory(NS_OS_CURRENT_PROCESS_DIR,
                                           getter_AddRefs(profD));
      NS_ENSURE_SUCCESS(rv, nullptr);

      rv = profD->Append(NS_LITERAL_STRING("cpp-unit-profd"));
      NS_ENSURE_SUCCESS(rv, nullptr);

      rv = profD->CreateUnique(nsIFile::DIRECTORY_TYPE, 0755);
      NS_ENSURE_SUCCESS(rv, nullptr);

      mProfD = profD;
      return profD.forget();
    }

    already_AddRefed<nsIFile> GetGREDirectory()
    {
      if (mGRED) {
        nsCOMPtr<nsIFile> copy = mGRED;
        return copy.forget();
      }

      char* env = PR_GetEnv("MOZ_XRE_DIR");
      nsCOMPtr<nsIFile> greD;
      if (env) {
        NS_NewLocalFile(NS_ConvertUTF8toUTF16(env), false,
                        getter_AddRefs(greD));
      }

      mGRED = greD;
      return greD.forget();
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

        *_persistent = true;
        clone.forget(_result);
        return NS_OK;
      }

      if (0 == strcmp(aProperty, NS_GRE_DIR)) {
        nsCOMPtr<nsIFile> greD = GetGREDirectory();
        NS_ENSURE_TRUE(greD, NS_ERROR_FAILURE);

        *_persistent = true;
        greD.forget(_result);
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
    nsCOMPtr<nsIFile> mGRED;
};

NS_IMPL_QUERY_INTERFACE(
  ScopedXPCOM,
  nsIDirectoryServiceProvider,
  nsIDirectoryServiceProvider2
)

NS_IMETHODIMP_(MozExternalRefCountType)
ScopedXPCOM::AddRef()
{
  return 2;
}

NS_IMETHODIMP_(MozExternalRefCountType)
ScopedXPCOM::Release()
{
  return 1;
}

#endif  
