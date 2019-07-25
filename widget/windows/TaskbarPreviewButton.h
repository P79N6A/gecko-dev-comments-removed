






#ifndef __mozilla_widget_TaskbarPreviewButton_h__
#define __mozilla_widget_TaskbarPreviewButton_h__

#include <windows.h>
#include <shobjidl.h>

#include <nsITaskbarPreviewButton.h>
#include <nsAutoPtr.h>
#include <nsString.h>
#include <nsWeakReference.h>

namespace mozilla {
namespace widget {

class TaskbarWindowPreview;
class TaskbarPreviewButton : public nsITaskbarPreviewButton, public nsSupportsWeakReference
{
public: 
  TaskbarPreviewButton(TaskbarWindowPreview* preview, uint32_t index);
  virtual ~TaskbarPreviewButton();

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

