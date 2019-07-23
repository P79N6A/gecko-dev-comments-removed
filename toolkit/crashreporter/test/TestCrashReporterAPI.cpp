




































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

#include "nsExceptionHandler.h"


namespace CrashReporter {
  bool GetAnnotation(const nsACString& key, nsACString& data);
}

#define ok(message, test) do {                   \
                               if (!(test))      \
                                 return message; \
                             } while (0)
#define equals(message, a, b) ok(message, a == b)
#define ok_nsresult(message, rv) ok(message, NS_SUCCEEDED(rv))
#define fail_nsresult(message, rv) ok(message, NS_FAILED(rv))
#define run_test(test) do { char *message = test(); tests_run++; \
                            if (message) return message; } while (0)
int tests_run;



char *
test_init_exception_handler()
{
  nsCOMPtr<nsILocalFile> lf;
  
  
  ok_nsresult("NS_NewNativeLocalFile", NS_NewNativeLocalFile(EmptyCString(),
                                                             PR_TRUE,
                                                             getter_AddRefs(lf)));

  ok_nsresult("CrashReporter::SetExceptionHandler",
            CrashReporter::SetExceptionHandler(lf, nsnull));
  return 0;
}

char *
test_set_minidump_path()
{
  nsresult rv;
  nsCOMPtr<nsIProperties> directoryService = 
    do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID, &rv);

  ok_nsresult("do_GetService", rv);

  nsCOMPtr<nsILocalFile> currentDirectory;
  rv = directoryService->Get(NS_XPCOM_CURRENT_PROCESS_DIR,
                             NS_GET_IID(nsILocalFile),
                             getter_AddRefs(currentDirectory));
  ok_nsresult("directoryService->Get", rv);

  nsAutoString currentDirectoryPath;
  rv = currentDirectory->GetPath(currentDirectoryPath);
  ok_nsresult("currentDirectory->GetPath", rv);
  
  ok_nsresult("CrashReporter::SetMinidumpPath",
            CrashReporter::SetMinidumpPath(currentDirectoryPath));

  return 0;
}

char *
test_annotate_crash_report_basic()
{
  ok_nsresult("CrashReporter::AnnotateCrashReport: basic 1",
     CrashReporter::AnnotateCrashReport(NS_LITERAL_CSTRING("test"),
                                        NS_LITERAL_CSTRING("some data")));


  nsCAutoString result;
  ok("CrashReporter::GetAnnotation", CrashReporter::GetAnnotation(NS_LITERAL_CSTRING("test"),
                                                                  result));
  nsCString msg = result + NS_LITERAL_CSTRING(" == ") +
    NS_LITERAL_CSTRING("some data");
  equals((char*)PromiseFlatCString(msg).get(), result,
         NS_LITERAL_CSTRING("some data"));

  
  ok_nsresult("CrashReporter::AnnotateCrashReport: basic 2",
     CrashReporter::AnnotateCrashReport(NS_LITERAL_CSTRING("test"),
                                        NS_LITERAL_CSTRING("some other data")));


  ok("CrashReporter::GetAnnotation", CrashReporter::GetAnnotation(NS_LITERAL_CSTRING("test"),
                                                                  result));
  msg = result + NS_LITERAL_CSTRING(" == ") +
    NS_LITERAL_CSTRING("some other data");
  equals((char*)PromiseFlatCString(msg).get(), result,
         NS_LITERAL_CSTRING("some other data"));
  return 0;
}

char *
test_appendnotes_crash_report()
{
  
  ok_nsresult("CrashReporter::AppendAppNotesToCrashReport: 1",
              CrashReporter::AppendAppNotesToCrashReport(NS_LITERAL_CSTRING("some data")));

  
  ok_nsresult("CrashReporter::AppendAppNotesToCrashReport: 2",
              CrashReporter::AppendAppNotesToCrashReport(NS_LITERAL_CSTRING("some other data")));

  
  nsCAutoString result;
  ok("CrashReporter::GetAnnotation",
     CrashReporter::GetAnnotation(NS_LITERAL_CSTRING("Notes"),
                                  result));

  nsCString msg = result + NS_LITERAL_CSTRING(" == ") +
    NS_LITERAL_CSTRING("some datasome other data");
  equals((char*)PromiseFlatCString(msg).get(), result,
         NS_LITERAL_CSTRING("some datasome other data"));
  return 0;
}

char *
test_annotate_crash_report_invalid_equals()
{
  fail_nsresult("CrashReporter::AnnotateCrashReport: invalid = in key",
     CrashReporter::AnnotateCrashReport(NS_LITERAL_CSTRING("test=something"),
                                        NS_LITERAL_CSTRING("some data")));
  return 0;
}

char *
test_annotate_crash_report_invalid_cr()
{
  fail_nsresult("CrashReporter::AnnotateCrashReport: invalid \n in key",
     CrashReporter::AnnotateCrashReport(NS_LITERAL_CSTRING("test\nsomething"),
                                        NS_LITERAL_CSTRING("some data")));
  return 0;
}

char *
test_unset_exception_handler()
{
  ok_nsresult("CrashReporter::UnsetExceptionHandler",
            CrashReporter::UnsetExceptionHandler());
  return 0;
}

static char* all_tests()
{
  run_test(test_init_exception_handler);
  run_test(test_set_minidump_path);
  run_test(test_annotate_crash_report_basic);
  run_test(test_annotate_crash_report_invalid_equals);
  run_test(test_annotate_crash_report_invalid_cr);
  run_test(test_appendnotes_crash_report);
  run_test(test_unset_exception_handler);
  return 0;
}

int
main (int argc, char **argv)
{
  NS_InitXPCOM2(nsnull, nsnull, nsnull);

  PR_SetEnv("MOZ_CRASHREPORTER=1");

  char* result = all_tests();
  if (result != 0) {
    printf("TEST-UNEXPECTED-FAIL | %s | %s\n", __FILE__, result);
  }
  else {
    printf("TEST-PASS | %s | all tests passed\n", __FILE__);
  }
  printf("Tests run: %d\n", tests_run);
 
  NS_ShutdownXPCOM(nsnull);

  return result != 0;
}
