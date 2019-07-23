


































#ifndef nst_wince_h___
#define nst_wince_h___

#ifndef __WINDOWS__
#include <windows.h>
#endif












#ifdef WINCE

#ifdef perror
#undef perror
#endif
#define perror(x)





BOOL WINAPI DllMain( HINSTANCE hDllHandle, 
                     DWORD     nReason, 
                     LPVOID    lpvReserved )
{
  BOOLEAN bSuccess = TRUE;

  switch ( nReason )
  {
    case DLL_PROCESS_ATTACH:
      
      break;

    case DLL_THREAD_ATTACH:
      
      break;

    case DLL_THREAD_DETACH:
      
      break;

    case DLL_PROCESS_DETACH:
      
      break;

    default:
      
      break;
  }

  return bSuccess;
}



#define main __declspec(dllexport) WINAPI nspr_test_runme

#undef getcwd
#define getcwd(x, nc) \
{ \
  int i; \
  unsigned short dir[MAX_PATH]; \
  GetModuleFileName(GetModuleHandle (NULL), dir, MAX_PATH); \
  for (i = _tcslen(dir); i && dir[i] != TEXT('\\'); i--) {} \
  dir[i + 1] = L'\0'; \
  WideCharToMultiByte(CP_ACP, 0, dir, -1, x, nc, NULL, NULL); \
}

#endif  

#endif 
