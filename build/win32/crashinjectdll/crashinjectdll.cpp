



#include <stdio.h>
#include <windows.h>


DWORD tid = -1;

DWORD WINAPI CrashingThread(
  LPVOID lpParameter
)
{
  
  volatile int* x = (int *)0x0;
  *x = 1;
  return 0;
}

BOOL WINAPI DllMain(
  HANDLE hinstDLL,
  DWORD dwReason,
  LPVOID lpvReserved
)
{
  if (tid == -1)
    
    
    CreateThread(
                 nullptr,                
                 0,                      
                 CrashingThread,         
                 nullptr,                
                 0,                      
                 &tid);                  
  return TRUE;
}
