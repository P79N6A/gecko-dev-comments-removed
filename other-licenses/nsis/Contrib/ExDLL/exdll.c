

#include <windows.h>
#include "tchar.h"
#include "exdll.h"

HINSTANCE g_hInstance;

HWND g_hwndParent;




void __declspec(dllexport) myFunction(HWND hwndParent, int string_size, 
                                      TCHAR *variables, stack_t **stacktop,
                                      extra_parameters *extra)
{
  g_hwndParent=hwndParent;

  EXDLL_INIT();


  
  
  
  
  
  

  
  {
    TCHAR buf[1024];
    wsprintf(buf,_T("$0=%s\n"),getuservariable(INST_0));
    MessageBox(g_hwndParent,buf,0,MB_OK);
  }
}



BOOL WINAPI DllMain(HANDLE hInst, ULONG ul_reason_for_call, LPVOID lpReserved)
{
  g_hInstance=hInst;
	return TRUE;
}
