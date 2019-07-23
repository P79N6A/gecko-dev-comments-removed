

































#include "nptest_platform.h"
#include "npapi.h"
#include <gdk/gdk.h>
#ifdef MOZ_X11
#include <gdk/gdkx.h>
#endif
#include <gtk/gtk.h>

bool
pluginSupportsWindowMode()
{
  return true;
}

bool
pluginSupportsWindowlessMode()
{
  return true;
}

NPError
pluginInstanceInit(InstanceData* instanceData)
{
#ifdef MOZ_X11
  return NPERR_NO_ERROR;
#else
  
  return NPERR_INCOMPATIBLE_VERSION_ERROR;
#endif
}

void
pluginInstanceShutdown(InstanceData* instanceData)
{
  GtkWidget* plug = static_cast<GtkWidget*>(instanceData->platformData);
  if (plug) {
    gtk_widget_destroy(plug);
    instanceData->platformData = 0;
  }
}

static void 
SetCairoRGBA(cairo_t* cairoWindow, PRUint32 rgba)
{
  float b = (rgba & 0xFF) / 255.0;
  float g = ((rgba & 0xFF00) >> 8) / 255.0;
  float r = ((rgba & 0xFF0000) >> 16) / 255.0;
  float a = ((rgba & 0xFF000000) >> 24) / 255.0;

  cairo_set_source_rgba(cairoWindow, r, g, b, a);
}

static void
pluginDrawSolid(InstanceData* instanceData, GdkDrawable* gdkWindow,
                int x, int y, int width, int height)
{
  cairo_t* cairoWindow = gdk_cairo_create(gdkWindow);

  GdkRectangle windowRect = { x, y, width, height };
  gdk_cairo_rectangle(cairoWindow, &windowRect);
  SetCairoRGBA(cairoWindow, instanceData->scriptableObject->drawColor);

  cairo_fill(cairoWindow);
  cairo_destroy(cairoWindow);
}

static void
pluginDrawWindow(InstanceData* instanceData, GdkDrawable* gdkWindow)
{
  NPWindow& window = instanceData->window;
  
  
  int x = instanceData->hasWidget ? 0 : window.x;
  int y = instanceData->hasWidget ? 0 : window.y;
  int width = window.width;
  int height = window.height;

  if (instanceData->scriptableObject->drawMode == DM_SOLID_COLOR) {
    
    pluginDrawSolid(instanceData, gdkWindow, x, y, width, height);
    return;
  }

  NPP npp = instanceData->npp;
  if (!npp)
    return;

  const char* uaString = NPN_UserAgent(npp);
  if (!uaString)
    return;

  GdkGC* gdkContext = gdk_gc_new(gdkWindow);

  
  GdkColor grey;
  grey.red = grey.blue = grey.green = 32767;
  gdk_gc_set_rgb_fg_color(gdkContext, &grey);
  gdk_draw_rectangle(gdkWindow, gdkContext, TRUE, x, y, width, height);

  
  GdkColor black;
  black.red = black.green = black.blue = 0;
  gdk_gc_set_rgb_fg_color(gdkContext, &black);
  gdk_gc_set_line_attributes(gdkContext, 3, GDK_LINE_SOLID, GDK_CAP_NOT_LAST, GDK_JOIN_MITER);
  gdk_draw_rectangle(gdkWindow, gdkContext, FALSE, x + 1, y + 1,
                     width - 3, height - 3);

  
  PangoContext* pangoContext = gdk_pango_context_get();
  PangoLayout* pangoTextLayout = pango_layout_new(pangoContext);
  pango_layout_set_width(pangoTextLayout, (width - 10) * PANGO_SCALE);
  pango_layout_set_text(pangoTextLayout, uaString, -1);
  gdk_draw_layout(gdkWindow, gdkContext, x + 5, y + 5, pangoTextLayout);
  g_object_unref(pangoTextLayout);

  g_object_unref(gdkContext);
}

static gboolean
ExposeWidget(GtkWidget* widget, GdkEventExpose* event,
             gpointer user_data)
{
  InstanceData* instanceData = static_cast<InstanceData*>(user_data);
  pluginDrawWindow(instanceData, event->window);
  return TRUE;
}

void
pluginWidgetInit(InstanceData* instanceData, void* oldWindow)
{
#ifdef MOZ_X11
  GtkWidget* oldPlug = static_cast<GtkWidget*>(instanceData->platformData);
  if (oldPlug) {
    gtk_widget_destroy(oldPlug);
    instanceData->platformData = 0;
  }

  GdkNativeWindow nativeWinId =
    reinterpret_cast<XID>(instanceData->window.window);

  
  GtkWidget* plug = gtk_plug_new(nativeWinId);

  
  GTK_WIDGET_SET_FLAGS (GTK_WIDGET(plug), GTK_CAN_FOCUS);

  
  gtk_widget_add_events(plug, GDK_EXPOSURE_MASK);
  g_signal_connect(G_OBJECT(plug), "event", G_CALLBACK(ExposeWidget),
                   instanceData);
  gtk_widget_show(plug);

  instanceData->platformData = plug;
#endif
}

int16_t
pluginHandleEvent(InstanceData* instanceData, void* event)
{
#ifdef MOZ_X11
  XEvent *nsEvent = (XEvent *)event;

  if (nsEvent->type != GraphicsExpose)
    return 0;

  XGraphicsExposeEvent *expose = &nsEvent->xgraphicsexpose;
  instanceData->window.window = (void*)(expose->drawable);

  GdkNativeWindow nativeWinId =
    reinterpret_cast<XID>(instanceData->window.window);
  GdkDrawable* gdkWindow = GDK_DRAWABLE(gdk_window_foreign_new(nativeWinId));  
  pluginDrawWindow(instanceData, gdkWindow);
  g_object_unref(gdkWindow);
#endif
  return 0;
}
