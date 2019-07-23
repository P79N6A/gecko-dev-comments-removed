







































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
  nsCOMPtr<nsIAccessible> accessible;

  
  if (accService) {
    AtkObject* gailWindow = gtk_widget_get_accessible(GTK_WIDGET(aDialog));
    accService->AddNativeRootAccessible(gailWindow, getter_AddRefs(accessible));
  }

  gint result = gtk_dialog_run (aDialog);

  
  if (accService && accessible) {
    accService->RemoveNativeRootAccessible(accessible);
  }

  return result;
#else
  return gtk_dialog_run (aDialog);
#endif
}
