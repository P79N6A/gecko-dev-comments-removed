




































#include <windows.h>
#include <windowsx.h>
#include <assert.h>

#include "resource.h"

#include "plugin.h"
#include "utils.h"

static void onCommand(HWND hWnd, int id, HWND hWndCtl, UINT codeNotify)
{
  CPlugin * pPlugin = (CPlugin *)GetWindowLong(hWnd, DWL_USER);
  switch (id)
  {
    case IDC_GET_PLUGIN:
      DestroyWindow(hWnd);
      if(pPlugin !=NULL)
      {
        pPlugin->m_hWndDialog = NULL;
        pPlugin->getPlugin();
      }
      break;
    case IDCANCEL:
    case IDC_BUTTON_CANCEL:
      DestroyWindow(hWnd);
      if(pPlugin !=NULL)
        pPlugin->m_hWndDialog = NULL;
      break;
    default:
      break;
  }
}

static BOOL onInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam)
{
  CPlugin * pPlugin = (CPlugin *)lParam;
  assert(pPlugin != NULL);
  if(pPlugin == NULL)
    return TRUE;

  SetWindowLong(hWnd, DWL_USER, (LONG)pPlugin);

  pPlugin->m_hWndDialog = hWnd;
  
  wchar_t szString[512];
  LoadStringW(hInst, IDS_TITLE, szString, sizeof(szString));
  SetWindowTextW(hWnd, szString);

  LoadStringW(hInst, IDS_INFO, szString, sizeof(szString));
  SetDlgItemTextW(hWnd, IDC_STATIC_INFO, szString);

  
  SetDlgItemTextA(hWnd, IDC_STATIC_INFOTYPE, pPlugin->m_pNPMIMEType);

  LoadStringW(hInst, IDS_LOCATION, szString, sizeof(szString));
  SetDlgItemTextW(hWnd, IDC_STATIC_LOCATION, szString);

  char contentTypeIsJava = 0;

  if (NULL != pPlugin->m_pNPMIMEType) {
    contentTypeIsJava = (0 == strcmp("application/x-java-vm",
				     pPlugin->m_pNPMIMEType)) ? 1 : 0;
  }
  
  if(pPlugin->m_szPageURL == NULL || contentTypeIsJava)
    LoadStringW(hInst, IDS_FINDER_PAGE, szString, sizeof(szString));
  else
  {
    MultiByteToWideChar( CP_ACP, 0,
                         pPlugin->m_szPageURL,
                         strlen(pPlugin->m_szPageURL)+1,
                         szString,   
                         511 ); 
  }

  SetDlgItemTextWrapped(hWnd, IDC_STATIC_URL, szString);

  LoadStringW(hInst, IDS_QUESTION, szString, sizeof(szString));
  SetDlgItemTextW(hWnd, IDC_STATIC_QUESTION, szString);

  SetDlgItemTextW(hWnd, IDC_STATIC_WARNING, L"");

  if(!pPlugin->m_bOnline)
  {
    EnableWindow(GetDlgItem(hWnd, IDC_GET_PLUGIN), FALSE);
    LoadStringW(hInst, IDS_WARNING_OFFLINE, szString, sizeof(szString));
    SetDlgItemTextW(hWnd, IDC_STATIC_WARNING, szString);
    SetDlgItemTextW(hWnd, IDC_STATIC_QUESTION, L"");
    return TRUE;
  }

  if((!pPlugin->m_bJava) || (!pPlugin->m_bJavaScript) || (!pPlugin->m_bSmartUpdate))
  {
    LoadStringW(hInst, IDS_WARNING_JS, szString, sizeof(szString));
    SetDlgItemTextW(hWnd, IDC_STATIC_WARNING, szString);
    return TRUE;
  }

  ShowWindow(GetDlgItem(hWnd, IDC_STATIC_WARNING), SW_HIDE);

  RECT rc;
  GetWindowRect(GetDlgItem(hWnd, IDC_STATIC_WARNING), &rc);
  int iHeight = rc.bottom - rc.top;
  GetWindowRect(hWnd, &rc);
  SetWindowPos(hWnd, NULL, 0, 0, rc.right - rc.left, rc.bottom - rc.top - iHeight, SWP_NOZORDER | SWP_NOMOVE);

  HWND hWndQuestion = GetDlgItem(hWnd, IDC_STATIC_QUESTION);
  HWND hWndButtonGetPlugin = GetDlgItem(hWnd, IDC_GET_PLUGIN);
  HWND hWndButtonCancel = GetDlgItem(hWnd, IDC_BUTTON_CANCEL);

  POINT pt;

  GetWindowRect(hWndQuestion, &rc);
  pt.x = rc.left;
  pt.y = rc.top;
  ScreenToClient(hWnd, &pt);
  SetWindowPos(hWndQuestion, NULL, pt.x, pt.y - iHeight, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

  GetWindowRect(hWndButtonGetPlugin, &rc);
  pt.x = rc.left;
  pt.y = rc.top;
  ScreenToClient(hWnd, &pt);
  SetWindowPos(hWndButtonGetPlugin, NULL, pt.x, pt.y - iHeight, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

  GetWindowRect(hWndButtonCancel, &rc);
  pt.x = rc.left;
  pt.y = rc.top;
  ScreenToClient(hWnd, &pt);
  SetWindowPos(hWndButtonCancel, NULL, pt.x, pt.y - iHeight, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

  if(pPlugin->m_bHidden)
    SetForegroundWindow(hWnd);

  return TRUE;
}

static void onClose(HWND hWnd)
{
  DestroyWindow(hWnd);
}

static void onDestroy(HWND hWnd)
{
}

BOOL CALLBACK NP_LOADDS GetPluginDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch(uMsg)
  {
    case WM_INITDIALOG:
      return (BOOL)HANDLE_WM_INITDIALOG(hWnd, wParam, lParam, onInitDialog);
    case WM_COMMAND:
      HANDLE_WM_COMMAND(hWnd, wParam, lParam, onCommand);
      break;
    case WM_DESTROY:
      HANDLE_WM_DESTROY(hWnd, wParam, lParam, onDestroy);
      break;
    case WM_CLOSE:
      HANDLE_WM_CLOSE(hWnd, wParam, lParam, onClose);
      break;

    default:
      return FALSE;
  }
  return TRUE;
}
