




































#include <stdlib.h>
#include <stdio.h>

#include "prenv.h"
#include "nsIComponentManager.h"
#include "nsISimpleEnumerator.h"
#include "nsIServiceManager.h"
#include "nsServiceManagerUtils.h"
#include "nsDirectoryServiceDefs.h"
#include "nsIProperties.h"
#include "nsCOMPtr.h"
#include "nsILocalFile.h"

#include "nsAirbagExceptionHandler.h"
#include "nsICrashReporter.h"

#define mu_assert(message, test) do { if (NS_FAILED(test)) \
                                       return message; } while (0)
#define mu_assert_failure(message, test) do { if (NS_SUCCEEDED(test)) \
                                               return message; } while (0)
#define mu_run_test(test) do { char *message = test(); tests_run++; \
                                if (message) return message; } while (0)
int tests_run;

char *
test_init_exception_handler()
{
  nsCOMPtr<nsILocalFile> lf;
  
  
  mu_assert("NS_NewNativeLocalFile", NS_NewNativeLocalFile(EmptyCString(),
                                                           PR_TRUE,
                                                           getter_AddRefs(lf)));

  mu_assert("CrashReporter::SetExceptionHandler",
            CrashReporter::SetExceptionHandler(lf, nsnull));
  return 0;
}

char *
test_set_minidump_path()
{
  nsresult rv;
  nsCOMPtr<nsIProperties> directoryService = 
    do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID, &rv);

  mu_assert("do_GetService", rv);

  nsCOMPtr<nsILocalFile> currentDirectory;
  rv = directoryService->Get(NS_XPCOM_CURRENT_PROCESS_DIR,
                             NS_GET_IID(nsILocalFile),
                             getter_AddRefs(currentDirectory));
  mu_assert("directoryService->Get", rv);

  nsAutoString currentDirectoryPath;
  rv = currentDirectory->GetPath(currentDirectoryPath);
  mu_assert("currentDirectory->GetPath", rv);
  
  mu_assert("CrashReporter::SetMinidumpPath",
            CrashReporter::SetMinidumpPath(currentDirectoryPath));

  return 0;
}

char *
test_annotate_crash_report_basic()
{
  mu_assert("CrashReporter::AnnotateCrashReport: basic",
     CrashReporter::AnnotateCrashReport(NS_LITERAL_CSTRING("test"),
                                        NS_LITERAL_CSTRING("some data")));

  return 0;
}

char *
test_annotate_crash_report_invalid_equals()
{
  mu_assert_failure("CrashReporter::AnnotateCrashReport: invalid = in key",
     CrashReporter::AnnotateCrashReport(NS_LITERAL_CSTRING("test=something"),
                                        NS_LITERAL_CSTRING("some data")));
  return 0;
}

char *
test_annotate_crash_report_invalid_cr()
{
  mu_assert_failure("CrashReporter::AnnotateCrashReport: invalid \n in key",
     CrashReporter::AnnotateCrashReport(NS_LITERAL_CSTRING("test\nsomething"),
                                        NS_LITERAL_CSTRING("some data")));
  return 0;
}

char *
test_unset_exception_handler()
{
  mu_assert("CrashReporter::UnsetExceptionHandler",
            CrashReporter::UnsetExceptionHandler());
  return 0;
}

static char* all_tests()
{
  mu_run_test(test_init_exception_handler);
  mu_run_test(test_set_minidump_path);
  mu_run_test(test_annotate_crash_report_basic);
  mu_run_test(test_annotate_crash_report_invalid_equals);
  mu_run_test(test_annotate_crash_report_invalid_cr);
  mu_run_test(test_unset_exception_handler);
  return 0;
}

int
main (int argc, char **argv)
{
  NS_InitXPCOM2(nsnull, nsnull, nsnull);

  char* env = new char[13];
  strcpy(env, "MOZ_AIRBAG=1");
  PR_SetEnv(env);

  char* result = all_tests();
  if (result != 0) {
    printf("FAIL: %s\n", result);
  }
  else {
    printf("ALL TESTS PASSED\n");
  }
  printf("Tests run: %d\n", tests_run);
 
  return result != 0;
}
