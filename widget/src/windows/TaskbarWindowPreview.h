







































#ifndef __mozilla_widget_TaskbarWindowPreview_h__
#define __mozilla_widget_TaskbarWindowPreview_h__

#if MOZ_WINSDK_TARGETVER >= MOZ_NTDDI_WIN7

#include "nsITaskbarWindowPreview.h"
#include "TaskbarPreview.h"
#include <nsWeakReference.h>

namespace mozilla {
namespace widget {

class TaskbarPreviewButton;
class TaskbarWindowPreview : public TaskbarPreview,
                             public nsITaskbarWindowPreview,
                             public nsSupportsWeakReference
{
public:
  TaskbarWindowPreview(ITaskbarList4 *aTaskbar, nsITaskbarPreviewController *aController, HWND aHWND, nsIDocShell *aShell);
  virtual ~TaskbarWindowPreview();

  NS_DECL_ISUPPORTS
  NS_DECL_NSITASKBARWINDOWPREVIEW
  NS_FORWARD_NSITASKBARPREVIEW(TaskbarPreview::)

  virtual LRESULT WndProc(UINT nMsg, WPARAM wParam, LPARAM lParam);
private:
  virtual nsresult ShowActive(PRBool active);
  virtual HWND &PreviewWindow();

  virtual nsresult UpdateTaskbarProperties();
  virtual nsresult Enable();
  virtual nsresult Disable();
  virtual void DetachFromNSWindow(PRBool windowIsAlive);
  nsresult UpdateButton(PRUint32 index);
  nsresult UpdateButtons();

  
  PRBool                  mCustomDrawing;
  
  PRBool                  mHaveButtons;
  
  THUMBBUTTON             mThumbButtons[nsITaskbarWindowPreview::MAX_TOOLBAR_BUTTONS];
  
  nsWeakPtr               mWeakButtons[nsITaskbarWindowPreview::MAX_TOOLBAR_BUTTONS];

  friend class TaskbarPreviewButton;
};

} 
} 

#endif 

#endif 

