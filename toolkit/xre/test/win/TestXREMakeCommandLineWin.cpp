




































#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include <fcntl.h>
#include <io.h>

#include "nsWindowsRestart.cpp"




#define DUMMY_ARG1 L"\"arg 1\" "

#define MAXPATHLEN 1024
#define LOG_PREFIX L"XRE MakeCommandLine"
#define MAX_TESTS 100


#define VERBOSE 0





static int
verifyCmdLineCreation(PRUnichar *inCmdLine,
                      PRUnichar *compareCmdLine,
                      PRBool passes, int testNum)
{
  int rv = 0;
  int i;
  int inArgc;
  int outArgc;
  PRBool isEqual;

  
  
  
  
  
  


  
  
  
  
  PRUnichar *inCmdLineNew = (PRUnichar *) malloc((wcslen(DUMMY_ARG1) + wcslen(inCmdLine) + 1) * sizeof(PRUnichar));
  wcscpy(inCmdLineNew, DUMMY_ARG1);
  wcscat(inCmdLineNew, inCmdLine);
  LPWSTR *inArgv = CommandLineToArgvW(inCmdLineNew, &inArgc);

  PRUnichar *outCmdLine = MakeCommandLine(inArgc - 1, inArgv + 1);
  PRUnichar *outCmdLineNew = (PRUnichar *) malloc((wcslen(DUMMY_ARG1) + wcslen(outCmdLine) + 1) * sizeof(PRUnichar));
  wcscpy(outCmdLineNew, DUMMY_ARG1);
  wcscat(outCmdLineNew, outCmdLine);
  LPWSTR *outArgv = CommandLineToArgvW(outCmdLineNew, &outArgc);

  if (VERBOSE) {
    wprintf(L"\n");
    wprintf(L"Verbose Output\n");
    wprintf(L"--------------\n");
    wprintf(L"Input command line   : >%s<\n", inCmdLine);
    wprintf(L"MakeComandLine output: >%s<\n", outCmdLine);
    wprintf(L"Expected command line: >%s<\n", compareCmdLine);

    wprintf(L"input argc : %d\n", inArgc - 1);
    wprintf(L"output argc: %d\n", outArgc - 1);

    for (i = 1; i < inArgc; ++i) {
      wprintf(L"input argv[%d] : >%s<\n", i - 1, inArgv[i]);
    }

    for (i = 1; i < outArgc; ++i) {
      wprintf(L"output argv[%d]: >%s<\n", i - 1, outArgv[i]);
    }
    wprintf(L"\n");
  }

  isEqual = (inArgc == outArgc);
  if (!isEqual) {
    wprintf(L"*** TEST-%s-FAIL | %s ARGC Comparison | Test %2d\n",
            passes ? L"UNEXPECTED" : L"KNOWN", LOG_PREFIX, testNum);
    if (passes) {
      rv = 1;
    }
    LocalFree(inArgv);
    LocalFree(outArgv);
    free(inCmdLineNew);
    free(outCmdLineNew);
    free(outCmdLine);
    return rv;
  }

  for (i = 1; i < inArgc; ++i) {
    isEqual = (wcscmp(inArgv[i], outArgv[i]) == 0);
    if (!isEqual) {
      wprintf(L"*** TEST-%s-FAIL | %s ARGV Comparison | Test %2d\n",
              passes ? L"UNEXPECTED" : L"KNOWN", LOG_PREFIX, testNum);
      if (passes) {
        rv = 1;
      }
      LocalFree(inArgv);
      LocalFree(outArgv);
      free(inCmdLineNew);
      free(outCmdLineNew);
      free(outCmdLine);
      return rv;
    }
  }

  isEqual = (wcscmp(outCmdLine, compareCmdLine) == 0);
  if (!isEqual) {
    wprintf(L"*** TEST-%s-FAIL | %s Command Line Comparison | Test %2d\n",
            passes ? L"UNEXPECTED" : L"KNOWN", LOG_PREFIX, testNum);
    if (passes) {
      rv = 1;
    }
    LocalFree(inArgv);
    LocalFree(outArgv);
    free(inCmdLineNew);
    free(outCmdLineNew);
    free(outCmdLine);
    return rv;
  }

  if (rv == 0) {
    if (passes) {
      wprintf(L"*** TEST-PASS | %s | Test %2d\n", LOG_PREFIX, testNum);
    } else {
      wprintf(L"*** TEST-UNEXPECTED-PASS | %s | Test %2d\n", LOG_PREFIX, testNum);
      rv = 1;
    }
  }

  LocalFree(inArgv);
  LocalFree(outArgv);
  free(inCmdLineNew);
  free(outCmdLineNew);
  free(outCmdLine);
  return rv;
}

