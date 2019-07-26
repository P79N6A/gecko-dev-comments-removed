










#include "nsDebug.h"
#include "nsPluginNativeWindow.h"
#include "nsNPAPIPlugin.h"
#include "npapi.h"
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gdk/gdk.h>

#include "gtk2xtbin.h"
#include "mozilla/X11Util.h"

class nsPluginNativeWindowGtk2 : public nsPluginNativeWindow {
public: 
  nsPluginNativeWindowGtk2();
  virtual ~nsPluginNativeWindowGtk2();

  virtual nsresult CallSetWindow(nsRefPtr<nsNPAPIPluginInstance> &aPluginInstance);
private:
  void SetWindow(XID aWindow)
  {
    window = reinterpret_cast<void*>(static_cast<uintptr_t>(aWindow));
  }
  XID GetWindow() const
  {
    return static_cast<XID>(reinterpret_cast<uintptr_t>(window));
  }

  NPSetWindowCallbackStruct mWsInfo;
  



  GtkWidget* mSocketWidget;
  nsresult  CreateXEmbedWindow(bool aEnableXtFocus);
  nsresult  CreateXtWindow();
  void      SetAllocation();
};

static gboolean plug_removed_cb   (GtkWidget *widget, gpointer data);
static void socket_unrealize_cb   (GtkWidget *widget, gpointer data);

nsPluginNativeWindowGtk2::nsPluginNativeWindowGtk2() : nsPluginNativeWindow()
{
  
  window = nullptr; 
  x = 0; 
  y = 0; 
  width = 0; 
  height = 0; 
  memset(&clipRect, 0, sizeof(clipRect));
  ws_info = &mWsInfo;
  type = NPWindowTypeWindow;
  mSocketWidget = 0;
  mWsInfo.type = 0;
  mWsInfo.display = nullptr;
  mWsInfo.visual = nullptr;
  mWsInfo.colormap = 0;
  mWsInfo.depth = 0;
}

