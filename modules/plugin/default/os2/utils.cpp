




































#define INCL_GPI
#define INCL_WIN
#include <os2.h>
#include <string.h>

#include "plugin.h"


BOOL IsNewMimeType(PSZ mime)   
{
  ULONG keysize = 512;
  char keybuf[512];

  PrfQueryProfileString(HINI_USERPROFILE, OS2INI_PLACE, mime, "", keybuf, keysize);
  if (keybuf[0] != '\0') {
    return FALSE;
  }
  else 
  {
    if (!(PrfWriteProfileString(HINI_USERPROFILE, OS2INI_PLACE, mime, "(none)")))
      WinMessageBox(HWND_DESKTOP, HWND_DESKTOP, "Error adding MIME type value", "Default Plugin", 0, MB_OK);
    return TRUE;
  }
}


static int getWindowStringLength(HWND hWnd, PSZ lpsz)
{
  HPS hPS = WinGetPS(hWnd);

  POINTL ptls[5];
  GpiQueryTextBox(hPS, strlen(lpsz), lpsz, 5, ptls);
  POINTL pt;
  pt.x = ptls[TXTBOX_CONCAT].x;
  pt.y = ptls[TXTBOX_TOPLEFT].y - ptls[TXTBOX_BOTTOMLEFT].y;
  WinReleasePS(hPS);
  return (int)pt.x;
}









void SetDlgItemTextWrapped(HWND hWnd, int iID, PSZ szText)
{
  HWND hWndStatic = WinWindowFromID(hWnd, iID);
  if(!szText || !*szText)
  {
    WinSetDlgItemText(hWnd, iID, "");
    return;
  }

  RECTL rc;
  WinQueryWindowRect(hWndStatic, &rc);

  int iStaticLength = rc.xRight - rc.xLeft;
  int iStringLength = getWindowStringLength(hWndStatic, szText);

  if(iStringLength <= iStaticLength)
  {
    WinSetDlgItemText(hWnd, iID, szText);
    return;
  }

  int iBreaks = iStringLength/iStaticLength;
  if(iBreaks <= 0)
    return;

  char * pBuf = new char[iStringLength + iBreaks + 1];
  if(pBuf == NULL)
    return;

  strcpy(pBuf, "");

  int iStart = 0;
  int iLines = 0;
  for(int i = 0; i < iStringLength; i++)
  {
    char * sz = &szText[iStart];
    int iIndex = i - iStart;
    char ch = sz[iIndex + 1];

    sz[iIndex + 1] = '\0';

    int iLength = getWindowStringLength(hWndStatic, sz);

    if(iLength < iStaticLength)
    {
      sz[iIndex + 1] = ch;
      if(iLines == iBreaks)
      {
        strcat(pBuf, sz);
        break;
      }
      continue;
    }

    sz[iIndex + 1] = ch;  
    i--;                  

    ch = sz[iIndex];
    sz[iIndex] = '\0';    

    strcat(pBuf, sz);    
    strcat(pBuf, " ");   

    iStart += strlen(sz);
    sz[iIndex] = ch;      
    iLines++;             
  }

  WinSetDlgItemText(hWnd, iID, pBuf);

  delete [] pBuf;
}
