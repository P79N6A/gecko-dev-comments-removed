






































#include "storage_test_harness.h"

#include "nsThreadUtils.h"









class ServiceInitializer : public nsRunnable
{
public:
  NS_IMETHOD Run()
  {
    nsCOMPtr<mozIStorageService> service = getService();
    do_check_true(service);
    return NS_OK;
  }
};




void
test_service_initialization_on_background_thread()
{
  nsCOMPtr<nsIRunnable> event = new ServiceInitializer();
  do_check_true(event);

  nsCOMPtr<nsIThread> thread;
  do_check_success(NS_NewThread(getter_AddRefs(thread)));

  do_check_success(thread->Dispatch(event, NS_DISPATCH_NORMAL));

  
  
  do_check_success(thread->Shutdown());
}

void (*gTests[])(void) = {
  test_service_initialization_on_background_thread,
};

const char *file = __FILE__;
#define TEST_NAME "Background Thread Initialization"
#define TEST_FILE file
#include "storage_test_harness_tail.h"
