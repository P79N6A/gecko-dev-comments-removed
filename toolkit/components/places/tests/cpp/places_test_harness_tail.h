






































#include "nsWidgetsCID.h"
#include "nsIComponentRegistrar.h"

#ifndef TEST_NAME
#error "Must #define TEST_NAME before including places_test_harness_tail.h"
#endif

#ifndef TEST_FILE
#error "Must #define TEST_FILE before include places_test_harness_tail.h"
#endif

int gTestsIndex = 0;

#define TEST_INFO_STR "TEST-INFO | (%s) | "

class RunNextTest : public nsRunnable
{
public:
  NS_IMETHOD Run()
  {
    NS_ASSERTION(NS_IsMainThread(), "Not running on the main thread?");
    if (gTestsIndex < int(NS_ARRAY_LENGTH(gTests))) {
      do_test_pending();
      Test &test = gTests[gTestsIndex++];
      (void)fprintf(stderr, TEST_INFO_STR "Running %s.\n", TEST_FILE,
                    test.name);
      test.func();
    }

    do_test_finished();
    return NS_OK;
  }
};

void
run_next_test()
{
  nsCOMPtr<nsIRunnable> event = new RunNextTest();
  do_check_success(NS_DispatchToCurrentThread(event));
}

int gPendingTests = 0;

void
do_test_pending()
{
  NS_ASSERTION(NS_IsMainThread(), "Not running on the main thread?");
  gPendingTests++;
}

void
do_test_finished()
{
  NS_ASSERTION(NS_IsMainThread(), "Not running on the main thread?");
  NS_ASSERTION(gPendingTests > 0, "Invalid pending test count!");
  gPendingTests--;
}

void
disable_idle_service()
{
  (void)fprintf(stderr, TEST_INFO_STR  "Disabling Idle Service.\n", TEST_FILE);
  static NS_DEFINE_IID(kIdleCID, NS_IDLE_SERVICE_CID);
  nsresult rv;
  nsCOMPtr<nsIFactory> idleFactory = do_GetClassObject(kIdleCID, &rv);
  do_check_success(rv);
  nsCOMPtr<nsIComponentRegistrar> registrar;
  rv = NS_GetComponentRegistrar(getter_AddRefs(registrar));
  do_check_success(rv);
  rv = registrar->UnregisterFactory(kIdleCID, idleFactory);
  do_check_success(rv);
}

int
main(int aArgc,
     char** aArgv)
{
  ScopedXPCOM xpcom(TEST_NAME);

  
  
  disable_idle_service();

  do_test_pending();
  run_next_test();

  
  while (gPendingTests) {
    (void)NS_ProcessNextEvent();
  }

  
  (void)NS_ProcessPendingEvents(nsnull);

  
  if (gPassedTests == gTotalTests) {
    passed(TEST_FILE);
  }

  (void)fprintf(stderr, TEST_INFO_STR  "%u of %u tests passed\n",
                TEST_FILE, unsigned(gPassedTests), unsigned(gTotalTests));

  return gPassedTests == gTotalTests ? 0 : -1;
}
