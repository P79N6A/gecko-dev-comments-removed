



































#ifndef _nswidgetdefs_h
#define _nswidgetdefs_h



#include "nsIWidget.h"

#define INCL_PM
#define INCL_NLS
#define INCL_DOS
#define INCL_WINSTDFILE
#define INCL_DOSERRORS
#include <os2.h>




#include <uconv.h>
#define UNICHAR_TYPE_DEFINED
#include <unikbd.h> 

#ifndef MAX_PATH
#define MAX_PATH CCHMAXPATH
#endif

#ifndef OPENFILENAME



#define OFN_READONLY                 0x00000001
#define OFN_OVERWRITEPROMPT          0x00000002
#define OFN_HIDEREADONLY             0x00000004
#define OFN_NOCHANGEDIR              0x00000008
#define OFN_SHOWHELP                 0x00000010
#define OFN_ENABLEHOOK               0x00000020
#define OFN_ENABLETEMPLATE           0x00000040
#define OFN_ENABLETEMPLATEHANDLE     0x00000080
#define OFN_NOVALIDATE               0x00000100
#define OFN_ALLOWMULTISELECT         0x00000200
#define OFN_EXTENSIONDIFFERENT       0x00000400
#define OFN_PATHMUSTEXIST            0x00000800
#define OFN_FILEMUSTEXIST            0x00001000
#define OFN_CREATEPROMPT             0x00002000
#define OFN_SHAREAWARE               0x00004000
#define OFN_NOREADONLYRETURN         0x00008000
#define OFN_NOTESTFILECREATE         0x00010000
#define OFN_NONETWORKBUTTON          0x00020000
#define OFN_NOLONGNAMES              0x00040000

typedef struct _tagOFN {
   ULONG                             lStructSize;
   HWND                              hwndOwner;
   HMODULE                           hInstance;
   PCSZ                              lpstrFilter;
   PSZ                               lpstrCustomFilter;
   ULONG                             nMaxCustFilter;
   ULONG                             nFilterIndex;
   PSZ                               lpstrFile;
   ULONG                             nMaxFile;
   PSZ                               lpstrFileTitle;
   ULONG                             nMaxFileTitle;
   PCSZ                              lpstrInitialDir;
   PCSZ                              lpstrTitle;
   ULONG                             Flags;
   USHORT                            nFileOffset;
   USHORT                            nFileExtension;
   PCSZ                              lpstrDefExt;
   ULONG                             lCustData;
   PFN                               lpfnHook;
   PCSZ                              lpTemplateName;
} OPENFILENAME, *POPENFILENAME, *LPOPENFILENAME;

extern "C" BOOL APIENTRY DaxOpenSave(BOOL, LONG *, LPOPENFILENAME, PFNWP);
#endif

class nsDragService;
class nsIAppShell;


#define WMU_CALLMETHOD   (WM_USER + 1)
#define WMU_SENDMSG      (WM_USER + 2)


#define WMU_SHOW_TOOLTIP (WM_USER + 3)


#define WMU_HIDE_TOOLTIP (WM_USER + 4)










#define WMU_GETFLAVOURLEN  (WM_USER + 5)

typedef struct _WZDROPXFER
{
   ATOM hAtomFlavour;
   CHAR data[1];
} WZDROPXFER, *PWZDROPXFER;









#define WMU_GETFLAVOURDATA (WM_USER + 6)

#define WinIsKeyDown(vk) ((WinGetKeyState(HWND_DESKTOP,vk) & 0x8000) ? PR_TRUE : PR_FALSE)





#ifndef WM_MOUSEENTER
#define WM_MOUSEENTER   0x041E
#endif
#ifndef WM_MOUSELEAVE
#define WM_MOUSELEAVE   0x041F
#endif

#ifndef WM_FOCUSCHANGED
#define WM_FOCUSCHANGED 0x000E
#endif

#ifndef FCF_CLOSEBUTTON 
#define FCF_CLOSEBUTTON 0x04000000L
#endif

#ifndef FCF_DIALOGBOX
#define FCF_DIALOGBOX   0x40000000L
#endif

#ifndef DRT_URL
#define DRT_URL "UniformResourceLocator"
#endif

#define BASE_CONTROL_STYLE WS_TABSTOP

#define NS_MIT_END ((const PRUint32) MIT_END)





#define NS_DECL_LABEL                           \
   NS_IMETHOD SetLabel( const nsString &aText); \
   NS_IMETHOD GetLabel( nsString &aBuffer);

#define NS_IMPL_LABEL(_clsname)                        \
   nsresult _clsname::SetLabel( const nsString &aText) \
   {                                                   \
      SetTitle( aText);                                \
      return NS_OK;                                    \
   }                                                   \
                                                       \
   nsresult _clsname::GetLabel( nsString &aBuffer)     \
   {                                                   \
      PRUint32 dummy;                                  \
      GetWindowText( aBuffer, &dummy);                 \
      return NS_OK;                                    \
   }


#define lastchar(s) *((s) + strlen((s)) - 1)

#endif