int wmain(int argc, PRUnichar *argv[])
{
  int i;
  int rv = 0;

  if (argc > 1 && (_wcsicmp(argv[1], L"-check-one") != 0 ||
                   _wcsicmp(argv[1], L"-check-one") == 0 && argc != 3)) {
    fwprintf(stderr, L"Displays and validates output from MakeCommandLine.\n\n");
    fwprintf(stderr, L"Usage: %s -check-one <test number>\n\n", argv[0]);
    fwprintf(stderr, L"  <test number>\tSpecifies the test number to run from the\n");
    fwprintf(stderr, L"\t\tTestXREMakeCommandLineWin.ini file.\n");
    return 255;
  }

  PRUnichar inifile[MAXPATHLEN];
  if (!::GetModuleFileNameW(0, inifile, MAXPATHLEN)) {
    wprintf(L"*** TEST-FAIL | %s | GetModuleFileNameW\n", LOG_PREFIX);
    return 2;
  }

  WCHAR *slash = wcsrchr(inifile, '\\');
  if (!slash) {
    wprintf(L"*** TEST-FAIL | %s | wcsrchr\n", LOG_PREFIX);
    return 3;
  }

  wcscpy(slash + 1, L"TestXREMakeCommandLineWin.ini\0");

  for (i = 0; i < MAX_TESTS; ++i) {
    PRUnichar sInputVal[MAXPATHLEN];
    PRUnichar sOutputVal[MAXPATHLEN];
    PRUnichar sPassesVal[MAXPATHLEN];
    PRUnichar sInputKey[MAXPATHLEN];
    PRUnichar sOutputKey[MAXPATHLEN];
    PRUnichar sPassesKey[MAXPATHLEN];

    if (argc > 2 && _wcsicmp(argv[1], L"-check-one") == 0 && argc == 3) {
      i = _wtoi(argv[2]);
    }

    _snwprintf(sInputKey, MAXPATHLEN, L"input_%d", i);
    _snwprintf(sOutputKey, MAXPATHLEN, L"output_%d", i);
    _snwprintf(sPassesKey, MAXPATHLEN, L"passes_%d", i);

    if (!GetPrivateProfileStringW(L"MakeCommandLineTests", sInputKey, nsnull,
                                  sInputVal, MAXPATHLEN, inifile)) {
      if (i == 0 || argc > 2 && _wcsicmp(argv[1], L"-check-one") == 0) {
        if (argc > 1 && _wcsicmp(argv[1], L"-check-one") == 0 && argc == 3) {
          wprintf(L"\nERROR: either the TestXREMakeCommandLineWin.ini file doesn't exist\n");
          wprintf(L"       or the test is not defined in the MakeCommandLineTests section.\n\n");
          wprintf(L"File: %s\n", inifile);
        } else {
          wprintf(L"*** TEST-FAIL | %s\n", LOG_PREFIX);
          wprintf(L"*** Either the TestXREMakeCommandLineWin.ini file doesn't exist\n");
          wprintf(L"*** or it has no tests defined in the MakeCommandLineTests section.\n");
          wprintf(L"*** File: %s\n", inifile);
        }
        return 4;
      }
      break;
    }

    GetPrivateProfileStringW(L"MakeCommandLineTests", sOutputKey, nsnull,
                             sOutputVal, MAXPATHLEN, inifile);
    GetPrivateProfileStringW(L"MakeCommandLineTests", sPassesKey, nsnull,
                             sPassesVal, MAXPATHLEN, inifile);

    rv |= verifyCmdLineCreation(sInputVal, sOutputVal,
                                (_wcsicmp(sPassesVal, L"false") == 0) ? FALSE : TRUE,
                                i);

    if (argc > 2 && _wcsicmp(argv[1], L"-check-one") == 0) {
      break;
    }
  }

  if (rv == 0) {
    wprintf(L"*** TEST-PASS | %s | all tests passed\n", LOG_PREFIX);
  } else {
    wprintf(L"*** TEST-FAIL | %s | some tests failed\n", LOG_PREFIX);
  }

  return rv;
}
