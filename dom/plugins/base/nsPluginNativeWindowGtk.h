



#ifndef _nsPluginNativeWindowGdk_h_
#define _nsPluginNativeWindowGdk_h_

#include "nsPluginNativeWindow.h"
#include "nsNPAPIPlugin.h"
#include "npapi.h"
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gdk/gdk.h>
#if (GTK_MAJOR_VERSION == 3)
#include <gtk/gtkx.h>
#else
#include "gtk2xtbin.h"
#endif
#include "mozilla/X11Util.h"

class nsPluginNativeWindowGtk : public nsPluginNativeWindow {
public:
  nsPluginNativeWindowGtk();
  virtual ~nsPluginNativeWindowGtk();

  virtual nsresult CallSetWindow(nsRefPtr<nsNPAPIPluginInstance> &aPluginInstance);
  nsresult CreateXEmbedWindow(bool aEnableXtFocus);
  void SetAllocation();

  XID GetWindow() const
  {
    return static_cast<XID>(reinterpret_cast<uintptr_t>(window));
  }

private:
  void SetWindow(XID aWindow)
  {
    window = reinterpret_cast<void*>(static_cast<uintptr_t>(aWindow));
  }

  NPSetWindowCallbackStruct mWsInfo;
  



  GtkWidget* mSocketWidget;
#if (MOZ_WIDGET_GTK == 2)
  nsresult  CreateXtWindow();
#endif
};

#endif
