







































#include "WidgetUtils.h"

#include "nsIBaseWindow.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDocShell.h"
#include "nsIInterfaceRequestorUtils.h"

namespace mozilla {
namespace widget {


already_AddRefed<nsIWidget>
WidgetUtils::DOMWindowToWidget(nsIDOMWindow *aDOMWindow)
{
  nsCOMPtr<nsIWidget> widget;

  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(aDOMWindow);
  if (window) {
    nsCOMPtr<nsIBaseWindow> baseWin(do_QueryInterface(window->GetDocShell()));

    while (!widget && baseWin) {
      baseWin->GetParentWidget(getter_AddRefs(widget));
      if (!widget) {
        nsCOMPtr<nsIDocShellTreeItem> docShellAsItem(do_QueryInterface(baseWin));
        if (!docShellAsItem)
          return nsnull;

        nsCOMPtr<nsIDocShellTreeItem> parent;
        docShellAsItem->GetParent(getter_AddRefs(parent));

        window = do_GetInterface(parent);
        if (!window)
          return nsnull;

        baseWin = do_QueryInterface(window->GetDocShell());
      }
    }
  }

  return widget.forget();
}



BrightnessLockingWidget::BrightnessLockingWidget()
{
  for (PRUint32 i = 0; i < nsIScreen_MOZILLA_2_0_BRANCH::BRIGHTNESS_LEVELS; i++)
    mBrightnessLocks[i] = 0;
}

NS_IMETHODIMP
BrightnessLockingWidget::LockMinimumBrightness(PRUint32 aBrightness)
{
  NS_ABORT_IF_FALSE(
    aBrightness < nsIScreen_MOZILLA_2_0_BRANCH::BRIGHTNESS_LEVELS,
    "Invalid brightness level to lock");
  mBrightnessLocks[aBrightness]++;
  NS_ABORT_IF_FALSE(mBrightnessLocks[aBrightness] > 0,
    "Overflow after locking brightness level");

  CheckMinimumBrightness();

  return NS_OK;
}

NS_IMETHODIMP
BrightnessLockingWidget::UnlockMinimumBrightness(PRUint32 aBrightness)
{
  NS_ABORT_IF_FALSE(
    aBrightness < nsIScreen_MOZILLA_2_0_BRANCH::BRIGHTNESS_LEVELS,
    "Invalid brightness level to lock");
  NS_ABORT_IF_FALSE(mBrightnessLocks[aBrightness] > 0,
    "Unlocking a brightness level with no corresponding lock");
  mBrightnessLocks[aBrightness]--;

  CheckMinimumBrightness();

  return NS_OK;
}

void
BrightnessLockingWidget::CheckMinimumBrightness()
{
  PRUint32 brightness = nsIScreen_MOZILLA_2_0_BRANCH::BRIGHTNESS_LEVELS;
  for (PRUint32 i = 0; i < nsIScreen_MOZILLA_2_0_BRANCH::BRIGHTNESS_LEVELS; i++)
    if (mBrightnessLocks[i] > 0)
      brightness = i;

  ApplyMinimumBrightness(brightness);
}

} 
} 
