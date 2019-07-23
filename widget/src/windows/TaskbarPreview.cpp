







































#if MOZ_WINSDK_TARGETVER >= MOZ_NTDDI_WIN7

#include "TaskbarPreview.h"
#include <nsITaskbarPreviewController.h>
#include <windows.h>

#include <nsError.h>
#include <nsCOMPtr.h>
#include <nsIWidget.h>
#include <nsIBaseWindow.h>
#include <nsIObserverService.h>
#include <nsServiceManagerUtils.h>

#include "nsUXThemeData.h"
#include "nsWindow.h"
#include "nsAppShell.h"
#include "TaskbarPreviewButton.h"

#include <nsIBaseWindow.h>
#include <nsICanvasRenderingContextInternal.h>
#include <nsIDOMCanvasRenderingContext2D.h>
#include <imgIContainer.h>
#include <nsIDocShell.h>


#define DWM_SIT_DISPLAYFRAME 0x1

namespace mozilla {
namespace widget {

namespace {









nsresult
CreateRenderingContext(nsIDocShell *shell, gfxASurface *surface, PRUint32 width, PRUint32 height, nsICanvasRenderingContextInternal **aCtx) {
  nsresult rv;
  nsCOMPtr<nsICanvasRenderingContextInternal> ctx(do_CreateInstance(
    "@mozilla.org/content/canvas-rendering-context;1?id=2d", &rv));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = ctx->InitializeWithSurface(shell, surface, width, height);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*aCtx = ctx);
  return NS_OK;
}

}

TaskbarPreview::TaskbarPreview(ITaskbarList4 *aTaskbar, nsITaskbarPreviewController *aController, HWND aHWND, nsIDocShell *aShell)
  : mTaskbar(aTaskbar),
    mController(aController),
    mWnd(aHWND),
    mVisible(PR_FALSE),
    mDocShell(do_GetWeakReference(aShell))
{
  
  ::CoInitialize(NULL);

  WindowHook &hook = GetWindowHook();
  hook.AddMonitor(WM_DESTROY, MainWindowHook, this);
}

TaskbarPreview::~TaskbarPreview() {
  
  if (sActivePreview == this)
    sActivePreview = nsnull;

  
  if (mWnd)
    DetachFromNSWindow(PR_TRUE);

  
  mTaskbar = NULL;

  ::CoUninitialize();
}

NS_IMETHODIMP
TaskbarPreview::SetController(nsITaskbarPreviewController *aController) {
  NS_ENSURE_ARG(aController);

  mController = aController;
  return NS_OK;
}

NS_IMETHODIMP
TaskbarPreview::GetController(nsITaskbarPreviewController **aController) {
  NS_ADDREF(*aController = mController);
  return NS_OK;
}

NS_IMETHODIMP
TaskbarPreview::GetTooltip(nsAString &aTooltip) {
  aTooltip = mTooltip;
  return NS_OK;
}

NS_IMETHODIMP
TaskbarPreview::SetTooltip(const nsAString &aTooltip) {
  return NS_OK;
  mTooltip = aTooltip;
  return CanMakeTaskbarCalls() ? UpdateTooltip() : NS_OK;
}

NS_IMETHODIMP
TaskbarPreview::SetVisible(PRBool visible) {
  if (mVisible == visible) return NS_OK;
  mVisible = visible;

  return visible ? Enable() : Disable();
}

NS_IMETHODIMP
TaskbarPreview::GetVisible(PRBool *visible) {
  *visible = mVisible;
  return NS_OK;
}

NS_IMETHODIMP
TaskbarPreview::SetActive(PRBool active) {
  if (active)
    sActivePreview = this;
  else if (sActivePreview == this)
    sActivePreview = NULL;

  return CanMakeTaskbarCalls() ? ShowActive(active) : NS_OK;
}

NS_IMETHODIMP
TaskbarPreview::GetActive(PRBool *active) {
  *active = sActivePreview == this;
  return NS_OK;
}

NS_IMETHODIMP
TaskbarPreview::Invalidate() {
  if (!mVisible)
    return NS_ERROR_FAILURE;

  
  if (!nsUXThemeData::CheckForCompositor())
    return NS_OK;

  HWND previewWindow = PreviewWindow();
  return FAILED(nsUXThemeData::dwmInvalidateIconicBitmapsPtr(previewWindow))
       ? NS_ERROR_FAILURE
       : NS_OK;
}

nsresult
TaskbarPreview::UpdateTaskbarProperties() {
  nsresult rv = UpdateTooltip();

  
  
  
  if (sActivePreview == this) {
    if (mWnd == ::GetActiveWindow()) {
      nsresult rvActive = ShowActive(PR_TRUE);
      if (NS_FAILED(rvActive))
        rv = rvActive;
    } else {
      sActivePreview = nsnull;
    }
  }
  return rv;
}

nsresult
TaskbarPreview::Enable() {
  nsresult rv = NS_OK;
  if (CanMakeTaskbarCalls()) {
    rv = UpdateTaskbarProperties();
  } else {
    WindowHook &hook = GetWindowHook();
    hook.AddMonitor(nsAppShell::GetTaskbarButtonCreatedMessage(), MainWindowHook, this);
  }
  return rv;
}

nsresult
TaskbarPreview::Disable() {
  WindowHook &hook = GetWindowHook();
  (void) hook.RemoveMonitor(nsAppShell::GetTaskbarButtonCreatedMessage(), MainWindowHook, this);

  return NS_OK;
}

void
TaskbarPreview::DetachFromNSWindow(PRBool windowIsAlive) {
  if (windowIsAlive) {
    WindowHook &hook = GetWindowHook();
    hook.RemoveMonitor(WM_DESTROY, MainWindowHook, this);
  }
  mWnd = NULL;
}

LRESULT
TaskbarPreview::WndProc(UINT nMsg, WPARAM wParam, LPARAM lParam) {
  switch (nMsg) {
    case WM_DWMSENDICONICTHUMBNAIL:
      {
        PRUint32 width = HIWORD(lParam);
        PRUint32 height = LOWORD(lParam);
        float aspectRatio = width/float(height);

        nsresult rv;
        float preferredAspectRatio;
        rv = mController->GetThumbnailAspectRatio(&preferredAspectRatio);
        if (NS_FAILED(rv))
          break;

        PRUint32 thumbnailWidth = width;
        PRUint32 thumbnailHeight = height;

        if (aspectRatio > preferredAspectRatio) {
          thumbnailWidth = PRUint32(thumbnailHeight * preferredAspectRatio);
        } else {
          thumbnailHeight = PRUint32(thumbnailWidth / preferredAspectRatio);
        }

        DrawBitmap(thumbnailWidth, thumbnailHeight, PR_FALSE);
      }
      break;
    case WM_DWMSENDICONICLIVEPREVIEWBITMAP:
      {
        PRUint32 width, height;
        nsresult rv;
        rv = mController->GetWidth(&width);
        if (NS_FAILED(rv))
          break;
        rv = mController->GetHeight(&height);
        if (NS_FAILED(rv))
          break;

        DrawBitmap(width, height, PR_TRUE);
      }
      break;
  }
  return ::DefWindowProcW(PreviewWindow(), nMsg, wParam, lParam);
}

PRBool
TaskbarPreview::CanMakeTaskbarCalls() {
  nsWindow *window = nsWindow::GetNSWindowPtr(mWnd);
  NS_ASSERTION(window, "Cannot use taskbar previews in an embedded context!");

  return mVisible && window->HasTaskbarIconBeenCreated();
}

WindowHook&
TaskbarPreview::GetWindowHook() {
  nsWindow *window = nsWindow::GetNSWindowPtr(mWnd);
  NS_ASSERTION(window, "Cannot use taskbar previews in an embedded context!");

  return window->GetWindowHook();
}

void
TaskbarPreview::EnableCustomDrawing(HWND aHWND, PRBool aEnable) {
  nsUXThemeData::dwmSetWindowAttributePtr(
      aHWND,
      DWMWA_FORCE_ICONIC_REPRESENTATION,
      &aEnable,
      sizeof(aEnable));

  nsUXThemeData::dwmSetWindowAttributePtr(
      aHWND,
      DWMWA_HAS_ICONIC_BITMAP,
      &aEnable,
      sizeof(aEnable));
}


nsresult
TaskbarPreview::UpdateTooltip() {
  NS_ASSERTION(CanMakeTaskbarCalls() && mVisible, "UpdateTooltip called on invisible tab preview");

  if (FAILED(mTaskbar->SetThumbnailTooltip(PreviewWindow(), mTooltip.get())))
    return NS_ERROR_FAILURE;
  return NS_OK;
}

void
TaskbarPreview::DrawBitmap(PRUint32 width, PRUint32 height, PRBool isPreview) {
  nsresult rv;
  nsRefPtr<gfxWindowsSurface> surface = new gfxWindowsSurface(gfxIntSize(width, height), gfxASurface::ImageFormatARGB32);

  nsCOMPtr<nsIDocShell> shell = do_QueryReferent(mDocShell);

  if (!shell)
    return;

  nsCOMPtr<nsICanvasRenderingContextInternal> ctxI;
  rv = CreateRenderingContext(shell, surface, width, height, getter_AddRefs(ctxI));

  nsCOMPtr<nsIDOMCanvasRenderingContext2D> ctx = do_QueryInterface(ctxI);

  PRBool drawFrame = PR_FALSE;
  if (NS_SUCCEEDED(rv) && ctx) {
    if (isPreview)
      rv = mController->DrawPreview(ctx, &drawFrame);
    else
      rv = mController->DrawThumbnail(ctx, width, height, &drawFrame);

  }

  if (NS_FAILED(rv))
    return;

  HDC hDC = surface->GetDC();
  HBITMAP hBitmap = (HBITMAP)GetCurrentObject(hDC, OBJ_BITMAP);

  DWORD flags = drawFrame ? DWM_SIT_DISPLAYFRAME : 0;
  POINT pptClient = { 0, 0 };
  if (isPreview)
    nsUXThemeData::dwmSetIconicLivePreviewBitmapPtr(PreviewWindow(), hBitmap, &pptClient, flags);
  else
    nsUXThemeData::dwmSetIconicThumbnailPtr(PreviewWindow(), hBitmap, flags);
}


PRBool
TaskbarPreview::MainWindowHook(void *aContext,
                               HWND hWnd, UINT nMsg,
                               WPARAM wParam, LPARAM lParam,
                               LRESULT *aResult)
{
  NS_ASSERTION(nMsg == nsAppShell::GetTaskbarButtonCreatedMessage() ||
               nMsg == WM_DESTROY,
               "Window hook proc called with wrong message");
  TaskbarPreview *preview = reinterpret_cast<TaskbarPreview*>(aContext);
  if (nMsg == WM_DESTROY) {
    
    
    
    preview->DetachFromNSWindow(PR_FALSE);
  } else {
    nsWindow *window = nsWindow::GetNSWindowPtr(preview->mWnd);
    NS_ASSERTION(window, "Cannot use taskbar previews in an embedded context!");

    window->SetHasTaskbarIconBeenCreated();

    if (preview->mVisible)
      preview->UpdateTaskbarProperties();
  }
  return PR_FALSE;
}

TaskbarPreview *
TaskbarPreview::sActivePreview = nsnull;

} 
} 

#endif 
