





































#include  <stdio.h>
#include  <string.h>
#include "nscore.h"
#include "nsString.h"


#if defined (XP_WIN)
#include <windows.h>
#elif defined (XP_MAC)
#include <Dialogs.h>
#include <TextUtils.h>
#elif defined (XP_OS2)
#define INCL_DOS
#define INCL_WIN
#include <os2.h>
#endif

extern "C" void ShowOSAlert(const char* aMessage);



void ShowOSAlert(const char* aMessage)
{
#ifdef DEBUG_dbragg
printf("\n****Inside ShowOSAlert ***\n");	
#endif 

    const PRInt32 max_len = 255;
    PRInt32 input_len = strlen(aMessage);
    PRInt32 copy_len = (input_len > max_len) ? max_len : input_len;
#if defined (XP_WIN)
    NS_ConvertUTF8toUTF16 msg_str(aMessage, copy_len);
    PRUnichar* message_copy =  (PRUnichar*)msg_str.get();
    MessageBoxW(NULL, message_copy, NULL, MB_OK | MB_ICONERROR | MB_SETFOREGROUND );
#else
    char message_copy[max_len+1] = { 0 };
    strncpy(message_copy, aMessage, copy_len);
    message_copy[copy_len] = 0;
#endif
#if (XP_MAC)
    short buttonClicked;
    StandardAlert(kAlertStopAlert, c2pstr(message_copy), nil, nil, &buttonClicked);
#elif defined (XP_OS2)
    
    PPIB ppib;
    PTIB ptib;
    DosGetInfoBlocks(&ptib, &ppib);
    ppib->pib_ultype = 3;
    HAB hab = WinInitialize(0);
    HMQ hmq = WinCreateMsgQueue(hmq,0);
    WinMessageBox( HWND_DESKTOP, HWND_DESKTOP, message_copy, "", 0, MB_OK);
    WinDestroyMsgQueue(hmq);
    WinTerminate(hab);
#endif
    
    
    fprintf(stdout, "%s\n", aMessage);
}
