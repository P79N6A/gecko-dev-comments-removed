







































#if MOZ_WINSDK_TARGETVER >= MOZ_NTDDI_WIN7

#include "TaskbarTabPreview.h"
#include "nsWindowGfx.h"
#include "nsUXThemeData.h"
#include <nsITaskbarPreviewController.h>

#define TASKBARPREVIEW_HWNDID L"TaskbarTabPreviewHwnd"

namespace mozilla {
namespace widget {

NS_IMPL_ISUPPORTS1(TaskbarTabPreview, nsITaskbarTabPreview)

const PRUnichar *const kWindowClass = L"MozillaTaskbarPreviewClass";

TaskbarTabPreview::TaskbarTabPreview(ITaskbarList4 *aTaskbar, nsITaskbarPreviewController *aController, HWND aHWND, nsIDocShell *aShell)
  : TaskbarPreview(aTaskbar, aController, aHWND, aShell),
    mProxyWindow(NULL),
    mIcon(NULL),
    mRegistered(PR_FALSE)
{
  WindowHook &hook = GetWindowHook();
  hook.AddMonitor(WM_WINDOWPOSCHANGED, MainWindowHook, this);
}

TaskbarTabPreview::~TaskbarTabPreview() {
  if (mIcon) {
    ::DestroyIcon(mIcon);
    mIcon = NULL;
  }

  
  if (mProxyWindow)
    Disable();

  NS_ASSERTION(!mProxyWindow, "Taskbar proxy window was not destroyed!");

  if (IsWindowAvailable()) {
    DetachFromNSWindow();
  } else {
    mWnd = NULL;
  }
}

nsresult
TaskbarTabPreview::ShowActive(bool active) {
  NS_ASSERTION(mVisible && CanMakeTaskbarCalls(), "ShowActive called on invisible window or before taskbar calls can be made for this window");
  return FAILED(mTaskbar->SetTabActive(active ? mProxyWindow : NULL, mWnd, 0))
       ? NS_ERROR_FAILURE
       : NS_OK;
}

HWND &
TaskbarTabPreview::PreviewWindow() {
  return mProxyWindow;
}

nativeWindow
TaskbarTabPreview::GetHWND() {
  return mProxyWindow;
}

void
TaskbarTabPreview::EnsureRegistration() {
  NS_ASSERTION(mVisible && CanMakeTaskbarCalls(), "EnsureRegistration called when it is not safe to do so");

  (void) UpdateTaskbarProperties();
}

NS_IMETHODIMP
TaskbarTabPreview::GetTitle(nsAString &aTitle) {
  aTitle = mTitle;
  return NS_OK;
}

NS_IMETHODIMP
TaskbarTabPreview::SetTitle(const nsAString &aTitle) {
  mTitle = aTitle;
  return mVisible ? UpdateTitle() : NS_OK;
}

NS_IMETHODIMP
TaskbarTabPreview::SetIcon(imgIContainer *icon) {
  HICON hIcon = NULL;
  if (icon) {
    nsresult rv;
    rv = nsWindowGfx::CreateIcon(icon, PR_FALSE, 0, 0,
                                 nsWindowGfx::GetIconMetrics(nsWindowGfx::kSmallIcon),
                                 &hIcon);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (mIcon)
    ::DestroyIcon(mIcon);
  mIcon = hIcon;
  mIconImage = icon;
  return mVisible ? UpdateIcon() : NS_OK;
}

NS_IMETHODIMP
TaskbarTabPreview::GetIcon(imgIContainer **icon) {
  NS_IF_ADDREF(*icon = mIconImage);
  return NS_OK;
}

NS_IMETHODIMP
TaskbarTabPreview::Move(nsITaskbarTabPreview *aNext) {
  if (aNext == this)
    return NS_ERROR_INVALID_ARG;
  mNext = aNext;
  return CanMakeTaskbarCalls() ? UpdateNext() : NS_OK;
}

nsresult
TaskbarTabPreview::UpdateTaskbarProperties() {
  if (mRegistered)
    return NS_OK;

  if (FAILED(mTaskbar->RegisterTab(mProxyWindow, mWnd)))
    return NS_ERROR_FAILURE;

  nsresult rv = UpdateNext();
  NS_ENSURE_SUCCESS(rv, rv);
  rv = TaskbarPreview::UpdateTaskbarProperties();
  mRegistered = PR_TRUE;
  return rv;
}

LRESULT
TaskbarTabPreview::WndProc(UINT nMsg, WPARAM wParam, LPARAM lParam) {
  nsRefPtr<TaskbarTabPreview> kungFuDeathGrip(this);
  switch (nMsg) {
    case WM_CREATE:
      TaskbarPreview::EnableCustomDrawing(mProxyWindow, PR_TRUE);
      return 0;
    case WM_CLOSE:
      mController->OnClose();
      return 0;
    case WM_ACTIVATE:
      if (LOWORD(wParam) == WA_ACTIVE) {
        
        
        bool activateWindow;
        nsresult rv = mController->OnActivate(&activateWindow);
        if (NS_SUCCEEDED(rv) && activateWindow) {
          nsWindow* win = nsWindow::GetNSWindowPtr(mWnd);
          if (win) {
            nsWindow * parent = win->GetTopLevelWindow(true);
            if (parent) {
              parent->Show(true);
            }
          }
        }
      }
      return 0;
    case WM_GETICON:
      return (LRESULT)mIcon;
    case WM_SYSCOMMAND:
      
      
      if (wParam == SC_RESTORE || wParam == SC_MAXIMIZE) {
        bool activateWindow;
        nsresult rv = mController->OnActivate(&activateWindow);
        if (NS_SUCCEEDED(rv) && activateWindow) {
          
          
          
          ::SendMessageW(mWnd, WM_SYSCOMMAND, wParam, lParam);
        }
        return 0;
      }
      
      
      
      return wParam == SC_CLOSE
           ? ::DefWindowProcW(mProxyWindow, WM_SYSCOMMAND, wParam, lParam)
           : ::SendMessageW(mWnd, WM_SYSCOMMAND, wParam, lParam);
      return 0;
  }
  return TaskbarPreview::WndProc(nMsg, wParam, lParam);
}


LRESULT CALLBACK
TaskbarTabPreview::GlobalWndProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam) {
  TaskbarTabPreview *preview(nsnull);
  if (nMsg == WM_CREATE) {
    CREATESTRUCT *cs = reinterpret_cast<CREATESTRUCT*>(lParam);
    preview = reinterpret_cast<TaskbarTabPreview*>(cs->lpCreateParams);
    if (!::SetPropW(hWnd, TASKBARPREVIEW_HWNDID, preview))
      NS_ERROR("Could not associate native window with tab preview");
    preview->mProxyWindow = hWnd;
  } else {
    preview = reinterpret_cast<TaskbarTabPreview*>(::GetPropW(hWnd, TASKBARPREVIEW_HWNDID));
    if (nMsg == WM_DESTROY)
      ::RemovePropW(hWnd, TASKBARPREVIEW_HWNDID);
  }

  if (preview)
    return preview->WndProc(nMsg, wParam, lParam);
  return ::DefWindowProcW(hWnd, nMsg, wParam, lParam);
}

nsresult
TaskbarTabPreview::Enable() {
  WNDCLASSW wc;
  HINSTANCE module = GetModuleHandle(NULL);

  if (!GetClassInfoW(module, kWindowClass, &wc)) {
    wc.style         = 0;
    wc.lpfnWndProc   = GlobalWndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = module;
    wc.hIcon         = NULL;
    wc.hCursor       = NULL;
    wc.hbrBackground = (HBRUSH) NULL;
    wc.lpszMenuName  = (LPCWSTR) NULL;
    wc.lpszClassName = kWindowClass;
    RegisterClassW(&wc);
  }
  ::CreateWindowW(kWindowClass, L"TaskbarPreviewWindow",
                  WS_CAPTION | WS_SYSMENU, 0, 0, 200, 60, NULL, NULL, module, this);
  
  if (!mProxyWindow)
    return NS_ERROR_INVALID_ARG;

  UpdateProxyWindowStyle();

  nsresult rv = TaskbarPreview::Enable();
  nsresult rvUpdate;
  rvUpdate = UpdateTitle();
  if (NS_FAILED(rvUpdate))
    rv = rvUpdate;

  rvUpdate = UpdateIcon();
  if (NS_FAILED(rvUpdate))
    rv = rvUpdate;

  return rv;
}

nsresult
TaskbarTabPreview::Disable() {
  
  
  
  if (mWnd)
    TaskbarPreview::Disable();

  if (FAILED(mTaskbar->UnregisterTab(mProxyWindow)))
    return NS_ERROR_FAILURE;
  mRegistered = PR_FALSE;

  
  if (!DestroyWindow(mProxyWindow))
    return NS_ERROR_FAILURE;
  mProxyWindow = NULL;
  return NS_OK;
}

void
TaskbarTabPreview::DetachFromNSWindow() {
  (void) SetVisible(PR_FALSE);
  WindowHook &hook = GetWindowHook();
  hook.RemoveMonitor(WM_WINDOWPOSCHANGED, MainWindowHook, this);

  TaskbarPreview::DetachFromNSWindow();
}


bool
TaskbarTabPreview::MainWindowHook(void *aContext,
                                  HWND hWnd, UINT nMsg,
                                  WPARAM wParam, LPARAM lParam,
                                  LRESULT *aResult) {
  if (nMsg == WM_WINDOWPOSCHANGED) {
    TaskbarTabPreview *preview = reinterpret_cast<TaskbarTabPreview*>(aContext);
    WINDOWPOS *pos = reinterpret_cast<WINDOWPOS*>(lParam);
    if (SWP_FRAMECHANGED == (pos->flags & SWP_FRAMECHANGED))
      preview->UpdateProxyWindowStyle();
  } else {
    NS_NOTREACHED("Style changed hook fired on non-style changed message");
  }
  return PR_FALSE;
}

void
TaskbarTabPreview::UpdateProxyWindowStyle() {
  if (!mProxyWindow)
    return;

  DWORD minMaxMask = WS_MINIMIZE | WS_MAXIMIZE;
  DWORD windowStyle = GetWindowLongW(mWnd, GWL_STYLE);

  DWORD proxyStyle = GetWindowLongW(mProxyWindow, GWL_STYLE);
  proxyStyle &= ~minMaxMask;
  proxyStyle |= windowStyle & minMaxMask;
  SetWindowLongW(mProxyWindow, GWL_STYLE, proxyStyle);

  DWORD exStyle = (WS_MAXIMIZE == (windowStyle & WS_MAXIMIZE)) ? WS_EX_TOOLWINDOW : 0;
  SetWindowLongW(mProxyWindow, GWL_EXSTYLE, exStyle);
}

nsresult
TaskbarTabPreview::UpdateTitle() {
  NS_ASSERTION(mVisible, "UpdateTitle called on invisible preview");

  if (!::SetWindowTextW(mProxyWindow, mTitle.get()))
    return NS_ERROR_FAILURE;
  return NS_OK;
}

nsresult
TaskbarTabPreview::UpdateIcon() {
  NS_ASSERTION(mVisible, "UpdateIcon called on invisible preview");

  ::SendMessageW(mProxyWindow, WM_SETICON, ICON_SMALL, (LPARAM)mIcon);

  return NS_OK;
}

nsresult
TaskbarTabPreview::UpdateNext() {
  NS_ASSERTION(CanMakeTaskbarCalls() && mVisible, "UpdateNext called on invisible tab preview");
  HWND hNext = NULL;
  if (mNext) {
    bool visible;
    nsresult rv = mNext->GetVisible(&visible);

    NS_ENSURE_SUCCESS(rv, rv);

    
    if (!visible)
      return NS_ERROR_FAILURE;

    hNext = (HWND)mNext->GetHWND();

    
    mNext->EnsureRegistration();
  }
  if (FAILED(mTaskbar->SetTabOrder(mProxyWindow, hNext)))
    return NS_ERROR_FAILURE;
  return NS_OK;
}


} 
} 

#endif 
