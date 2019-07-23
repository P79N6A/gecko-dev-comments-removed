







































#include "nsCWebBrowser.h"
#include "nsIComponentManager.h"
#include "nsIDocShellTreeItem.h"
#include "nsIWidget.h"
#ifdef MOZILLA_INTERNAL_API
#include "nsReadableUtils.h"
#else
#include "nsComponentManagerUtils.h"
#endif
#include "EmbedWindow.h"
#include "EmbedPrivate.h"
#include "EmbedPrompter.h"

GtkWidget *EmbedWindow::sTipWindow = nsnull;

EmbedWindow::EmbedWindow(void)
{
  mOwner       = nsnull;
  mVisibility  = PR_FALSE;
  mIsModal     = PR_FALSE;
}

EmbedWindow::~EmbedWindow(void)
{
  ExitModalEventLoop(PR_FALSE);
}

nsresult
EmbedWindow::Init(EmbedPrivate *aOwner)
{
  
  mOwner = aOwner;

  
  mWebBrowser = do_CreateInstance(NS_WEBBROWSER_CONTRACTID);
  if (!mWebBrowser)
    return NS_ERROR_FAILURE;

  mWebBrowser->SetContainerWindow(NS_STATIC_CAST(nsIWebBrowserChrome *, this));

  nsCOMPtr<nsIDocShellTreeItem> item = do_QueryInterface(mWebBrowser);
  item->SetItemType(nsIDocShellTreeItem::typeContentWrapper);

  return NS_OK;
}

nsresult
EmbedWindow::CreateWindow(void)
{
  nsresult rv;
  GtkWidget *ownerAsWidget = GTK_WIDGET(mOwner->mOwningWidget);

  
  
  mBaseWindow = do_QueryInterface(mWebBrowser);
  rv = mBaseWindow->InitWindow(GTK_WIDGET(mOwner->mOwningWidget),
             nsnull,
             0, 0,
             ownerAsWidget->allocation.width,
             ownerAsWidget->allocation.height);
  if (NS_FAILED(rv))
    return rv;

  rv = mBaseWindow->Create();
  if (NS_FAILED(rv))
    return rv;

  return NS_OK;
}

void
EmbedWindow::ReleaseChildren(void)
{
  ExitModalEventLoop(PR_FALSE);

  mBaseWindow->Destroy();
  mBaseWindow = 0;
  mWebBrowser = 0;
}



NS_IMPL_ADDREF(EmbedWindow)
NS_IMPL_RELEASE(EmbedWindow)

NS_INTERFACE_MAP_BEGIN(EmbedWindow)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIWebBrowserChrome)
  NS_INTERFACE_MAP_ENTRY(nsIWebBrowserChrome)
  NS_INTERFACE_MAP_ENTRY(nsIWebBrowserChromeFocus)
  NS_INTERFACE_MAP_ENTRY(nsIEmbeddingSiteWindow)

  NS_INTERFACE_MAP_ENTRY(nsIInterfaceRequestor)
NS_INTERFACE_MAP_END



NS_IMETHODIMP
EmbedWindow::SetStatus(PRUint32 aStatusType, const PRUnichar *aStatus)
{
  switch (aStatusType) {
    case STATUS_SCRIPT:
    {
      mJSStatus = aStatus; 
      gtk_signal_emit(GTK_OBJECT(mOwner->mOwningWidget),
          moz_embed_signals[JS_STATUS]);
    }
    break;
    case STATUS_SCRIPT_DEFAULT:
    
    break;
    case STATUS_LINK:
    {
      mLinkMessage = aStatus;
      gtk_signal_emit(GTK_OBJECT(mOwner->mOwningWidget),
          moz_embed_signals[LINK_MESSAGE]);
    }
    break;
  }
  return NS_OK;
}

NS_IMETHODIMP
EmbedWindow::GetWebBrowser(nsIWebBrowser **aWebBrowser)
{
  *aWebBrowser = mWebBrowser;
  NS_IF_ADDREF(*aWebBrowser);
  return NS_OK;
}

NS_IMETHODIMP
EmbedWindow::SetWebBrowser(nsIWebBrowser *aWebBrowser)
{
  mWebBrowser = aWebBrowser;
  return NS_OK;
}

NS_IMETHODIMP
EmbedWindow::GetChromeFlags(PRUint32 *aChromeFlags)
{
  *aChromeFlags = mOwner->mChromeMask;
  return NS_OK;
}

NS_IMETHODIMP
EmbedWindow::SetChromeFlags(PRUint32 aChromeFlags)
{
  mOwner->SetChromeMask(aChromeFlags);
  return NS_OK;
}

NS_IMETHODIMP
EmbedWindow::DestroyBrowserWindow(void)
{
  
  mOwner->mIsDestroyed = PR_TRUE;

  gtk_signal_emit(GTK_OBJECT(mOwner->mOwningWidget),
      moz_embed_signals[DESTROY_BROWSER]);
  return NS_OK;
}

