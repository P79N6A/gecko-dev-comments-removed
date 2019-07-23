




































#ifndef NSDEFS_H
#define NSDEFS_H

#ifdef _DEBUG
#define INCL_WINERRORS
#endif
#define INCL_WIN
#define INCL_DOS
#define INCL_GPI
#include <os2.h>

#ifdef _DEBUG
  #define BREAK_TO_DEBUGGER           asm("int $3")
#else   
  #define BREAK_TO_DEBUGGER
#endif  

#ifdef _DEBUG
  #define VERIFY(exp)                 if (!(exp)) { WinGetLastError((HAB)0); BREAK_TO_DEBUGGER; }
#else   
  #define VERIFY(exp)                 (exp)
#endif  

#define WC_SCROLLBAR_STRING    "#8"  //string equivalent to WC_SCROLLBAR
#define WC_FRAME_STRING        "#1"  //string equivalent to WC_FRAME


extern "C" {
  PVOID  APIENTRY WinQueryProperty(HWND hwnd, PCSZ  pszNameOrAtom);

  PVOID  APIENTRY WinRemoveProperty(HWND hwnd, PCSZ  pszNameOrAtom);

  BOOL   APIENTRY WinSetProperty(HWND hwnd, PCSZ  pszNameOrAtom,
                                 PVOID pvData, ULONG ulFlags);

  APIRET APIENTRY DosQueryModFromEIP(HMODULE *phMod, ULONG *pObjNum,
                                     ULONG BuffLen,  PCHAR pBuff,
                                     ULONG *pOffset, ULONG Address);
}

#endif  


