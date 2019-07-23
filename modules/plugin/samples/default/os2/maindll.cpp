




































#include <os2.h>

HMODULE hInst; 


extern "C" {
int  _CRT_init( void );
void _CRT_term( void );
void __ctordtorInit( void );
void __ctordtorTerm( void );
}

extern "C" unsigned long _System _DLL_InitTerm(unsigned long hModule,
                                               unsigned long ulFlag)
{
  switch (ulFlag) {
  case 0 :
    
    if ( _CRT_init() == -1 ) {
      return 0UL;
    } else {
      __ctordtorInit();
      hInst = hModule;
    }
    break;
    
  case 1 :
    __ctordtorTerm();
    _CRT_term();
    hInst = NULLHANDLE;
    break;
    
  default  :
    return 0UL;
  }

  return 1;
}
