






#ifndef __mozilla_widget_TaskbarPreviewButton_h__
#define __mozilla_widget_TaskbarPreviewButton_h__

#include <windows.h>
#include <shobjidl.h>
#undef LogSeverity // SetupAPI.h #defines this as DWORD

#include <nsITaskbarPreviewButton.h>
#include <nsAutoPtr.h>
#include <nsString.h>
#include <nsWeakReference.h>

namespace mozilla {
namespace widget {

class TaskbarWindowPreview;
class TaskbarPreviewButton : public nsITaskbarPreviewButton, public nsSupportsWeakReference
{
  virtual ~TaskbarPreviewButton();

public: 
  TaskbarPreviewButton(TaskbarWindowPreview* preview, uint32_t index);

  NS_DECL_ISUPPORTS
  NS_DECL_NSITASKBARPREVIEWBUTTON

private:
  THUMBBUTTON&            Button();
  nsresult                Update();

  nsRefPtr<TaskbarWindowPreview> mPreview;
  uint32_t                mIndex;
  nsString                mTooltip;
  nsCOMPtr<imgIContainer> mImage;
};

} 
} 

#endif 

