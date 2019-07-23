







































#if MOZ_WINSDK_TARGETVER >= MOZ_NTDDI_WIN7

#include <nsITaskbarPreviewController.h>
#include "TaskbarWindowPreview.h"
#include "WindowHook.h"
#include "nsUXThemeData.h"
#include "TaskbarPreviewButton.h"
#include "nsWindow.h"

namespace mozilla {
namespace widget {

namespace {
PRBool WindowHookProc(void *aContext, HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam, LRESULT *aResult)
{
  TaskbarWindowPreview *preview = reinterpret_cast<TaskbarWindowPreview*>(aContext);
  *aResult = preview->WndProc(nMsg, wParam, lParam);
  return PR_TRUE;
}
}

NS_IMPL_ISUPPORTS2(TaskbarWindowPreview, nsITaskbarWindowPreview, nsISupportsWeakReference)

TaskbarWindowPreview::TaskbarWindowPreview(ITaskbarList4 *aTaskbar, nsITaskbarPreviewController *aController, HWND aHWND, nsIDocShell *aShell)
  : TaskbarPreview(aTaskbar, aController, aHWND, aShell),
    mCustomDrawing(PR_FALSE),
    mHaveButtons(PR_FALSE)
{
  
  (void) SetVisible(PR_TRUE);

  memset(mThumbButtons, 0, sizeof mThumbButtons);
  for (PRInt32 i = 0; i < nsITaskbarWindowPreview::NUM_TOOLBAR_BUTTONS; i++) {
    mThumbButtons[i].dwMask = THB_FLAGS | THB_ICON | THB_TOOLTIP;
    mThumbButtons[i].iId = i;
    mThumbButtons[i].dwFlags = THBF_HIDDEN;
  }

}

TaskbarWindowPreview::~TaskbarWindowPreview() {
  if (mWnd)
    DetachFromNSWindow(PR_TRUE);
}

nsresult
TaskbarWindowPreview::ShowActive(PRBool active) {
  return FAILED(mTaskbar->ActivateTab(active ? mWnd : NULL))
       ? NS_ERROR_FAILURE
       : NS_OK;

}

HWND &
TaskbarWindowPreview::PreviewWindow() {
  return mWnd;
}

nsresult
TaskbarWindowPreview::GetButton(PRUint32 index, nsITaskbarPreviewButton **_retVal) {
  if (index >= nsITaskbarWindowPreview::NUM_TOOLBAR_BUTTONS)
    return NS_ERROR_INVALID_ARG;

  nsCOMPtr<nsITaskbarPreviewButton> button(do_QueryReferent(mWeakButtons[index]));

  if (!button) {
    
    button = new TaskbarPreviewButton(this, index);
    if (!button) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    mWeakButtons[index] = do_GetWeakReference(button);
  }

  if (!mHaveButtons) {
    mHaveButtons = PR_TRUE;

    WindowHook &hook = GetWindowHook();
    (void) hook.AddHook(WM_COMMAND, WindowHookProc, this);

    if (mVisible && FAILED(mTaskbar->ThumbBarAddButtons(mWnd, nsITaskbarWindowPreview::NUM_TOOLBAR_BUTTONS, mThumbButtons))) {
      return NS_ERROR_FAILURE;
    }
  }
  button.forget(_retVal);
  return NS_OK;
}

NS_IMETHODIMP
TaskbarWindowPreview::SetEnableCustomDrawing(PRBool aEnable) {
  if (aEnable == mCustomDrawing)
    return NS_OK;
  mCustomDrawing = aEnable;
  TaskbarPreview::EnableCustomDrawing(mWnd, aEnable);

  WindowHook &hook = GetWindowHook();
  if (aEnable) {
    (void) hook.AddHook(WM_DWMSENDICONICTHUMBNAIL, WindowHookProc, this);
    (void) hook.AddHook(WM_DWMSENDICONICLIVEPREVIEWBITMAP, WindowHookProc, this);
  } else {
    (void) hook.RemoveHook(WM_DWMSENDICONICLIVEPREVIEWBITMAP, WindowHookProc, this);
    (void) hook.RemoveHook(WM_DWMSENDICONICTHUMBNAIL, WindowHookProc, this);
  }
  return NS_OK;
}

NS_IMETHODIMP
TaskbarWindowPreview::GetEnableCustomDrawing(PRBool *aEnable) {
  *aEnable = mCustomDrawing;
  return NS_OK;
}

nsresult
TaskbarWindowPreview::UpdateTaskbarProperties() {
  if (mHaveButtons) {
    if (FAILED(mTaskbar->ThumbBarAddButtons(mWnd, nsITaskbarWindowPreview::NUM_TOOLBAR_BUTTONS, mThumbButtons)))
      return NS_ERROR_FAILURE;
  }
  return TaskbarPreview::UpdateTaskbarProperties();
}

LRESULT
TaskbarWindowPreview::WndProc(UINT nMsg, WPARAM wParam, LPARAM lParam) {
  nsRefPtr<TaskbarWindowPreview> kungFuDeathGrip(this);
  switch (nMsg) {
    case WM_COMMAND:
      {
        PRUint32 id = LOWORD(wParam);
        PRUint32 index = id;
        nsCOMPtr<nsITaskbarPreviewButton> button;
        nsresult rv = GetButton(index, getter_AddRefs(button));
        if (NS_SUCCEEDED(rv))
          mController->OnClick(button);
      }
      return 0;
  }
  return TaskbarPreview::WndProc(nMsg, wParam, lParam);
}

nsresult
TaskbarWindowPreview::Enable() {
  nsresult rv = TaskbarPreview::Enable();
  NS_ENSURE_SUCCESS(rv, rv);

  return FAILED(mTaskbar->AddTab(mWnd))
       ? NS_ERROR_FAILURE
       : NS_OK;
}

nsresult
TaskbarWindowPreview::Disable() {
  nsresult rv = TaskbarPreview::Disable();
  NS_ENSURE_SUCCESS(rv, rv);

  return FAILED(mTaskbar->DeleteTab(mWnd))
       ? NS_ERROR_FAILURE
       : NS_OK;
}

void
TaskbarWindowPreview::DetachFromNSWindow(PRBool windowIsAlive) {
  if (windowIsAlive) {
    
    SetEnableCustomDrawing(PR_FALSE);

    WindowHook &hook = GetWindowHook();
    (void) hook.RemoveHook(WM_COMMAND, WindowHookProc, this);
  }

  TaskbarPreview::DetachFromNSWindow(windowIsAlive);
}

nsresult
TaskbarWindowPreview::UpdateButtons() {
  NS_ASSERTION(mVisible, "UpdateButtons called on invisible preview");

  if (FAILED(mTaskbar->ThumbBarUpdateButtons(mWnd, nsITaskbarWindowPreview::NUM_TOOLBAR_BUTTONS, mThumbButtons)))
    return NS_ERROR_FAILURE;
  return NS_OK;
}

nsresult
TaskbarWindowPreview::UpdateButton(PRUint32 index) {
  if (index >= nsITaskbarWindowPreview::NUM_TOOLBAR_BUTTONS)
    return NS_ERROR_INVALID_ARG;
  if (mVisible) {
    if (FAILED(mTaskbar->ThumbBarUpdateButtons(mWnd, 1, &mThumbButtons[index])))
      return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

} 
} 

#endif 

