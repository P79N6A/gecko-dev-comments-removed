






#include "nsGTKRemoteService.h"

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>

#include "nsIBaseWindow.h"
#include "nsIDocShell.h"
#include "nsPIDOMWindow.h"
#include "mozilla/ModuleUtils.h"
#include "nsIServiceManager.h"
#include "nsIWeakReference.h"
#include "nsIWidget.h"
#include "nsIAppShellService.h"
#include "nsAppShellCID.h"

#include "nsCOMPtr.h"

#include "nsGTKToolkit.h"

NS_IMPL_ISUPPORTS(nsGTKRemoteService,
                  nsIRemoteService,
                  nsIObserver)

NS_IMETHODIMP
nsGTKRemoteService::Startup(const char* aAppName, const char* aProfileName)
{
  NS_ASSERTION(aAppName, "Don't pass a null appname!");
  sRemoteImplementation = this;

  if (mServerWindow) return NS_ERROR_ALREADY_INITIALIZED;

  XRemoteBaseStartup(aAppName, aProfileName);

  mServerWindow = gtk_invisible_new();
  gtk_widget_realize(mServerWindow);
  HandleCommandsFor(mServerWindow, nullptr);

  mWindows.EnumerateRead(StartupHandler, this);

  return NS_OK;
}

PLDHashOperator
nsGTKRemoteService::StartupHandler(GtkWidget* aKey,
                                   nsIWeakReference* aData,
                                   void* aClosure)
{
  GtkWidget* widget = (GtkWidget*) aKey;
  nsGTKRemoteService* aThis = (nsGTKRemoteService*) aClosure;

  aThis->HandleCommandsFor(widget, aData);
  return PL_DHASH_NEXT;
}

static nsIWidget* GetMainWidget(nsIDOMWindow* aWindow)
{
  
  nsCOMPtr<nsPIDOMWindow> window(do_QueryInterface(aWindow));
  NS_ENSURE_TRUE(window, nullptr);

  nsCOMPtr<nsIBaseWindow> baseWindow
    (do_QueryInterface(window->GetDocShell()));
  NS_ENSURE_TRUE(baseWindow, nullptr);

  nsCOMPtr<nsIWidget> mainWidget;
  baseWindow->GetMainWidget(getter_AddRefs(mainWidget));
  return mainWidget;
}

NS_IMETHODIMP
nsGTKRemoteService::RegisterWindow(nsIDOMWindow* aWindow)
{
  nsIWidget* mainWidget = GetMainWidget(aWindow);
  NS_ENSURE_TRUE(mainWidget, NS_ERROR_FAILURE);

  GtkWidget* widget =
    (GtkWidget*) mainWidget->GetNativeData(NS_NATIVE_SHELLWIDGET);
  NS_ENSURE_TRUE(widget, NS_ERROR_FAILURE);

  nsCOMPtr<nsIWeakReference> weak = do_GetWeakReference(aWindow);
  NS_ENSURE_TRUE(weak, NS_ERROR_FAILURE);

  mWindows.Put(widget, weak);

  
  if (mServerWindow) {
    HandleCommandsFor(widget, weak);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsGTKRemoteService::Shutdown()
{
  if (!mServerWindow)
    return NS_ERROR_NOT_INITIALIZED;

  gtk_widget_destroy(mServerWindow);
  mServerWindow = nullptr;
  return NS_OK;
}






void
nsGTKRemoteService::SetDesktopStartupIDOrTimestamp(const nsACString& aDesktopStartupID,
                                                   uint32_t aTimestamp) {
  nsGTKToolkit* toolkit = nsGTKToolkit::GetToolkit();
  if (!toolkit)
    return;

  if (!aDesktopStartupID.IsEmpty()) {
    toolkit->SetDesktopStartupID(aDesktopStartupID);
  }

  toolkit->SetFocusTimestamp(aTimestamp);
}


void
nsGTKRemoteService::HandleCommandsFor(GtkWidget* widget,
                                      nsIWeakReference* aWindow)
{
  g_signal_connect(G_OBJECT(widget), "property_notify_event",
                   G_CALLBACK(HandlePropertyChange), aWindow);

  gtk_widget_add_events(widget, GDK_PROPERTY_CHANGE_MASK);

#if (MOZ_WIDGET_GTK == 2)
  Window window = GDK_WINDOW_XWINDOW(widget->window);
#else
  Window window = gdk_x11_window_get_xid(gtk_widget_get_window(widget));
#endif
  nsXRemoteService::HandleCommandsFor(window);

}

gboolean
nsGTKRemoteService::HandlePropertyChange(GtkWidget *aWidget,
                                         GdkEventProperty *pevent,
                                         nsIWeakReference *aThis)
{
  if (pevent->state == GDK_PROPERTY_NEW_VALUE) {
    Atom changedAtom = gdk_x11_atom_to_xatom(pevent->atom);

#if (MOZ_WIDGET_GTK == 2)
    XID window = GDK_WINDOW_XWINDOW(pevent->window);
#else
    XID window = gdk_x11_window_get_xid(gtk_widget_get_window(aWidget));
#endif
    return HandleNewProperty(window,
                             GDK_DISPLAY_XDISPLAY(gdk_display_get_default()),
                             pevent->time, changedAtom, aThis);
  }
  return FALSE;
}



#define NS_REMOTESERVICE_CID \
  { 0xc0773e90, 0x5799, 0x4eff, { 0xad, 0x3, 0x3e, 0xbc, 0xd8, 0x56, 0x24, 0xac } }

NS_GENERIC_FACTORY_CONSTRUCTOR(nsGTKRemoteService)
NS_DEFINE_NAMED_CID(NS_REMOTESERVICE_CID);

static const mozilla::Module::CIDEntry kRemoteCIDs[] = {
  { &kNS_REMOTESERVICE_CID, false, nullptr, nsGTKRemoteServiceConstructor },
  { nullptr }
};

static const mozilla::Module::ContractIDEntry kRemoteContracts[] = {
  { "@mozilla.org/toolkit/remote-service;1", &kNS_REMOTESERVICE_CID },
  { nullptr }
};

static const mozilla::Module kRemoteModule = {
  mozilla::Module::kVersion,
  kRemoteCIDs,
  kRemoteContracts
};

NSMODULE_DEFN(RemoteServiceModule) = &kRemoteModule;
