





































#ifndef __EmbedTools_h
#define __EmbedTools_h

#include "nsCOMPtr.h"
#ifdef MOZILLA_INTERNAL_API
#include "nsString.h"
#else
#include "nsStringAPI.h"
#endif
#include "nsIDOMWindow.h"
#include "nsIWindowWatcher.h"
#include "nsIWebBrowserChrome.h"
#include "nsIEmbeddingSiteWindow.h"
#include "nsIServiceManager.h"
#include "nsIContentViewer.h"
#include "nsIDocShell.h"
#include "nsIInterfaceRequestorUtils.h"
#include <gtk/gtk.h>

GtkWidget*
GetGtkWidgetForDOMWindow(nsIDOMWindow* aDOMWindow);

GtkWindow*
GetGtkWindowForDOMWindow(nsIDOMWindow* aDOMWindow);

nsresult
GetContentViewer (nsIWebBrowser *webBrowser, nsIContentViewer **aViewer);

PRUnichar*
LocaleToUnicode (const char *locStr);

#endif 