NS_IMETHODIMP
EmbedWindow::SizeBrowserTo(PRInt32 aCX, PRInt32 aCY)
{
  gtk_signal_emit(GTK_OBJECT(mOwner->mOwningWidget),
      moz_embed_signals[SIZE_TO], aCX, aCY);
  return NS_OK;
}

NS_IMETHODIMP
EmbedWindow::ShowAsModal(void)
{
  mIsModal = PR_TRUE;
  GtkWidget *toplevel;
  toplevel = gtk_widget_get_toplevel(GTK_WIDGET(mOwner->mOwningWidget));
  gtk_grab_add(toplevel);
  gtk_main();
  return NS_OK;
}

NS_IMETHODIMP
EmbedWindow::IsWindowModal(PRBool *_retval)
{
  *_retval = mIsModal;
  return NS_OK;
}

NS_IMETHODIMP
EmbedWindow::ExitModalEventLoop(nsresult aStatus)
{
  if (mIsModal) {
    GtkWidget *toplevel;
    toplevel = gtk_widget_get_toplevel(GTK_WIDGET(mOwner->mOwningWidget));
    gtk_grab_remove(toplevel);
    mIsModal = PR_FALSE;
    gtk_main_quit();
  }
  return NS_OK;
}



NS_IMETHODIMP
EmbedWindow::FocusNextElement()
{
  GtkWidget *toplevel;
  toplevel = gtk_widget_get_toplevel(GTK_WIDGET(mOwner->mOwningWidget));
  if (!GTK_WIDGET_TOPLEVEL(toplevel))
    return NS_OK;

  g_signal_emit_by_name(G_OBJECT(toplevel), "move_focus",
      GTK_DIR_TAB_FORWARD);

  return NS_OK;
}

NS_IMETHODIMP
EmbedWindow::FocusPrevElement()
{
  GtkWidget *toplevel;
  toplevel = gtk_widget_get_toplevel(GTK_WIDGET(mOwner->mOwningWidget));
  if (!GTK_WIDGET_TOPLEVEL(toplevel))
    return NS_OK;

  g_signal_emit_by_name(G_OBJECT(toplevel), "move_focus",
      GTK_DIR_TAB_BACKWARD);

  return NS_OK;
}



NS_IMETHODIMP
EmbedWindow::SetDimensions(PRUint32 aFlags, PRInt32 aX, PRInt32 aY,
         PRInt32 aCX, PRInt32 aCY)
{
  if (aFlags & nsIEmbeddingSiteWindow::DIM_FLAGS_POSITION &&
      (aFlags & (nsIEmbeddingSiteWindow::DIM_FLAGS_SIZE_INNER |
     nsIEmbeddingSiteWindow::DIM_FLAGS_SIZE_OUTER))) {
    return mBaseWindow->SetPositionAndSize(aX, aY, aCX, aCY, PR_TRUE);
  }
  else if (aFlags & nsIEmbeddingSiteWindow::DIM_FLAGS_POSITION) {
    return mBaseWindow->SetPosition(aX, aY);
  }
  else if (aFlags & (nsIEmbeddingSiteWindow::DIM_FLAGS_SIZE_INNER |
         nsIEmbeddingSiteWindow::DIM_FLAGS_SIZE_OUTER)) {
    return mBaseWindow->SetSize(aCX, aCY, PR_TRUE);
  }
  return NS_ERROR_INVALID_ARG;
}

NS_IMETHODIMP
EmbedWindow::GetDimensions(PRUint32 aFlags, PRInt32 *aX,
         PRInt32 *aY, PRInt32 *aCX, PRInt32 *aCY)
{
  if (aFlags & nsIEmbeddingSiteWindow::DIM_FLAGS_POSITION &&
      (aFlags & (nsIEmbeddingSiteWindow::DIM_FLAGS_SIZE_INNER |
     nsIEmbeddingSiteWindow::DIM_FLAGS_SIZE_OUTER))) {
    return mBaseWindow->GetPositionAndSize(aX, aY, aCX, aCY);
  }
  else if (aFlags & nsIEmbeddingSiteWindow::DIM_FLAGS_POSITION) {
    return mBaseWindow->GetPosition(aX, aY);
  }
  else if (aFlags & (nsIEmbeddingSiteWindow::DIM_FLAGS_SIZE_INNER |
         nsIEmbeddingSiteWindow::DIM_FLAGS_SIZE_OUTER)) {
    return mBaseWindow->GetSize(aCX, aCY);
  }
  return NS_ERROR_INVALID_ARG;
}

NS_IMETHODIMP
EmbedWindow::SetFocus(void)
{
  
  return mBaseWindow->SetFocus();
}

NS_IMETHODIMP
EmbedWindow::GetTitle(PRUnichar **aTitle)
{
  *aTitle = ToNewUnicode(mTitle);
  return NS_OK;
}

