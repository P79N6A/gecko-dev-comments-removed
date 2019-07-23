#include <stdio.h>
#include <Windows.h>


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
                 NULL,                   
                 0,                      
                 CrashingThread  ,       
                 NULL,                   
                 0,                      
                 &tid);                  
  return TRUE;
}
