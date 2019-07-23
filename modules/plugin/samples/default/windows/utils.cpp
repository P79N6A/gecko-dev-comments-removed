




































#include <windows.h>
#include <windowsx.h>

#include "plugin.h"


HKEY openRegistry()      
{
  HKEY phkResult;

  if(RegCreateKeyW(HKEY_CURRENT_USER, REGISTRY_PLACE, &phkResult) != ERROR_SUCCESS)
    MessageBoxW(0, L"Error creating Default Plugin registry key", L"Default Plugin", MB_OK);

  return phkResult;
}


BOOL IsNewMimeType(LPSTR mime)   
{
  HKEY hkey = openRegistry();
  DWORD dwType, keysize = 512;
  wchar_t keybuf[512];
  wchar_t wideMime[64];

  MultiByteToWideChar(CP_ACP, 0, 
                      mime,
                      strlen(mime) + 1, 
                      wideMime, 
                      64);
  
  if(RegQueryValueExW(hkey, wideMime, 0, &dwType, (LPBYTE) &keybuf, &keysize) == ERROR_SUCCESS)
  {
    
    return FALSE;
  }
  else 
  {
    if(RegSetValueExW(hkey, wideMime, 0,  REG_SZ, (LPBYTE) L"(none)", 7) != ERROR_SUCCESS)
      MessageBoxW(0, L"Error adding MIME type value", L"Default Plugin", MB_OK);

    return TRUE;
  }
}


static int getWindowStringLength(HWND hWnd, wchar_t* lpsz)
{
  SIZE sz;
  HDC hDC = GetDC(hWnd);
  HFONT hWindowFont = GetWindowFont(hWnd);
  HFONT hFontOld = SelectFont(hDC, hWindowFont);
  GetTextExtentPoint32W(hDC, lpsz, wcslen(lpsz), &sz);
  POINT pt;
  pt.x = sz.cx;
  pt.y = sz.cy;
  LPtoDP(hDC, &pt, 1);
  SelectFont(hDC, hFontOld);
  ReleaseDC(hWnd, hDC);
  return (int)pt.x;
}









void SetDlgItemTextWrapped(HWND hWnd, int iID, wchar_t* szText)
{
  HWND hWndStatic = GetDlgItem(hWnd, iID);
  if((szText == NULL) || (wcslen(szText) == 0))
  {
    SetDlgItemTextW(hWnd, iID, L"");
    return;
  }

  RECT rc;
  GetClientRect(hWndStatic, &rc);

  int iStaticLength = rc.right - rc.left;
  int iStringLength = getWindowStringLength(hWndStatic, szText);

  if(iStringLength <= iStaticLength)
  {
    SetDlgItemTextW(hWnd, iID, szText);
    return;
  }

  int iBreaks = iStringLength/iStaticLength;
  if(iBreaks <= 0)
    return;

  wchar_t * pBuf = new wchar_t[iStringLength + iBreaks + 1];
  if(pBuf == NULL)
    return;

  wcscpy(pBuf, L"");

  int iStart = 0;
  int iLines = 0;
  for(int i = 0; i < iStringLength; i++)
  {
    wchar_t* sz = &szText[iStart];
    int iIndex = i - iStart;
    wchar_t ch = sz[iIndex + 1];

    sz[iIndex + 1] = '\0';

    int iLength = getWindowStringLength(hWndStatic, sz);

    if(iLength < iStaticLength)
    {
      sz[iIndex + 1] = ch;
      if(iLines == iBreaks)
      {
        wcscat(pBuf, sz);
        break;
      }
      continue;
    }

    sz[iIndex + 1] = ch;  
    i--;                  

    ch = sz[iIndex];
    sz[iIndex] = '\0';    

    wcscat(pBuf, sz);     
    wcscat(pBuf, L" ");   

    iStart += wcslen(sz); 
    sz[iIndex] = ch;      
    iLines++;             
  }

  SetDlgItemTextW(hWnd, iID, pBuf);

  delete [] pBuf;
}
