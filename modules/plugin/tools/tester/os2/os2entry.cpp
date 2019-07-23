









































#include <os2.h>

#include "xp.h"
#include "guiprefs.h"
#include "loadstatus.h"
#include "os2utils.h"

HMODULE hInst = NULL;
HWND hWndLoadStatus = NULL;


extern "C" {
int  _CRT_init( void );
void _CRT_term( void );
void __ctordtorInit( void );
void __ctordtorTerm( void );
}

extern "C" unsigned long _System _DLL_InitTerm(unsigned long hModule, unsigned long ulFlag)
{
 APIRET rc; 

 switch (ulFlag) {
   case 0:
    
     if ( _CRT_init() == -1 ) {
       return 0UL;
     } else {
       __ctordtorInit();
       hInst = hModule;
       char szFileName[_MAX_PATH];
       GetINIFileName(hInst, szFileName, sizeof(szFileName));
       char sz[256];
       XP_GetPrivateProfileString(SECTION_PREFERENCES, KEY_LOADSTATUS_WINDOW, ENTRY_NO, sz, sizeof(sz), szFileName);
       if (stricmp(sz, ENTRY_YES) == 0)
         hWndLoadStatus = ShowLoadStatus("Tester dll is loaded");
     }
     break;
     
   case 1 :
     __ctordtorTerm();
     _CRT_term();
     hInst = NULLHANDLE;
     if (hWndLoadStatus)
       DestroyLoadStatus(hWndLoadStatus);

     break;
     
   default  :
     return 0UL;
 }

 return 1;
}


