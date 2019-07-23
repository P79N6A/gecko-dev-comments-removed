







































#if MOZ_WINSDK_TARGETVER >= MOZ_NTDDI_WIN7

#include <windows.h>
#include <strsafe.h>

#include "TaskbarWindowPreview.h"
#include "TaskbarPreviewButton.h"
#include "nsWindowGfx.h"
#include <imgIContainer.h>

namespace mozilla {
namespace widget {

NS_IMPL_ISUPPORTS2(TaskbarPreviewButton, nsITaskbarPreviewButton, nsISupportsWeakReference)

TaskbarPreviewButton::TaskbarPreviewButton(TaskbarWindowPreview* preview, PRUint32 index)
  : mPreview(preview), mIndex(index)
{
}

TaskbarPreviewButton::~TaskbarPreviewButton() {
  SetVisible(PR_FALSE);
}

NS_IMETHODIMP
TaskbarPreviewButton::GetTooltip(nsAString &aTooltip) {
  aTooltip = mTooltip;
  return NS_OK;
}

NS_IMETHODIMP
TaskbarPreviewButton::SetTooltip(const nsAString &aTooltip) {
  mTooltip = aTooltip;
  size_t destLength = sizeof Button().szTip / (sizeof Button().szTip[0]);
  wchar_t *tooltip = &(Button().szTip[0]);
  StringCchCopyNW(tooltip,
                  destLength,
                  mTooltip.get(),
                  mTooltip.Length());
  return Update();
}

NS_IMETHODIMP
TaskbarPreviewButton::GetDismissOnClick(PRBool *dismiss) {
  *dismiss = (Button().dwFlags & THBF_DISMISSONCLICK) == THBF_DISMISSONCLICK;
  return NS_OK;
}

NS_IMETHODIMP
TaskbarPreviewButton::SetDismissOnClick(PRBool dismiss) {
  if (dismiss)
    Button().dwFlags |= THBF_DISMISSONCLICK;
  else
    Button().dwFlags &= ~THBF_DISMISSONCLICK;
  return Update();
}

NS_IMETHODIMP
TaskbarPreviewButton::GetHasBorder(PRBool *hasBorder) {
  *hasBorder = (Button().dwFlags & THBF_NOBACKGROUND) != THBF_NOBACKGROUND;
  return NS_OK;
}

NS_IMETHODIMP
TaskbarPreviewButton::SetHasBorder(PRBool hasBorder) {
  if (hasBorder)
    Button().dwFlags &= ~THBF_NOBACKGROUND;
  else
    Button().dwFlags |= THBF_NOBACKGROUND;
  return Update();
}

NS_IMETHODIMP
TaskbarPreviewButton::GetDisabled(PRBool *disabled) {
  *disabled = (Button().dwFlags & THBF_DISABLED) == THBF_DISABLED;
  return NS_OK;
}

NS_IMETHODIMP
TaskbarPreviewButton::SetDisabled(PRBool disabled) {
  if (disabled)
    Button().dwFlags |= THBF_DISABLED;
  else
    Button().dwFlags &= ~THBF_DISABLED;
  return Update();
}

NS_IMETHODIMP
TaskbarPreviewButton::GetImage(imgIContainer **img) {
  if (mImage)
    NS_ADDREF(*img = mImage);
  else
    *img = NULL;
  return NS_OK;
}

NS_IMETHODIMP
TaskbarPreviewButton::SetImage(imgIContainer *img) {
  if (Button().hIcon)
    ::DestroyIcon(Button().hIcon);
  if (img) {
    nsresult rv;
    rv = nsWindowGfx::CreateIcon(img, PR_FALSE, 0, 0, &Button().hIcon);
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    Button().hIcon = NULL;
  }
  return Update();
}

NS_IMETHODIMP
TaskbarPreviewButton::GetVisible(PRBool *visible) {
  *visible = (Button().dwFlags & THBF_HIDDEN) == THBF_HIDDEN;
  return NS_OK;
}

NS_IMETHODIMP
TaskbarPreviewButton::SetVisible(PRBool visible) {
  if (visible)
    Button().dwFlags &= ~THBF_HIDDEN;
  else
    Button().dwFlags |= THBF_HIDDEN;
  return Update();
}

THUMBBUTTON&
TaskbarPreviewButton::Button() {
  return mPreview->mThumbButtons[mIndex];
}

nsresult
TaskbarPreviewButton::Update() {
  return mPreview->UpdateButton(mIndex);
}

} 
} 

#endif 

