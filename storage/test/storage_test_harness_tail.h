






































#ifndef TEST_NAME
#error "Must #define TEST_NAME before including storage_test_harness_tail.h"
#endif

#ifndef TEST_FILE
#error "Must #define TEST_FILE before include storage_test_harness_tail.h"
#endif

int
main(int aArgc,
     char **aArgv)
{
  ScopedXPCOM xpcom(TEST_NAME);

  for (size_t i = 0; i < NS_ARRAY_LENGTH(gTests); i++)
    gTests[i]();

  if (gPassedTests == gTotalTests)
    passed(TEST_FILE);

  (void)printf("%i of %i tests passed\n", gPassedTests, gTotalTests);
}
