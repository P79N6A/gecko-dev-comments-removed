






































#include "EmbedGtkTools.h"
#ifndef MOZILLA_INTERNAL_API
#include "nsServiceManagerUtils.h"
#endif
#include "EmbedPrivate.h"

GtkWidget * GetGtkWidgetForDOMWindow(nsIDOMWindow* aDOMWindow)
{
  nsCOMPtr<nsIWindowWatcher> wwatch = do_GetService("@mozilla.org/embedcomp/window-watcher;1");
  if (!aDOMWindow)
    return NULL;
  nsCOMPtr<nsIWebBrowserChrome> chrome;
  wwatch->GetChromeForWindow(aDOMWindow, getter_AddRefs(chrome));
  if (!chrome) {
    return GTK_WIDGET(EmbedCommon::GetAnyLiveWidget());
  }

  nsCOMPtr<nsIEmbeddingSiteWindow> siteWindow = nsnull;
  siteWindow = do_QueryInterface(chrome);

  if (!siteWindow) {
    return GTK_WIDGET(EmbedCommon::GetAnyLiveWidget());
  }

  GtkWidget* parentWidget;
  siteWindow->GetSiteWindow((void**)&parentWidget);
  if (GTK_IS_WIDGET(parentWidget))
    return parentWidget;
  return NULL;
}

GtkWindow * GetGtkWindowForDOMWindow(nsIDOMWindow* aDOMWindow)
{
  GtkWidget* parentWidget = GetGtkWidgetForDOMWindow(aDOMWindow);
  if (!parentWidget)
    return NULL;
  GtkWidget* gtkWin = gtk_widget_get_toplevel(parentWidget);
  if (GTK_WIDGET_TOPLEVEL(gtkWin))
    return GTK_WINDOW(gtkWin);
  return NULL;
}

nsresult GetContentViewer(nsIWebBrowser *webBrowser, nsIContentViewer **aViewer)
{
  g_return_val_if_fail(webBrowser, NS_ERROR_FAILURE);
  nsCOMPtr<nsIDocShell> docShell(do_GetInterface((nsISupports*)webBrowser));
  NS_ENSURE_TRUE(docShell, NS_ERROR_FAILURE);
  return docShell->GetContentViewer(aViewer);
}
