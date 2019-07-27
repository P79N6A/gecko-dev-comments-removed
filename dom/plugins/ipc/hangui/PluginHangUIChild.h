





#ifndef mozilla_plugins_PluginHangUIChild_h
#define mozilla_plugins_PluginHangUIChild_h

#include "MiniShmChild.h"

#include <string>

#include <windows.h>

namespace mozilla {
namespace plugins {










class PluginHangUIChild : public MiniShmObserver
{
public:
  PluginHangUIChild();
  virtual ~PluginHangUIChild();

  bool
  Init(int aArgc, wchar_t* aArgv[]);

  






  bool
  Show();

  






  bool
  WaitForDismissal();

  virtual void
  OnMiniShmEvent(MiniShmBase* aMiniShmObj) override;

private:
  bool
  RecvShow();

  bool
  RecvCancel();

  bool
  SetMainThread();

  void
  ResizeButtons();

  INT_PTR
  HangUIDlgProc(HWND aDlgHandle, UINT aMsgCode, WPARAM aWParam, LPARAM aLParam);

  static VOID CALLBACK
  ShowAPC(ULONG_PTR aContext);

  static INT_PTR CALLBACK
  SHangUIDlgProc(HWND aDlgHandle, UINT aMsgCode, WPARAM aWParam,
                 LPARAM aLParam);

  static VOID CALLBACK
  SOnParentProcessExit(PVOID aObject, BOOLEAN aIsTimer);

  static PluginHangUIChild *sSelf;

  const wchar_t* mMessageText;
  const wchar_t* mWindowTitle;
  const wchar_t* mWaitBtnText;
  const wchar_t* mKillBtnText;
  const wchar_t* mNoFutureText;
  unsigned int mResponseBits;
  HWND mParentWindow;
  HWND mDlgHandle;
  HANDLE mMainThread;
  HANDLE mParentProcess;
  HANDLE mRegWaitProcess;
  DWORD mIPCTimeoutMs;
  MiniShmChild mMiniShm;

  static const int kExpectedMinimumArgc;

  typedef HRESULT (WINAPI *SETAPPUSERMODELID)(PCWSTR);

  DISALLOW_COPY_AND_ASSIGN(PluginHangUIChild);
};

} 
} 

#endif 

