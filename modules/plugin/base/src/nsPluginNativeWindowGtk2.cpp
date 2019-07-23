











































#include "nsDebug.h"
#include "nsPluginNativeWindow.h"
#include "npapi.h"
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gdk/gdk.h>

#ifdef MOZ_PLATFORM_HILDON
#define MOZ_COMPOSITED_PLUGINS
#endif

#ifdef MOZ_COMPOSITED_PLUGINS
extern "C" {
#include <X11/extensions/Xdamage.h>
#include <X11/extensions/Xcomposite.h>
}
#endif

#include "gtk2xtbin.h"

class nsPluginNativeWindowGtk2 : public nsPluginNativeWindow {
public: 
  nsPluginNativeWindowGtk2();
  virtual ~nsPluginNativeWindowGtk2();

  virtual nsresult CallSetWindow(nsCOMPtr<nsIPluginInstance> &aPluginInstance);
private:
  NPSetWindowCallbackStruct mWsInfo;
  



  GtkWidget* mSocketWidget;
  nsresult  CreateXEmbedWindow();
  nsresult  CreateXtWindow();
  void      SetAllocation();
  PRBool    CanGetValueFromPlugin(nsCOMPtr<nsIPluginInstance> &aPluginInstance);
#ifdef MOZ_COMPOSITED_PLUGINS
  nsresult  CreateXCompositedWindow();
  static GdkFilterReturn    plugin_composite_filter_func (GdkXEvent *xevent,
    GdkEvent *event,
    gpointer data);

  Damage     mDamage;
  GtkWidget* mParentWindow;
#endif
};

static gboolean plug_removed_cb   (GtkWidget *widget, gpointer data);

nsPluginNativeWindowGtk2::nsPluginNativeWindowGtk2() : nsPluginNativeWindow()
{
  
  window = nsnull; 
  x = 0; 
  y = 0; 
  width = 0; 
  height = 0; 
  memset(&clipRect, 0, sizeof(clipRect));
  ws_info = &mWsInfo;
  type = NPWindowTypeWindow;
  mSocketWidget = 0;
  mWsInfo.type = 0;
  mWsInfo.display = nsnull;
  mWsInfo.visual = nsnull;
  mWsInfo.colormap = 0;
  mWsInfo.depth = 0;
#ifdef MOZ_COMPOSITED_PLUGINS
  mDamage = 0;
  mParentWindow = 0;
#endif
}

nsPluginNativeWindowGtk2::~nsPluginNativeWindowGtk2() 
{
  if(mSocketWidget) {
    gtk_widget_destroy(mSocketWidget);
  }

#ifdef MOZ_COMPOSITED_PLUGINS
  if (mParentWindow) {
    gtk_widget_destroy(mParentWindow);
  }

  if (mDamage) {
    gdk_window_remove_filter (nsnull, plugin_composite_filter_func, this);
  }
#endif
}

