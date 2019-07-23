









































#ifndef TestHarness_h__
#define TestHarness_h__

#include "nsIServiceManager.h"
#include "nsComponentManagerUtils.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include <stdio.h>
#include <stdlib.h>

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
        printf("FAIL NS_InitXPCOM2 returned failure code %x\n", rv);
        mServMgr = NULL;
      }
    }

    ~ScopedXPCOM()
    {
      if (mServMgr)
      {
        NS_RELEASE(mServMgr);
        nsresult rv = NS_ShutdownXPCOM(NULL);
        if (NS_FAILED(rv))
        {
          printf("FAIL XPCOM shutdown failed with code %x\n", rv);
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
