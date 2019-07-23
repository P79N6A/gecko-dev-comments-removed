








































#ifdef XP_WIN
  #include <windows.h>
  #define NS_main wmain
  #define NS_tstrrchr wcsrchr
  #define NS_tsnprintf _snwprintf
  #define NS_T(str) L ## str
  #define PATH_SEPARATOR_CHAR L'\\'
#else
  #include <unistd.h>
  #define NS_main main
  #define NS_tstrrchr strrchr
  #define NS_tsnprintf snprintf
  #define NS_T(str) str
#ifdef XP_OS2
  #define PATH_SEPARATOR_CHAR '\\'
#else
  #define PATH_SEPARATOR_CHAR '/'
#endif
#endif

#include <stdio.h>
#include <string.h>

#include "updater/resource.h"
#include "updater/progressui.h"
#include "updater/readstrings.h"
#include "updater/errors.h"

#ifndef MAXPATHLEN
# ifdef PATH_MAX
#  define MAXPATHLEN PATH_MAX
# elif defined(_MAX_PATH)
#  define MAXPATHLEN MAX_PATH
# elif defined(_MAX_PATH)
#  define MAXPATHLEN _MAX_PATH
# elif defined(CCHMAXPATH)
#  define MAXPATHLEN CCHMAXPATH
# else
#  define MAXPATHLEN 1024
# endif
#endif

#define TEST_NAME "Updater ReadStrings"
#define UNEXPECTED_FAIL_PREFIX "*** TEST-UNEXPECTED-FAIL"

int NS_main(int argc, NS_tchar **argv)
{
  printf("Running TestAUSReadStrings tests\n");

  int rv = 0;
  int retval;
  NS_tchar inifile[MAXPATHLEN];
  StringTable testStrings;

  NS_tchar *slash = NS_tstrrchr(argv[0], PATH_SEPARATOR_CHAR);
  if (!slash) {
    printf("%s | %s | unable to find platform specific path separator\n",
           UNEXPECTED_FAIL_PREFIX, TEST_NAME);
    return 20;
  }

  *(++slash) = '\0';
  
  
  NS_tsnprintf(inifile, sizeof(inifile), NS_T("%sTestAUSReadStrings1.ini"), argv[0]);
  retval = ReadStrings(inifile, &testStrings);
  if (retval == OK) {
    if (strcmp(testStrings.title, "Title Test - \xD0\x98\xD1\x81\xD0\xBF\xD1\x8B" \
                                  "\xD1\x82\xD0\xB0\xD0\xBD\xD0\xB8\xD0\xB5 " \
                                  "\xCE\x94\xCE\xBF\xCE\xBA\xCE\xB9\xCE\xBC\xCE\xAE " \
                                  "\xE3\x83\x86\xE3\x82\xB9\xE3\x83\x88 " \
                                  "\xE6\xB8\xAC\xE8\xA9\xA6 " \
                                  "\xE6\xB5\x8B\xE8\xAF\x95") != 0) {
      rv = 21;
      printf("%s | %s Title ini value incorrect | Test 2\n",
             UNEXPECTED_FAIL_PREFIX, TEST_NAME);
    }

    if (strcmp(testStrings.info, "Info Test - \xD0\x98\xD1\x81\xD0\xBF\xD1\x8B" \
                                 "\xD1\x82\xD0\xB0\xD0\xBD\xD0\xB8\xD0\xB5 " \
                                 "\xCE\x94\xCE\xBF\xCE\xBA\xCE\xB9\xCE\xBC\xCE\xAE " \
                                 "\xE3\x83\x86\xE3\x82\xB9\xE3\x83\x88 " \
                                 "\xE6\xB8\xAC\xE8\xA9\xA6 " \
                                 "\xE6\xB5\x8B\xE8\xAF\x95\xE2\x80\xA6") != 0) {
      rv = 22;
      printf("%s | %s Info ini value incorrect | Test 2\n",
             UNEXPECTED_FAIL_PREFIX, TEST_NAME);
    }
  }
  else {
    printf("%s | %s ReadStrings returned %i | Test 2\n",
           UNEXPECTED_FAIL_PREFIX, TEST_NAME, retval);
    rv = 23;
  }

  
  
  NS_tsnprintf(inifile, sizeof(inifile), NS_T("%sTestAUSReadStrings2.ini"), argv[0]);
  retval = ReadStrings(inifile, &testStrings);
  if (retval != PARSE_ERROR) {
    rv = 24;
    printf("%s | %s ReadStrings returned %i | Test 3\n",
           UNEXPECTED_FAIL_PREFIX, TEST_NAME, retval);
  }

  
  
  NS_tsnprintf(inifile, sizeof(inifile), NS_T("%sTestAUSReadStrings3.ini"), argv[0]);
  retval = ReadStrings(inifile, &testStrings);
  if (retval != PARSE_ERROR) {
    rv = 25;
    printf("%s | %s ReadStrings returned %i | Test 4\n",
           UNEXPECTED_FAIL_PREFIX, TEST_NAME, retval);
  }

  
  NS_tsnprintf(inifile, sizeof(inifile), NS_T("%sTestAUSReadStringsBogus.ini"), argv[0]);
  retval = ReadStrings(inifile, &testStrings);
  if (retval != READ_ERROR) {
    rv = 26;
    printf("%s | %s ini file doesn't exist | Test 5\n",
           UNEXPECTED_FAIL_PREFIX, TEST_NAME);
  }


  if (rv == 0) {
    printf("*** TEST-PASS | %s | all tests passed\n", TEST_NAME);
  } else {
    printf("*** TEST-FAIL | %s | some tests failed\n", TEST_NAME);
  }

  return rv;
}
