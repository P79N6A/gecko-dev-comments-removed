







































#ifndef __mozilla_widget_TaskbarTabPreview_h__
#define __mozilla_widget_TaskbarTabPreview_h__

#include "nsITaskbarTabPreview.h"
#include "TaskbarPreview.h"

namespace mozilla {
namespace widget {

class TaskbarTabPreview : public nsITaskbarTabPreview,
                          public TaskbarPreview
{
public:
  TaskbarTabPreview(ITaskbarList4 *aTaskbar, nsITaskbarPreviewController *aController, HWND aHWND, nsIDocShell *aShell);
  virtual ~TaskbarTabPreview();

  NS_DECL_ISUPPORTS
  NS_DECL_NSITASKBARTABPREVIEW
  NS_FORWARD_NSITASKBARPREVIEW(TaskbarPreview::)

private:
  virtual nsresult ShowActive(bool active);
  virtual HWND &PreviewWindow();
  virtual LRESULT WndProc(UINT nMsg, WPARAM wParam, LPARAM lParam);
  static LRESULT CALLBACK GlobalWndProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam);

  virtual nsresult UpdateTaskbarProperties();
  virtual nsresult Enable();
  virtual nsresult Disable();
  virtual void DetachFromNSWindow();

  
  static bool MainWindowHook(void *aContext,
                               HWND hWnd, UINT nMsg,
                               WPARAM wParam, LPARAM lParam,
                               LRESULT *aResult);

  
  
  
  void UpdateProxyWindowStyle();

  nsresult UpdateTitle();
  nsresult UpdateIcon();
  nsresult UpdateNext();

  
  HWND                    mProxyWindow;
  nsString                mTitle;
  nsCOMPtr<imgIContainer> mIconImage;
  
  HICON                   mIcon;
  
  nsCOMPtr<nsITaskbarTabPreview> mNext;
  
  bool                    mRegistered;
};

} 
} 

#endif 