NS_IMETHODIMP
EmbedWindow::SetTitle(const PRUnichar *aTitle)
{
  mTitle = aTitle;
  gtk_signal_emit(GTK_OBJECT(mOwner->mOwningWidget),
      moz_embed_signals[TITLE]);
  return NS_OK;
}

NS_IMETHODIMP
EmbedWindow::GetSiteWindow(void **aSiteWindow)
{
  GtkWidget *ownerAsWidget(GTK_WIDGET(mOwner->mOwningWidget));
  *aSiteWindow = NS_STATIC_CAST(void *, ownerAsWidget);
  return NS_OK;
}

NS_IMETHODIMP
EmbedWindow::GetVisibility(PRBool *aVisibility)
{
  *aVisibility = mVisibility ||
                 !mOwner->mIsChrome &&
                 mOwner->mOwningWidget &&
                 GTK_WIDGET_MAPPED(mOwner->mOwningWidget);
  return NS_OK;
}

NS_IMETHODIMP
EmbedWindow::SetVisibility(PRBool aVisibility)
{
  
  
  mVisibility = aVisibility;

  
  
  if (mOwner->mIsChrome && !mOwner->mChromeLoaded)
    return NS_OK;

  gtk_signal_emit(GTK_OBJECT(mOwner->mOwningWidget),
      moz_embed_signals[VISIBILITY],
      aVisibility);
  return NS_OK;
}



#if 0
static gint
tooltips_paint_window(GtkWidget *window)
{
  
  gtk_paint_flat_box(window->style, window->window,
                     GTK_STATE_NORMAL, GTK_SHADOW_OUT,
                     NULL, window, "tooltip",
                     0, 0,
                     window->allocation.width, window->allocation.height);

  return FALSE;
}

NS_IMETHODIMP
EmbedWindow::OnShowTooltip(PRInt32 aXCoords, PRInt32 aYCoords,
         const PRUnichar *aTipText)
{
  nsAutoString tipText(aTipText);

  const char* tipString = ToNewUTF8String(tipText);

  if (sTipWindow)
    gtk_widget_destroy(sTipWindow);

  
  nsCOMPtr<nsIWidget> mainWidget;
  mBaseWindow->GetMainWidget(getter_AddRefs(mainWidget));
  GdkWindow *window;
  window = NS_STATIC_CAST(GdkWindow *,
        mainWidget->GetNativeData(NS_NATIVE_WINDOW));
  gint root_x, root_y;
  gdk_window_get_origin(window, &root_x, &root_y);

  
  
  
  root_y += 10;

  sTipWindow = gtk_window_new(GTK_WINDOW_POPUP);
  gtk_widget_set_app_paintable(sTipWindow, TRUE);
  gtk_window_set_policy(GTK_WINDOW(sTipWindow), FALSE, FALSE, TRUE);
  
  gtk_widget_set_name(sTipWindow, "gtk-tooltips");

  
  GtkWidget *toplevel_window;
  toplevel_window = gtk_widget_get_toplevel(GTK_WIDGET(mOwner->mOwningWidget));
  if (!GTK_WINDOW(toplevel_window)) {
    NS_ERROR("no gtk window in hierarchy!\n");
    return NS_ERROR_FAILURE;
  }
  gtk_window_set_transient_for(GTK_WINDOW(sTipWindow),
             GTK_WINDOW(toplevel_window));

  
  gtk_widget_realize(sTipWindow);

  gtk_signal_connect(GTK_OBJECT(sTipWindow), "expose_event",
                     GTK_SIGNAL_FUNC(tooltips_paint_window), NULL);

  
  GtkWidget *label = gtk_label_new(tipString);
  
  gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
  gtk_container_add(GTK_CONTAINER(sTipWindow), label);
  gtk_container_set_border_width(GTK_CONTAINER(sTipWindow), 4);
  
  gtk_widget_set_uposition(sTipWindow, aXCoords + root_x,
         aYCoords + root_y);

  
  gtk_widget_show_all(sTipWindow);

  nsMemory::Free((void*)tipString);

  return NS_OK;
}

NS_IMETHODIMP
EmbedWindow::OnHideTooltip(void)
{
  if (sTipWindow)
    gtk_widget_destroy(sTipWindow);
  sTipWindow = NULL;
  return NS_OK;
}
#endif



NS_IMETHODIMP
EmbedWindow::GetInterface(const nsIID &aIID, void** aInstancePtr)
{
  nsresult rv;

  rv = QueryInterface(aIID, aInstancePtr);

  
  if (NS_FAILED(rv) || !*aInstancePtr) {
    nsCOMPtr<nsIInterfaceRequestor> ir = do_QueryInterface(mWebBrowser);
    return ir->GetInterface(aIID, aInstancePtr);
  }

  return rv;
}
