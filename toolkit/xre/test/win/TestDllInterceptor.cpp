




































#include <stdio.h>
#include "nsWindowsDllInterceptor.h"

static bool patched_func_called = false;

static BOOL (*orig_GetVersionExA)(__inout LPOSVERSIONINFO);

static BOOL
patched_GetVersionExA(__inout LPOSVERSIONINFO lpVersionInfo)
{
  patched_func_called = true;
  return orig_GetVersionExA(lpVersionInfo);
}

bool osvi_equal(OSVERSIONINFO &info0, OSVERSIONINFO &info1)
{
  return (info0.dwMajorVersion == info1.dwMajorVersion &&
          info0.dwMinorVersion == info1.dwMinorVersion &&
          info0.dwBuildNumber == info1.dwBuildNumber &&
          info0.dwPlatformId == info1.dwPlatformId &&
          !strncmp(info0.szCSDVersion, info1.szCSDVersion, sizeof(info0.szCSDVersion)));
}

int wmain()
{
  OSVERSIONINFO info0, info1;
  ZeroMemory(&info0, sizeof(OSVERSIONINFO));
  ZeroMemory(&info1, sizeof(OSVERSIONINFO));
  info0.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
  info1.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

  GetVersionExA(&info0);

  {
    WindowsDllInterceptor Kernel32Intercept;
    Kernel32Intercept.Init("kernel32.dll");
    if (Kernel32Intercept.AddHook("GetVersionExA", reinterpret_cast<intptr_t>(patched_GetVersionExA), (void**) &orig_GetVersionExA)) {
      printf("TEST-PASS | WindowsDllInterceptor | Hook added\n");
    } else {
      printf("TEST-UNEXPECTED-FAIL | WindowsDllInterceptor | Failed to add hook\n");
      return 1;
    }

    GetVersionExA(&info1);

    if (patched_func_called) {
      printf("TEST-PASS | WindowsDllInterceptor | Hook called\n");
    } else {
      printf("TEST-UNEXPECTED-FAIL | WindowsDllInterceptor | Hook was not called\n");
      return 1;
    }

    if (osvi_equal(info0, info1)) {
      printf("TEST-PASS | WindowsDllInterceptor | Hook works properly\n");
    } else {
      printf("TEST-UNEXPECTED-FAIL | WindowsDllInterceptor | Hook didn't return the right information\n");
      return 1;
    }
  }

  patched_func_called = false;

  GetVersionExA(&info1);

  if (!patched_func_called) {
    printf("TEST-PASS | WindowsDllInterceptor | Hook was not called after unregistration\n");
  } else {
    printf("TEST-UNEXPECTED-FAIL | WindowsDllInterceptor | Hook was still called after unregistration\n");
    return 1;
  }

  if (osvi_equal(info0, info1)) {
    printf("TEST-PASS | WindowsDllInterceptor | Original function worked properly\n");
  } else {
    printf("TEST-UNEXPECTED-FAIL | WindowsDllInterceptor | Original function didn't return the right information\n");
    return 1;
  }

  printf("TEST-PASS | WindowsDllInterceptor | all checks passed\n");
  return 0;
}
