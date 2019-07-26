





#include "PluginHangUI.h"

#include "PluginHangUIChild.h"
#include "HangUIDlg.h"

#include <assert.h>
#include <commctrl.h>
#include <windowsx.h>
#include <algorithm>
#include <sstream>
#include <vector>

namespace mozilla {
namespace plugins {

struct WinInfo
{
  WinInfo(HWND aHwnd, POINT& aPos, SIZE& aSize)
    :hwnd(aHwnd)
  {
    pos.x = aPos.x;
    pos.y = aPos.y;
    size.cx = aSize.cx;
    size.cy = aSize.cy;
  }
  HWND  hwnd;
  POINT pos;
  SIZE  size;
};
typedef std::vector<WinInfo> WinInfoVec;

PluginHangUIChild* PluginHangUIChild::sSelf = nullptr;
const int PluginHangUIChild::kExpectedMinimumArgc = 10;

PluginHangUIChild::PluginHangUIChild()
  : mResponseBits(0),
    mParentWindow(nullptr),
    mDlgHandle(nullptr),
    mMainThread(nullptr),
    mParentProcess(nullptr),
    mRegWaitProcess(nullptr),
    mIPCTimeoutMs(0)
{
}

PluginHangUIChild::~PluginHangUIChild()
{
  if (mMainThread) {
    CloseHandle(mMainThread);
  }
  if (mRegWaitProcess) {
    UnregisterWaitEx(mRegWaitProcess, INVALID_HANDLE_VALUE);
  }
  if (mParentProcess) {
    CloseHandle(mParentProcess);
  }
  sSelf = nullptr;
}

bool
PluginHangUIChild::Init(int aArgc, wchar_t* aArgv[])
{
  if (aArgc < kExpectedMinimumArgc) {
    return false;
  }
  unsigned int i = 1;
  mMessageText = aArgv[i];
  mWindowTitle = aArgv[++i];
  mWaitBtnText = aArgv[++i];
  mKillBtnText = aArgv[++i];
  mNoFutureText = aArgv[++i];
  std::wistringstream issHwnd(aArgv[++i]);
  issHwnd >> reinterpret_cast<HANDLE&>(mParentWindow);
  if (!issHwnd) {
    return false;
  }
  std::wistringstream issProc(aArgv[++i]);
  issProc >> mParentProcess;
  if (!issProc) {
    return false;
  }
  
  if (wcscmp(aArgv[++i], L"-")) {
    HMODULE shell32 = LoadLibrary(L"shell32.dll");
    if (shell32) {
      SETAPPUSERMODELID fSetAppUserModelID = (SETAPPUSERMODELID)
        GetProcAddress(shell32, "SetCurrentProcessExplicitAppUserModelID");
      if (fSetAppUserModelID) {
        fSetAppUserModelID(aArgv[i]);
      }
      FreeLibrary(shell32);
    }
  }
  std::wistringstream issTimeout(aArgv[++i]);
  issTimeout >> mIPCTimeoutMs;
  if (!issTimeout) {
    return false;
  }

  nsresult rv = mMiniShm.Init(this,
                              std::wstring(aArgv[++i]),
                              IsDebuggerPresent() ? INFINITE : mIPCTimeoutMs);
  if (NS_FAILED(rv)) {
    return false;
  }
  sSelf = this;
  return true;
}

void
PluginHangUIChild::OnMiniShmEvent(MiniShmBase* aMiniShmObj)
{
  const PluginHangUICommand* cmd = nullptr;
  nsresult rv = aMiniShmObj->GetReadPtr(cmd);
  assert(NS_SUCCEEDED(rv));
  bool returnStatus = false;
  if (NS_SUCCEEDED(rv)) {
    switch (cmd->mCode) {
      case PluginHangUICommand::HANGUI_CMD_SHOW:
        returnStatus = RecvShow();
        break;
      case PluginHangUICommand::HANGUI_CMD_CANCEL:
        returnStatus = RecvCancel();
        break;
      default:
        break;
    }
  }
}


INT_PTR CALLBACK
PluginHangUIChild::SHangUIDlgProc(HWND aDlgHandle, UINT aMsgCode,
                                  WPARAM aWParam, LPARAM aLParam)
{
  PluginHangUIChild *self = PluginHangUIChild::sSelf;
  if (self) {
    return self->HangUIDlgProc(aDlgHandle, aMsgCode, aWParam, aLParam);
  }
  return FALSE;
}

void
PluginHangUIChild::ResizeButtons()
{
  
  UINT ids[] = { IDC_STOP, IDC_CONTINUE };
  UINT numIds = sizeof(ids)/sizeof(ids[0]);

  
  bool needResizing = false;
  SIZE idealSize = {0};
  WinInfoVec winInfo;
  for (UINT i = 0; i < numIds; ++i) {
    HWND wnd = GetDlgItem(mDlgHandle, ids[i]);
    if (!wnd) {
      return;
    }

    
    RECT curRect;
    if (!GetWindowRect(wnd, &curRect)) {
      return;
    }

    
    POINT pt;
    pt.x = curRect.left;
    pt.y = curRect.top;
    if (!ScreenToClient(mDlgHandle, &pt)) {
      return;
    }

    
    RECT margins;
    if (!Button_GetTextMargin(wnd, &margins)) {
      return;
    }

    
    SIZE curSize;
    curSize.cx = curRect.right - curRect.left;
    curSize.cy = curRect.bottom - curRect.top;

    
    SIZE size = {0};
    if (!Button_GetIdealSize(wnd, &size)) {
      return;
    }
    size.cx += margins.left + margins.right;
    size.cy += margins.top + margins.bottom;

    
    idealSize.cx = std::max(idealSize.cx, size.cx);
    idealSize.cy = std::max(idealSize.cy, size.cy);

    
    if (idealSize.cx > curSize.cx) {
      needResizing = true;
    }

    
    
    winInfo.push_back(WinInfo(wnd, pt, curSize));
  }

  if (!needResizing) {
    return;
  }

  
  int deltaX = 0;
  HDWP hwp = BeginDeferWindowPos((int) winInfo.size());
  if (!hwp) {
    return;
  }
  for (WinInfoVec::const_iterator itr = winInfo.begin();
       itr != winInfo.end(); ++itr) {
    
    
    deltaX += idealSize.cx - itr->size.cx;
    hwp = DeferWindowPos(hwp, itr->hwnd, nullptr,
                         itr->pos.x - deltaX, itr->pos.y,
                         idealSize.cx, itr->size.cy,
                         SWP_NOZORDER | SWP_NOACTIVATE);
    if (!hwp) {
      return;
    }
  }
  EndDeferWindowPos(hwp);
}

INT_PTR
PluginHangUIChild::HangUIDlgProc(HWND aDlgHandle, UINT aMsgCode, WPARAM aWParam,
                                 LPARAM aLParam)
{
  mDlgHandle = aDlgHandle;
  switch (aMsgCode) {
    case WM_INITDIALOG: {
      
      
      RegisterWaitForSingleObject(&mRegWaitProcess,
                                  mParentProcess,
                                  &SOnParentProcessExit,
                                  this,
                                  INFINITE,
                                  WT_EXECUTEDEFAULT | WT_EXECUTEONLYONCE);
      SetWindowText(aDlgHandle, mWindowTitle);
      SetDlgItemText(aDlgHandle, IDC_MSG, mMessageText);
      SetDlgItemText(aDlgHandle, IDC_NOFUTURE, mNoFutureText);
      SetDlgItemText(aDlgHandle, IDC_CONTINUE, mWaitBtnText);
      SetDlgItemText(aDlgHandle, IDC_STOP, mKillBtnText);
      ResizeButtons();
      HANDLE icon = LoadImage(nullptr, IDI_QUESTION, IMAGE_ICON, 0, 0,
                              LR_DEFAULTSIZE | LR_SHARED);
      if (icon) {
        SendDlgItemMessage(aDlgHandle, IDC_DLGICON, STM_SETICON, (WPARAM)icon, 0);
      }
      EnableWindow(mParentWindow, FALSE);
      return TRUE;
    }
    case WM_CLOSE: {
      mResponseBits |= HANGUI_USER_RESPONSE_CANCEL;
      EndDialog(aDlgHandle, 0);
      SetWindowLongPtr(aDlgHandle, DWLP_MSGRESULT, 0);
      return TRUE;
    }
    case WM_COMMAND: {
      switch (LOWORD(aWParam)) {
        case IDC_CONTINUE:
          if (HIWORD(aWParam) == BN_CLICKED) {
            mResponseBits |= HANGUI_USER_RESPONSE_CONTINUE;
            EndDialog(aDlgHandle, 1);
            SetWindowLongPtr(aDlgHandle, DWLP_MSGRESULT, 0);
            return TRUE;
          }
          break;
        case IDC_STOP:
          if (HIWORD(aWParam) == BN_CLICKED) {
            mResponseBits |= HANGUI_USER_RESPONSE_STOP;
            EndDialog(aDlgHandle, 1);
            SetWindowLongPtr(aDlgHandle, DWLP_MSGRESULT, 0);
            return TRUE;
          }
          break;
        case IDC_NOFUTURE:
          if (HIWORD(aWParam) == BN_CLICKED) {
            if (Button_GetCheck(GetDlgItem(aDlgHandle,
                                           IDC_NOFUTURE)) == BST_CHECKED) {
              mResponseBits |= HANGUI_USER_RESPONSE_DONT_SHOW_AGAIN;
            } else {
              mResponseBits &=
                ~static_cast<DWORD>(HANGUI_USER_RESPONSE_DONT_SHOW_AGAIN);
            }
            SetWindowLongPtr(aDlgHandle, DWLP_MSGRESULT, 0);
            return TRUE;
          }
          break;
        default:
          break;
      }
      break;
    }
    case WM_DESTROY: {
      EnableWindow(mParentWindow, TRUE);
      SetForegroundWindow(mParentWindow);
      break;
    }
    default:
      break;
  }
  return FALSE;
}


VOID CALLBACK
PluginHangUIChild::SOnParentProcessExit(PVOID aObject, BOOLEAN aIsTimer)
{
  
  PluginHangUIChild* object = static_cast<PluginHangUIChild*>(aObject);
  object->RecvCancel();
}

bool
PluginHangUIChild::RecvShow()
{
  return (QueueUserAPC(&ShowAPC,
                       mMainThread,
                       reinterpret_cast<ULONG_PTR>(this)));
}

bool
PluginHangUIChild::Show()
{
  INT_PTR dlgResult = DialogBox(GetModuleHandle(nullptr),
                                MAKEINTRESOURCE(IDD_HANGUIDLG),
                                nullptr,
                                &SHangUIDlgProc);
  mDlgHandle = nullptr;
  assert(dlgResult != -1);
  bool result = false;
  if (dlgResult != -1) {
    PluginHangUIResponse* response = nullptr;
    nsresult rv = mMiniShm.GetWritePtr(response);
    if (NS_SUCCEEDED(rv)) {
      response->mResponseBits = mResponseBits;
      result = NS_SUCCEEDED(mMiniShm.Send());
    }
  }
  return result;
}


VOID CALLBACK
PluginHangUIChild::ShowAPC(ULONG_PTR aContext)
{
  PluginHangUIChild* object = reinterpret_cast<PluginHangUIChild*>(aContext);
  object->Show();
}

bool
PluginHangUIChild::RecvCancel()
{
  if (mDlgHandle) {
    PostMessage(mDlgHandle, WM_CLOSE, 0, 0);
  }
  return true;
}

bool
PluginHangUIChild::WaitForDismissal()
{
  if (!SetMainThread()) {
    return false;
  }
  DWORD waitResult = WaitForSingleObjectEx(mParentProcess,
                                           mIPCTimeoutMs,
                                           TRUE);
  return waitResult == WAIT_OBJECT_0 ||
         waitResult == WAIT_IO_COMPLETION;
}

bool
PluginHangUIChild::SetMainThread()
{
  if (mMainThread) {
    CloseHandle(mMainThread);
    mMainThread = nullptr;
  }
  mMainThread = OpenThread(THREAD_SET_CONTEXT,
                           FALSE,
                           GetCurrentThreadId());
  return !(!mMainThread);
}

} 
} 

#ifdef __MINGW32__
extern "C"
#endif
int
wmain(int argc, wchar_t *argv[])
{
  INITCOMMONCONTROLSEX icc = { sizeof(INITCOMMONCONTROLSEX),
                               ICC_STANDARD_CLASSES };
  if (!InitCommonControlsEx(&icc)) {
    return 1;
  }
  mozilla::plugins::PluginHangUIChild hangui;
  if (!hangui.Init(argc, argv)) {
    return 1;
  }
  if (!hangui.WaitForDismissal()) {
    return 1;
  }
  return 0;
}