nsresult PLUG_NewPluginNativeWindow(nsPluginNativeWindow ** aPluginNativeWindow)
{
  NS_ENSURE_ARG_POINTER(aPluginNativeWindow);
  *aPluginNativeWindow = new nsPluginNativeWindowGtk2();
  return *aPluginNativeWindow ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

nsresult PLUG_DeletePluginNativeWindow(nsPluginNativeWindow * aPluginNativeWindow)
{
  NS_ENSURE_ARG_POINTER(aPluginNativeWindow);
  nsPluginNativeWindowGtk2 *p = (nsPluginNativeWindowGtk2 *)aPluginNativeWindow;
  delete p;
  return NS_OK;
}

#ifdef MOZ_COMPOSITED_PLUGINS

static int xdamage_event_base;

GdkFilterReturn
nsPluginNativeWindowGtk2::plugin_composite_filter_func (GdkXEvent *xevent,
    GdkEvent *event,
    gpointer data)
{
  nsPluginNativeWindowGtk2 *native_window = (nsPluginNativeWindowGtk2*)data;
  XDamageNotifyEvent *ev;
  ev = (XDamageNotifyEvent *) xevent;
  if (ev->type != xdamage_event_base + XDamageNotify)
    return GDK_FILTER_CONTINUE;

  
  XDamageSubtract (GDK_DISPLAY(), native_window->mDamage, None, None);

  
  NPRect rect;
  rect.top = ev->area.x;
  rect.left = ev->area.y;
  rect.right = ev->area.x + ev->area.width;
  rect.bottom = ev->area.y + ev->area.height;

  if (native_window->mPluginInstance)
    native_window->mPluginInstance->InvalidateRect(&rect);

  return GDK_FILTER_REMOVE;
}
#endif

nsresult nsPluginNativeWindowGtk2::CallSetWindow(nsCOMPtr<nsIPluginInstance> &aPluginInstance)
{
  if(aPluginInstance) {
    if (type == NPWindowTypeWindow) {
      nsresult rv;
      if(!mSocketWidget) {
        PRBool needXEmbed = PR_FALSE;
        if (CanGetValueFromPlugin(aPluginInstance)) {
          rv = aPluginInstance->GetValueFromPlugin(NPPVpluginNeedsXEmbed, &needXEmbed);
#ifdef DEBUG
          printf("nsPluginNativeWindowGtk2: NPPVpluginNeedsXEmbed=%d\n", needXEmbed);
#endif
        }
        nsresult rv;
        if(needXEmbed) {
#ifdef MOZ_COMPOSITED_PLUGINS
          rv = CreateXCompositedWindow();
#else
          rv = CreateXEmbedWindow();
#endif
        }
        else {
          rv = CreateXtWindow();
        }
        if(NS_FAILED(rv))
          return NS_ERROR_FAILURE;
      }

      if(!mSocketWidget)
        return NS_ERROR_FAILURE;

      
      
      
      if(GTK_IS_XTBIN(mSocketWidget)) {
        gtk_xtbin_resize(mSocketWidget, width, height);
        
        window = (void*)GTK_XTBIN(mSocketWidget)->xtwindow;
      }
      else { 
        SetAllocation();
        window = (void*)gtk_socket_get_id(GTK_SOCKET(mSocketWidget));
      }
#ifdef DEBUG
      printf("nsPluginNativeWindowGtk2: call SetWindow with xid=%p\n", (void *)window);
#endif
    } 
    aPluginInstance->SetWindow(this);
  }
  else if (mPluginInstance)
    mPluginInstance->SetWindow(nsnull);

  SetPluginInstance(aPluginInstance);
  return NS_OK;
}

nsresult nsPluginNativeWindowGtk2::CreateXEmbedWindow() {
  NS_ASSERTION(!mSocketWidget,"Already created a socket widget!");

  GdkWindow *parent_win = gdk_window_lookup((XID)window);
  mSocketWidget = gtk_socket_new();

  
  gtk_widget_set_parent_window(mSocketWidget, parent_win);

  
  
  
  
  g_signal_connect(mSocketWidget, "plug_removed",
                   G_CALLBACK(plug_removed_cb), NULL);

  g_signal_connect(mSocketWidget, "destroy",
                   G_CALLBACK(gtk_widget_destroyed), &mSocketWidget);

  gpointer user_data = NULL;
  gdk_window_get_user_data(parent_win, &user_data);

  GtkContainer *container = GTK_CONTAINER(user_data);
  gtk_container_add(container, mSocketWidget);
  gtk_widget_realize(mSocketWidget);

  
  
  
  
  
  gdk_window_set_back_pixmap(mSocketWidget->window, NULL, FALSE);

  
  SetAllocation();

  gtk_widget_show(mSocketWidget);

  gdk_flush();
  window = (void*)gtk_socket_get_id(GTK_SOCKET(mSocketWidget));

  
  
  GdkWindow *gdkWindow = gdk_window_lookup((XID)window);
  if(!gdkWindow)
    return NS_ERROR_FAILURE;

  mWsInfo.display = GDK_WINDOW_XDISPLAY(gdkWindow);
  mWsInfo.colormap = GDK_COLORMAP_XCOLORMAP(gdk_drawable_get_colormap(gdkWindow));
  GdkVisual* gdkVisual = gdk_drawable_get_visual(gdkWindow);
  mWsInfo.visual = GDK_VISUAL_XVISUAL(gdkVisual);
  mWsInfo.depth = gdkVisual->depth;

  return NS_OK;
}

#ifdef MOZ_COMPOSITED_PLUGINS
#include <dlfcn.h>
nsresult nsPluginNativeWindowGtk2::CreateXCompositedWindow() {
  NS_ASSERTION(!mSocketWidget,"Already created a socket widget!");

  mParentWindow = gtk_window_new(GTK_WINDOW_POPUP);
  mSocketWidget = gtk_socket_new();
  GdkWindow *parent_win = mParentWindow->window;

  
  gtk_widget_set_parent_window(mSocketWidget, parent_win);

  
  
  
  
  g_signal_connect(mSocketWidget, "plug_removed",
                   G_CALLBACK(plug_removed_cb), NULL);

  g_signal_connect(mSocketWidget, "destroy",
                   G_CALLBACK(gtk_widget_destroyed), &mSocketWidget);

  


  GtkContainer *container = GTK_CONTAINER(mParentWindow);
  gtk_container_add(container, mSocketWidget);
  gtk_widget_realize(mSocketWidget);

  
  SetAllocation();
  gtk_widget_set_size_request (mSocketWidget, width, height);
  
  gtk_window_move (GTK_WINDOW(mParentWindow), width+1000, height+1000);


  gtk_widget_show(mSocketWidget);
  gtk_widget_show_all(mParentWindow);

  
  mPlugWindow = (mSocketWidget);

  gdk_flush();
  window = (void*)gtk_socket_get_id(GTK_SOCKET(mSocketWidget));

  

  

  if (!mDamage) {
    

    gdk_window_add_filter (parent_win, plugin_composite_filter_func, this);

    int junk;
    if (!XDamageQueryExtension (GDK_DISPLAY (), &xdamage_event_base, &junk))
      printf ("This requires the XDamage extension");

    mDamage = XDamageCreate(GDK_DISPLAY(), (Drawable)window, XDamageReportNonEmpty);
    XCompositeRedirectWindow (GDK_DISPLAY(),
        (Drawable)window,
        CompositeRedirectManual);

    




    static void *libplayback_handle;
    if (!libplayback_handle) {
      libplayback_handle = dlopen("libplayback-1.so.0", RTLD_NOW);
    }

  }

  
  
  GdkWindow *gdkWindow = gdk_window_lookup((XID)window);
  mWsInfo.display = GDK_WINDOW_XDISPLAY(gdkWindow);
  mWsInfo.colormap = GDK_COLORMAP_XCOLORMAP(gdk_drawable_get_colormap(gdkWindow));
  GdkVisual* gdkVisual = gdk_drawable_get_visual(gdkWindow);
  mWsInfo.visual = GDK_VISUAL_XVISUAL(gdkVisual);
  mWsInfo.depth = gdkVisual->depth;

  return NS_OK;
}
#endif

void nsPluginNativeWindowGtk2::SetAllocation() {
  if (!mSocketWidget)
    return;

  GtkAllocation new_allocation;
  new_allocation.x = 0;
  new_allocation.y = 0;
  new_allocation.width = width;
  new_allocation.height = height;
  gtk_widget_size_allocate(mSocketWidget, &new_allocation);
}

nsresult nsPluginNativeWindowGtk2::CreateXtWindow() {
  NS_ASSERTION(!mSocketWidget,"Already created a socket widget!");

#ifdef NS_DEBUG      
  printf("About to create new xtbin of %i X %i from %p...\n",
         width, height, (void*)window);
#endif
  GdkWindow *gdkWindow = gdk_window_lookup((XID)window);
  mSocketWidget = gtk_xtbin_new(gdkWindow, 0);
  
  
  if (!mSocketWidget)
    return NS_ERROR_FAILURE;

  g_signal_connect(mSocketWidget, "destroy",
                   G_CALLBACK(gtk_widget_destroyed), &mSocketWidget);

  gtk_widget_set_size_request(mSocketWidget, width, height);

#ifdef NS_DEBUG
  printf("About to show xtbin(%p)...\n", (void*)mSocketWidget); fflush(NULL);
#endif
  gtk_widget_show(mSocketWidget);
#ifdef NS_DEBUG
  printf("completed gtk_widget_show(%p)\n", (void*)mSocketWidget); fflush(NULL);
#endif

  
  GtkXtBin* xtbin = GTK_XTBIN(mSocketWidget);
  
  mWsInfo.display = xtbin->xtdisplay;
  mWsInfo.colormap = xtbin->xtclient.xtcolormap;
  mWsInfo.visual = xtbin->xtclient.xtvisual;
  mWsInfo.depth = xtbin->xtclient.xtdepth;
  

  XFlush(mWsInfo.display);

  return NS_OK;
}

PRBool nsPluginNativeWindowGtk2::CanGetValueFromPlugin(nsCOMPtr<nsIPluginInstance> &aPluginInstance)
{
  return PR_TRUE;
}


gboolean
plug_removed_cb (GtkWidget *widget, gpointer data)
{
  
  return TRUE;
}