nsPluginNativeWindowGtk2::~nsPluginNativeWindowGtk2() 
{
  if(mSocketWidget) {
    gtk_widget_destroy(mSocketWidget);
  }
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

nsresult nsPluginNativeWindowGtk2::CallSetWindow(nsRefPtr<nsNPAPIPluginInstance> &aPluginInstance)
{
  if (aPluginInstance) {
    if (type == NPWindowTypeWindow) {
      if (!mSocketWidget) {
        nsresult rv;

        
        
        
        
        
        
        
        int needsXEmbed = 0;
        rv = aPluginInstance->GetValueFromPlugin(NPPVpluginNeedsXEmbed, &needsXEmbed);
        
        if (NS_FAILED(rv)) {
          needsXEmbed = 0;
        }
#ifdef DEBUG
        printf("nsPluginNativeWindowGtk2: NPPVpluginNeedsXEmbed=%d\n", needsXEmbed);
#endif

        bool isOOPPlugin = aPluginInstance->GetPlugin()->GetLibrary()->IsOOP();
        if (needsXEmbed || isOOPPlugin) {        
          bool enableXtFocus = !needsXEmbed;
          rv = CreateXEmbedWindow(enableXtFocus);
        }
        else {
          rv = CreateXtWindow();
        }

        if (NS_FAILED(rv)) {
          return NS_ERROR_FAILURE;
        }
      }

      if (!mSocketWidget) {
        return NS_ERROR_FAILURE;
      }

      
      
      
      if (GTK_IS_XTBIN(mSocketWidget)) {
        gtk_xtbin_resize(mSocketWidget, width, height);
        
        SetWindow(GTK_XTBIN(mSocketWidget)->xtwindow);
      }
      else { 
        SetAllocation();
        SetWindow(gtk_socket_get_id(GTK_SOCKET(mSocketWidget)));
      }
#ifdef DEBUG
      printf("nsPluginNativeWindowGtk2: call SetWindow with xid=%p\n", (void *)window);
#endif
    } 
    aPluginInstance->SetWindow(this);
  }
  else if (mPluginInstance)
    mPluginInstance->SetWindow(nullptr);

  SetPluginInstance(aPluginInstance);
  return NS_OK;
}

nsresult nsPluginNativeWindowGtk2::CreateXEmbedWindow(bool aEnableXtFocus) {
  NS_ASSERTION(!mSocketWidget,"Already created a socket widget!");
  GdkDisplay *display = gdk_display_get_default();
  GdkWindow *parent_win = gdk_x11_window_lookup_for_display(display, GetWindow());
  mSocketWidget = gtk_socket_new();

  
  gtk_widget_set_parent_window(mSocketWidget, parent_win);

  
  
  g_object_set_data(G_OBJECT(mSocketWidget), "enable-xt-focus", (void *)aEnableXtFocus);

  
  
  
  
  g_signal_connect(mSocketWidget, "plug_removed",
                   G_CALLBACK(plug_removed_cb), NULL);

  g_signal_connect(mSocketWidget, "unrealize",
                   G_CALLBACK(socket_unrealize_cb), NULL);

  g_signal_connect(mSocketWidget, "destroy",
                   G_CALLBACK(gtk_widget_destroyed), &mSocketWidget);

  gpointer user_data = NULL;
  gdk_window_get_user_data(parent_win, &user_data);

  GtkContainer *container = GTK_CONTAINER(user_data);
  gtk_container_add(container, mSocketWidget);
  gtk_widget_realize(mSocketWidget);

  
  
  
  
  
  
#if (MOZ_WIDGET_GTK == 2)
  gdk_window_set_back_pixmap(gtk_widget_get_window(mSocketWidget), NULL, FALSE);
#endif

  
  SetAllocation();

  gtk_widget_show(mSocketWidget);

  gdk_flush();
  SetWindow(gtk_socket_get_id(GTK_SOCKET(mSocketWidget)));

  
  
  GdkWindow *gdkWindow = gdk_window_lookup(GetWindow());
  if(!gdkWindow)
    return NS_ERROR_FAILURE;

  mWsInfo.display = GDK_WINDOW_XDISPLAY(gdkWindow);
#if (MOZ_WIDGET_GTK == 2)
  mWsInfo.colormap = GDK_COLORMAP_XCOLORMAP(gdk_drawable_get_colormap(gdkWindow));
  GdkVisual* gdkVisual = gdk_drawable_get_visual(gdkWindow);
  mWsInfo.depth = gdkVisual->depth;
#else
  mWsInfo.colormap = None;
  GdkVisual* gdkVisual = gdk_window_get_visual(gdkWindow);
  mWsInfo.depth = gdk_visual_get_depth(gdkVisual);
#endif
  mWsInfo.visual = GDK_VISUAL_XVISUAL(gdkVisual);
    
  return NS_OK;
}

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

#ifdef DEBUG      
  printf("About to create new xtbin of %i X %i from %p...\n",
         width, height, (void*)window);
#endif
  GdkDisplay *display = gdk_display_get_default();
  GdkWindow *gdkWindow = gdk_x11_window_lookup_for_display(display, GetWindow());
  mSocketWidget = gtk_xtbin_new(gdkWindow, 0);
  
  
  if (!mSocketWidget)
    return NS_ERROR_FAILURE;

  g_signal_connect(mSocketWidget, "destroy",
                   G_CALLBACK(gtk_widget_destroyed), &mSocketWidget);

  gtk_widget_set_size_request(mSocketWidget, width, height);

#ifdef DEBUG
  printf("About to show xtbin(%p)...\n", (void*)mSocketWidget); fflush(NULL);
#endif
  gtk_widget_show(mSocketWidget);
#ifdef DEBUG
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


gboolean
plug_removed_cb (GtkWidget *widget, gpointer data)
{
  
  return TRUE;
}

static void
socket_unrealize_cb(GtkWidget *widget, gpointer data)
{
  
  
  GdkWindow* socket_window =  gtk_widget_get_window(widget);
  GdkDisplay* gdkDisplay = gdk_display_get_default();
  Display* display = GDK_DISPLAY_XDISPLAY(gdkDisplay);

  
  
  gdk_error_trap_push();

  Window root, parent;
  Window* children;
  unsigned int nchildren;
  if (!XQueryTree(display, gdk_x11_window_get_xid(socket_window),
                  &root, &parent, &children, &nchildren))
    return;

  for (unsigned int i = 0; i < nchildren; ++i) {
    Window child = children[i];
    if (!gdk_x11_window_lookup_for_display(gdkDisplay, child)) {
      
      XUnmapWindow(display, child);
      XReparentWindow(display, child, DefaultRootWindow(display), 0, 0);
    }
  }

  if (children) XFree(children);

  mozilla::FinishX(display);
  gdk_error_trap_pop();
}
