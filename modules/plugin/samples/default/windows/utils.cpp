




































#include <windows.h>
#include <windowsx.h>

#include "plugin.h"


HKEY openRegistry()      
{
  HKEY phkResult;

  if(RegCreateKey(HKEY_CURRENT_USER, REGISTRY_PLACE, &phkResult) != ERROR_SUCCESS)
    MessageBox(0, "Error creating Default Plugin registry key", "Default Plugin", MB_OK);

  return phkResult;
}


BOOL IsNewMimeType(LPSTR mime)   
{
  HKEY hkey = openRegistry();
  DWORD dwType, keysize = 512;
  char keybuf[512];

  if(RegQueryValueEx(hkey, mime, 0, &dwType, (LPBYTE) &keybuf, &keysize) == ERROR_SUCCESS)
  {
    
    return FALSE;
  }
  else 
  {
    if(RegSetValueEx(hkey, mime, 0,  REG_SZ, (LPBYTE) "(none)", 7) != ERROR_SUCCESS)
      MessageBox(0, "Error adding MIME type value", "Default Plugin", MB_OK);

    return TRUE;
  }
}


static int getWindowStringLength(HWND hWnd, LPSTR lpsz)
{
  SIZE sz;
  HDC hDC = GetDC(hWnd);
  HFONT hWindowFont = GetWindowFont(hWnd);
  HFONT hFontOld = SelectFont(hDC, hWindowFont);
  GetTextExtentPoint32(hDC, lpsz, lstrlen(lpsz), &sz);
  POINT pt;
  pt.x = sz.cx;
  pt.y = sz.cy;
  LPtoDP(hDC, &pt, 1);
  SelectFont(hDC, hFontOld);
  ReleaseDC(hWnd, hDC);
  return (int)pt.x;
}









void SetDlgItemTextWrapped(HWND hWnd, int iID, LPSTR szText)
{
  HWND hWndStatic = GetDlgItem(hWnd, iID);
  if((szText == NULL) || (lstrlen(szText) == 0))
  {
    SetDlgItemText(hWnd, iID, "");
    return;
  }

  RECT rc;
  GetClientRect(hWndStatic, &rc);

  int iStaticLength = rc.right - rc.left;
  int iStringLength = getWindowStringLength(hWndStatic, szText);

  if(iStringLength <= iStaticLength)
  {
    SetDlgItemText(hWnd, iID, szText);
    return;
  }

  int iBreaks = iStringLength/iStaticLength;
  if(iBreaks <= 0)
    return;

  char * pBuf = new char[iStringLength + iBreaks + 1];
  if(pBuf == NULL)
    return;

  lstrcpy(pBuf, "");

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
        lstrcat(pBuf, sz);
        break;
      }
      continue;
    }

    sz[iIndex + 1] = ch;  
    i--;                  

    ch = sz[iIndex];
    sz[iIndex] = '\0';    

    lstrcat(pBuf, sz);    
    lstrcat(pBuf, " ");   

    iStart += lstrlen(sz);
    sz[iIndex] = ch;      
    iLines++;             
  }

  SetDlgItemText(hWnd, iID, pBuf);

  delete [] pBuf;
}
