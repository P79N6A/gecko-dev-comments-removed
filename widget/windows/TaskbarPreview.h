







































#ifndef __mozilla_widget_TaskbarPreview_h__
#define __mozilla_widget_TaskbarPreview_h__

#include <windows.h>
#include <shobjidl.h>

#include <nsITaskbarPreview.h>
#include <nsAutoPtr.h>
#include <nsString.h>
#include <nsWeakPtr.h>
#include <nsIDocShell.h>
#include "WindowHook.h"

namespace mozilla {
namespace widget {

class TaskbarPreview : public nsITaskbarPreview
{
public:
  TaskbarPreview(ITaskbarList4 *aTaskbar, nsITaskbarPreviewController *aController, HWND aHWND, nsIDocShell *aShell);
  virtual ~TaskbarPreview();

  NS_DECL_NSITASKBARPREVIEW

protected:
  
  virtual nsresult UpdateTaskbarProperties();

  
  virtual nsresult Enable();
  
  virtual nsresult Disable();

  
  virtual void DetachFromNSWindow();

  
  bool IsWindowAvailable() const;

  
  virtual nsresult ShowActive(bool active) = 0;
  
  virtual HWND& PreviewWindow() = 0;

  
  virtual LRESULT WndProc(UINT nMsg, WPARAM wParam, LPARAM lParam);

  
  
  
  bool CanMakeTaskbarCalls();

  
  WindowHook &GetWindowHook();

  
  static void EnableCustomDrawing(HWND aHWND, bool aEnable);

  
  nsRefPtr<ITaskbarList4> mTaskbar;
  
  nsCOMPtr<nsITaskbarPreviewController> mController;
  
  HWND                    mWnd;
  
  bool                    mVisible;

private:
  
  nsresult UpdateTooltip();

  
  
  void DrawBitmap(PRUint32 width, PRUint32 height, bool isPreview);

  
  static bool MainWindowHook(void *aContext,
                               HWND hWnd, UINT nMsg,
                               WPARAM wParam, LPARAM lParam,
                               LRESULT *aResult);

  
  nsWeakPtr               mDocShell;
  nsString                mTooltip;

  
  
  static TaskbarPreview  *sActivePreview;
};

} 
} 

#endif 

