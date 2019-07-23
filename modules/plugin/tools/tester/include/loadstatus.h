




































#ifndef __LOADSTATUS_H__
#define __LOADSTATUS_H__

#if defined(XP_WIN) || defined(XP_OS2)

HWND ShowLoadStatus(char * aMessage);
void DestroyLoadStatus(HWND ahWnd);

#endif 

#endif 
