











































#include "nsDebug.h"
#include "nsPluginNativeWindow.h"
#include "npapi.h"
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gdk/gdk.h>
#ifdef OJI
#include "plstr.h"
#include "nsIPlugin.h"
#include "nsIPluginHost.h"

static NS_DEFINE_CID(kPluginManagerCID, NS_PLUGINMANAGER_CID);
#endif

class nsPluginNativeWindowGtk2 : public nsPluginNativeWindow {
public: 
  nsPluginNativeWindowGtk2();
  virtual ~nsPluginNativeWindowGtk2();

  virtual nsresult CallSetWindow(nsCOMPtr<nsIPluginInstance> &aPluginInstance);
private:
  GtkWidget*  mGtkSocket;
  nsresult  CreateXEmbedWindow();
  void      SetAllocation();
  PRBool    CanGetValueFromPlugin(nsCOMPtr<nsIPluginInstance> &aPluginInstance);
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
  ws_info = nsnull;
  type = nsPluginWindowType_Window;
  mGtkSocket = 0;
}

nsPluginNativeWindowGtk2::~nsPluginNativeWindowGtk2() 
{
  if(mGtkSocket) {
    gtk_widget_destroy(mGtkSocket);
    mGtkSocket = 0;
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

nsresult nsPluginNativeWindowGtk2::CallSetWindow(nsCOMPtr<nsIPluginInstance> &aPluginInstance)
{
  if(aPluginInstance) {
    nsresult rv;
    PRBool val = PR_FALSE;
    if(!mGtkSocket) {
      if (CanGetValueFromPlugin(aPluginInstance))
        rv = aPluginInstance->GetValue
               ((nsPluginInstanceVariable)NPPVpluginNeedsXEmbed, &val);
    }
#ifdef DEBUG
    printf("nsPluginNativeWindowGtk2: NPPVpluginNeedsXEmbed=%d\n", val);
#endif
    if(val) {
      CreateXEmbedWindow();
    }

    if(mGtkSocket) {
      
      SetAllocation();
      window = (nsPluginPort *)gtk_socket_get_id(GTK_SOCKET(mGtkSocket));
    }
#ifdef DEBUG
    printf("nsPluginNativeWindowGtk2: call SetWindow with xid=%p\n", (void *)window);
#endif
    aPluginInstance->SetWindow(this);
  }
  else if (mPluginInstance)
    mPluginInstance->SetWindow(nsnull);

  SetPluginInstance(aPluginInstance);
  return NS_OK;
}

nsresult nsPluginNativeWindowGtk2::CreateXEmbedWindow() {
  if(!mGtkSocket) {
    GdkWindow *win = gdk_window_lookup((XID)window);
    mGtkSocket = gtk_socket_new();

    
    gtk_widget_set_parent_window(mGtkSocket, win);

    
    
    
    
    g_signal_connect(mGtkSocket, "plug_removed",
                     G_CALLBACK(plug_removed_cb), NULL);

    gpointer user_data = NULL;
    gdk_window_get_user_data(win, &user_data);

    GtkContainer *container = GTK_CONTAINER(user_data);
    gtk_container_add(container, mGtkSocket);
    gtk_widget_realize(mGtkSocket);

    
    SetAllocation();

    gtk_widget_show(mGtkSocket);

    gdk_flush();
    window = (nsPluginPort *)gtk_socket_get_id(GTK_SOCKET(mGtkSocket));
  }

  return NS_OK;
}

void nsPluginNativeWindowGtk2::SetAllocation() {
  if (!mGtkSocket)
    return;

  GtkAllocation new_allocation;
  new_allocation.x = 0;
  new_allocation.y = 0;
  new_allocation.width = width;
  new_allocation.height = height;
  gtk_widget_size_allocate(mGtkSocket, &new_allocation);
}

PRBool nsPluginNativeWindowGtk2::CanGetValueFromPlugin(nsCOMPtr<nsIPluginInstance> &aPluginInstance)
{
#ifdef OJI
  if(aPluginInstance) {
    nsresult rv;
    nsCOMPtr<nsIPluginInstancePeer> peer;

    rv = aPluginInstance->GetPeer(getter_AddRefs(peer));
    if (NS_SUCCEEDED(rv) && peer) {
      const char *aMimeType = nsnull;

      peer->GetMIMEType((nsMIMEType*)&aMimeType);
      if (aMimeType &&
          (PL_strncasecmp(aMimeType, "application/x-java-vm", 21) == 0 ||
           PL_strncasecmp(aMimeType, "application/x-java-applet", 25) == 0)) {
        nsCOMPtr<nsIPluginHost> pluginHost = do_GetService(kPluginManagerCID, &rv);
        if (NS_SUCCEEDED(rv) && pluginHost) {
          nsIPlugin* pluginFactory = NULL;

          rv = pluginHost->GetPluginFactory("application/x-java-vm", &pluginFactory);
          if (NS_SUCCEEDED(rv) && pluginFactory) {
            const char * jpiDescription;

            pluginFactory->GetValue(nsPluginVariable_DescriptionString, (void*)&jpiDescription);
            




            if (PL_strncasecmp(jpiDescription, "Java(TM) Plug-in", 16) == 0) {
              
              if (PL_strcasecmp(jpiDescription + 17, "1.5") < 0)
                return PR_FALSE;
            }
            if (PL_strncasecmp(jpiDescription, "<a href=\"http://www.blackdown.org/java-linux.html\">", 51) == 0) {
              
              if (PL_strcasecmp(jpiDescription + 92, "1.5") < 0)
                return PR_FALSE;
            }
            if (PL_strncasecmp(jpiDescription, "IBM Java(TM) Plug-in", 20) == 0) {
              
              if (PL_strcasecmp(jpiDescription + 27, "1.5") < 0)
                return PR_FALSE;
            }
          }
        }
      }
    }
  }
#endif

  return PR_TRUE;
}


gboolean
plug_removed_cb (GtkWidget *widget, gpointer data)
{
  
  return TRUE;
}


