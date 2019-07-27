






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
#include "WinUtils.h"
#include "gfxWindowsPlatform.h"

#include <nsIBaseWindow.h>
#include <nsICanvasRenderingContextInternal.h>
#include "mozilla/dom/CanvasRenderingContext2D.h"
#include <imgIContainer.h>
#include <nsIDocShell.h>

#include "mozilla/Telemetry.h"


#define DWM_SIT_DISPLAYFRAME 0x1

namespace mozilla {
namespace widget {

namespace {




dom::CanvasRenderingContext2D* gCtx = nullptr;


uint32_t gInstCount = 0;










nsresult
GetRenderingContext(nsIDocShell *shell, gfxASurface *surface,
                    uint32_t width, uint32_t height) {
  if (!gCtx) {
    
    Telemetry::Accumulate(Telemetry::CANVAS_2D_USED, 1);
    gCtx = new mozilla::dom::CanvasRenderingContext2D();
    NS_ADDREF(gCtx);
  }

  
  return gCtx->InitializeWithSurface(shell, surface, width, height);
}



void
ResetRenderingContext() {
  if (!gCtx)
    return;

  if (NS_FAILED(gCtx->Reset())) {
    NS_RELEASE(gCtx);
    gCtx = nullptr;
  }
}

}

TaskbarPreview::TaskbarPreview(ITaskbarList4 *aTaskbar, nsITaskbarPreviewController *aController, HWND aHWND, nsIDocShell *aShell)
  : mTaskbar(aTaskbar),
    mController(aController),
    mWnd(aHWND),
    mVisible(false),
    mDocShell(do_GetWeakReference(aShell))
{
  
  ::CoInitialize(nullptr);

  gInstCount++;

  WindowHook &hook = GetWindowHook();
  hook.AddMonitor(WM_DESTROY, MainWindowHook, this);
}

TaskbarPreview::~TaskbarPreview() {
  
  if (sActivePreview == this)
    sActivePreview = nullptr;

  
  NS_ASSERTION(!mWnd, "TaskbarPreview::DetachFromNSWindow was not called before destruction");

  
  mTaskbar = nullptr;

  if (--gInstCount == 0)
    NS_IF_RELEASE(gCtx);

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
  mTooltip = aTooltip;
  return CanMakeTaskbarCalls() ? UpdateTooltip() : NS_OK;
}

NS_IMETHODIMP
TaskbarPreview::SetVisible(bool visible) {
  if (mVisible == visible) return NS_OK;
  mVisible = visible;

  
  
  
  
  if (!mWnd)
    return NS_OK;

  return visible ? Enable() : Disable();
}

NS_IMETHODIMP
TaskbarPreview::GetVisible(bool *visible) {
  *visible = mVisible;
  return NS_OK;
}

NS_IMETHODIMP
TaskbarPreview::SetActive(bool active) {
  if (active)
    sActivePreview = this;
  else if (sActivePreview == this)
    sActivePreview = nullptr;

  return CanMakeTaskbarCalls() ? ShowActive(active) : NS_OK;
}

NS_IMETHODIMP
TaskbarPreview::GetActive(bool *active) {
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
  return FAILED(WinUtils::dwmInvalidateIconicBitmapsPtr(previewWindow))
       ? NS_ERROR_FAILURE
       : NS_OK;
}

nsresult
TaskbarPreview::UpdateTaskbarProperties() {
  nsresult rv = UpdateTooltip();

  
  
  
  if (sActivePreview == this) {
    if (mWnd == ::GetActiveWindow()) {
      nsresult rvActive = ShowActive(true);
      if (NS_FAILED(rvActive))
        rv = rvActive;
    } else {
      sActivePreview = nullptr;
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

bool
TaskbarPreview::IsWindowAvailable() const {
  if (mWnd) {
    nsWindow* win = WinUtils::GetNSWindowPtr(mWnd);
    if(win && !win->Destroyed()) {
      return true;
    }
  }
  return false;
}

void
TaskbarPreview::DetachFromNSWindow() {
  WindowHook &hook = GetWindowHook();
  hook.RemoveMonitor(WM_DESTROY, MainWindowHook, this);
  mWnd = nullptr;
}

LRESULT
TaskbarPreview::WndProc(UINT nMsg, WPARAM wParam, LPARAM lParam) {
  switch (nMsg) {
    case WM_DWMSENDICONICTHUMBNAIL:
      {
        uint32_t width = HIWORD(lParam);
        uint32_t height = LOWORD(lParam);
        float aspectRatio = width/float(height);

        nsresult rv;
        float preferredAspectRatio;
        rv = mController->GetThumbnailAspectRatio(&preferredAspectRatio);
        if (NS_FAILED(rv))
          break;

        uint32_t thumbnailWidth = width;
        uint32_t thumbnailHeight = height;

        if (aspectRatio > preferredAspectRatio) {
          thumbnailWidth = uint32_t(thumbnailHeight * preferredAspectRatio);
        } else {
          thumbnailHeight = uint32_t(thumbnailWidth / preferredAspectRatio);
        }

        DrawBitmap(thumbnailWidth, thumbnailHeight, false);
      }
      break;
    case WM_DWMSENDICONICLIVEPREVIEWBITMAP:
      {
        uint32_t width, height;
        nsresult rv;
        rv = mController->GetWidth(&width);
        if (NS_FAILED(rv))
          break;
        rv = mController->GetHeight(&height);
        if (NS_FAILED(rv))
          break;

        double scale = nsIWidget::DefaultScaleOverride();
        if (scale <= 0.0)
          scale = gfxWindowsPlatform::GetPlatform()->GetDPIScale();

        DrawBitmap(NSToIntRound(scale * width), NSToIntRound(scale * height), true);
      }
      break;
  }
  return ::DefWindowProcW(PreviewWindow(), nMsg, wParam, lParam);
}

bool
TaskbarPreview::CanMakeTaskbarCalls() {
  
  
  if (!mWnd)
    return false;
  
  
  if (!::IsWindowVisible(mWnd))
    return false;
  if (mVisible) {
    nsWindow *window = WinUtils::GetNSWindowPtr(mWnd);
    NS_ASSERTION(window, "Could not get nsWindow from HWND");
    return window->HasTaskbarIconBeenCreated();
  }
  return false;
}

WindowHook&
TaskbarPreview::GetWindowHook() {
  nsWindow *window = WinUtils::GetNSWindowPtr(mWnd);
  NS_ASSERTION(window, "Cannot use taskbar previews in an embedded context!");

  return window->GetWindowHook();
}

void
TaskbarPreview::EnableCustomDrawing(HWND aHWND, bool aEnable) {
  BOOL enabled = aEnable;
  WinUtils::dwmSetWindowAttributePtr(
      aHWND,
      DWMWA_FORCE_ICONIC_REPRESENTATION,
      &enabled,
      sizeof(enabled));

  WinUtils::dwmSetWindowAttributePtr(
      aHWND,
      DWMWA_HAS_ICONIC_BITMAP,
      &enabled,
      sizeof(enabled));
}


nsresult
TaskbarPreview::UpdateTooltip() {
  NS_ASSERTION(CanMakeTaskbarCalls() && mVisible, "UpdateTooltip called on invisible tab preview");

  if (FAILED(mTaskbar->SetThumbnailTooltip(PreviewWindow(), mTooltip.get())))
    return NS_ERROR_FAILURE;
  return NS_OK;
}

void
TaskbarPreview::DrawBitmap(uint32_t width, uint32_t height, bool isPreview) {
  nsresult rv;
  nsRefPtr<gfxWindowsSurface> surface = new gfxWindowsSurface(gfxIntSize(width, height), gfxImageFormat::ARGB32);

  nsCOMPtr<nsIDocShell> shell = do_QueryReferent(mDocShell);

  if (!shell)
    return;

  rv = GetRenderingContext(shell, surface, width, height);
  if (NS_FAILED(rv))
    return;

  bool drawFrame = false;
  if (isPreview)
    rv = mController->DrawPreview(gCtx, &drawFrame);
  else
    rv = mController->DrawThumbnail(gCtx, width, height, &drawFrame);

  if (NS_FAILED(rv))
    return;

  HDC hDC = surface->GetDC();
  HBITMAP hBitmap = (HBITMAP)GetCurrentObject(hDC, OBJ_BITMAP);

  DWORD flags = drawFrame ? DWM_SIT_DISPLAYFRAME : 0;
  POINT pptClient = { 0, 0 };
  if (isPreview)
    WinUtils::dwmSetIconicLivePreviewBitmapPtr(PreviewWindow(), hBitmap, &pptClient, flags);
  else
    WinUtils::dwmSetIconicThumbnailPtr(PreviewWindow(), hBitmap, flags);

  ResetRenderingContext();
}


bool
TaskbarPreview::MainWindowHook(void *aContext,
                               HWND hWnd, UINT nMsg,
                               WPARAM wParam, LPARAM lParam,
                               LRESULT *aResult)
{
  NS_ASSERTION(nMsg == nsAppShell::GetTaskbarButtonCreatedMessage() ||
               nMsg == WM_DESTROY,
               "Window hook proc called with wrong message");
  NS_ASSERTION(aContext, "Null context in MainWindowHook");
  if (!aContext)
    return false;
  TaskbarPreview *preview = reinterpret_cast<TaskbarPreview*>(aContext);
  if (nMsg == WM_DESTROY) {
    
    
    return false;
  } else {
    nsWindow *window = WinUtils::GetNSWindowPtr(preview->mWnd);
    if (window) {
      window->SetHasTaskbarIconBeenCreated();

      if (preview->mVisible)
        preview->UpdateTaskbarProperties();
    }
  }
  return false;
}

TaskbarPreview *
TaskbarPreview::sActivePreview = nullptr;

} 
} 

