








































#ifndef __mozilla_widget_TaskbarWindowPreview_h__
#define __mozilla_widget_TaskbarWindowPreview_h__

#if MOZ_WINSDK_TARGETVER >= MOZ_NTDDI_WIN7

#include "nsITaskbarWindowPreview.h"
#include "nsITaskbarProgress.h"
#include "TaskbarPreview.h"
#include <nsWeakReference.h>

namespace mozilla {
namespace widget {

class TaskbarPreviewButton;
class TaskbarWindowPreview : public TaskbarPreview,
                             public nsITaskbarWindowPreview,
                             public nsITaskbarProgress,
                             public nsSupportsWeakReference
{
public:
  TaskbarWindowPreview(ITaskbarList4 *aTaskbar, nsITaskbarPreviewController *aController, HWND aHWND, nsIDocShell *aShell);
  virtual ~TaskbarWindowPreview();

  NS_DECL_ISUPPORTS
  NS_DECL_NSITASKBARWINDOWPREVIEW
  NS_DECL_NSITASKBARPROGRESS
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
  
  THUMBBUTTON             mThumbButtons[nsITaskbarWindowPreview::NUM_TOOLBAR_BUTTONS];
  
  nsWeakPtr               mWeakButtons[nsITaskbarWindowPreview::NUM_TOOLBAR_BUTTONS];

  
  nsresult UpdateTaskbarProgress();

  
  TBPFLAG                 mState;
  ULONGLONG               mCurrentValue;
  ULONGLONG               mMaxValue;

  
  static PRBool TaskbarProgressWindowHook(void *aContext,
                                          HWND hWnd, UINT nMsg,
                                          WPARAM wParam, LPARAM lParam,
                                          LRESULT *aResult);

  friend class TaskbarPreviewButton;
};

} 
} 

#endif 

#endif 

