







































#include "nsAccessibilityHelper.h"

#ifdef ACCESSIBILITY
#include "nsIAccessibilityService.h"
#include "nsWindow.h"
#endif

gint RunDialog(GtkDialog* aDialog)
{
#ifdef ACCESSIBILITY
  if (!nsWindow::sAccessibilityEnabled) {
    return gtk_dialog_run (aDialog);
  }

  nsCOMPtr<nsIAccessibilityService> accService =
    do_GetService ("@mozilla.org/accessibilityService;1");

  
  nsAccessible* windowAcc = nsnull;
  if (accService) {
    AtkObject* gailWindow = gtk_widget_get_accessible(GTK_WIDGET(aDialog));
    windowAcc = accService->AddNativeRootAccessible(gailWindow);
  }

  gint result = gtk_dialog_run (aDialog);

  
  if (accService && windowAcc) {
    accService->RemoveNativeRootAccessible(windowAcc);
  }

  return result;
#else
  return gtk_dialog_run (aDialog);
#endif
}
