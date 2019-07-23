








































#ifdef XP_WIN
#include <windows.h>
#include <windowsx.h>
#endif

#ifdef XP_MAC
#include <TextEdit.h>
#endif

#include "plugin.h"

CPlugin::CPlugin(NPP pNPInstance) :
  m_pNPInstance(pNPInstance),
  m_pNPStream(NULL),
  m_bInitialized(FALSE),
  m_pScriptablePeer(NULL)
{
#ifdef XP_WIN
  m_hWnd = NULL;
#endif

  const char *ua = NPN_UserAgent(m_pNPInstance);
  strcpy(m_String, ua);
}

CPlugin::~CPlugin()
{
  NS_IF_RELEASE(m_pScriptablePeer);
}

#ifdef XP_WIN
static LRESULT CALLBACK PluginWinProc(HWND, UINT, WPARAM, LPARAM);
static WNDPROC lpOldProc = NULL;
#endif

NPBool CPlugin::init(NPWindow* pNPWindow)
{
  if(pNPWindow == NULL)
    return FALSE;

#ifdef XP_WIN
  m_hWnd = (HWND)pNPWindow->window;
  if(m_hWnd == NULL)
    return FALSE;

  
  
  lpOldProc = SubclassWindow(m_hWnd, (WNDPROC)PluginWinProc);

  
  
  SetWindowLong(m_hWnd, GWL_USERDATA, (LONG)this);
#endif

  m_Window = pNPWindow;

  m_bInitialized = TRUE;
  return TRUE;
}

void CPlugin::shut()
{
#ifdef XP_WIN
  
  SubclassWindow(m_hWnd, lpOldProc);
  m_hWnd = NULL;
#endif

  m_bInitialized = FALSE;
}

NPBool CPlugin::isInitialized()
{
  return m_bInitialized;
}

int16 CPlugin::handleEvent(void* event)
{
#ifdef XP_MAC
    NPEvent* ev = (NPEvent*)event;
    if (m_Window) {
        Rect box = { m_Window->y, m_Window->x,
                     m_Window->y + m_Window->height, m_Window->x + m_Window->width };
        if (ev->what == updateEvt) {
            ::TETextBox(m_String, strlen(m_String), &box, teJustCenter);
        }
    }
#endif
    return 0;
}


void CPlugin::showVersion()
{
  const char *ua = NPN_UserAgent(m_pNPInstance);
  strcpy(m_String, ua);

#ifdef XP_WIN
  InvalidateRect(m_hWnd, NULL, TRUE);
  UpdateWindow(m_hWnd);
#endif

  if (m_Window) {
    NPRect r = { m_Window->y, m_Window->x,
                 m_Window->y + m_Window->height, m_Window->x + m_Window->width };
    NPN_InvalidateRect(m_pNPInstance, &r);
  }
}


void CPlugin::clear()
{
  strcpy(m_String, "");

#ifdef XP_WIN
  InvalidateRect(m_hWnd, NULL, TRUE);
  UpdateWindow(m_hWnd);
#endif
}

void CPlugin::getVersion(char* *aVersion)
{
  const char *ua = NPN_UserAgent(m_pNPInstance);
  char*& version = *aVersion;
  version = (char*)NPN_MemAlloc(1 + strlen(ua));
  if (version)
    strcpy(version, ua);
}






nsI4xScriptablePlugin* CPlugin::getScriptablePeer()
{
  if (!m_pScriptablePeer) {
    m_pScriptablePeer = new nsScriptablePeer(this);
    if(!m_pScriptablePeer)
      return NULL;

    NS_ADDREF(m_pScriptablePeer);
  }

  
  NS_ADDREF(m_pScriptablePeer);
  return m_pScriptablePeer;
}

#ifdef XP_WIN
static LRESULT CALLBACK PluginWinProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg) {
    case WM_PAINT:
      {
        
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        RECT rc;
        GetClientRect(hWnd, &rc);
        FrameRect(hdc, &rc, GetStockBrush(BLACK_BRUSH));
        CPlugin * p = (CPlugin *)GetWindowLong(hWnd, GWL_USERDATA);
        if(p)
          DrawText(hdc, p->m_String, strlen(p->m_String), &rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
        EndPaint(hWnd, &ps);
      }
      break;
    default:
      break;
  }

  return DefWindowProc(hWnd, msg, wParam, lParam);
}
#endif
