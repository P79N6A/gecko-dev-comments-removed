


































#include "nptest_platform.h"
#include "npapi.h"
#include <gdk/gdk.h>
#ifdef MOZ_X11
#include <gdk/gdkx.h>
#endif
#include <gtk/gtk.h>









struct _PlatformData {
  Display* display;
  GtkWidget* plug;
};

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
  instanceData->platformData = static_cast<PlatformData*>
    (NPN_MemAlloc(sizeof(PlatformData)));
  if (!instanceData->platformData)
    return NPERR_OUT_OF_MEMORY_ERROR;

  instanceData->platformData->display = 0;
  instanceData->platformData->plug = 0;

  return NPERR_NO_ERROR;
#else
  
  return NPERR_INCOMPATIBLE_VERSION_ERROR;
#endif
}

void
pluginInstanceShutdown(InstanceData* instanceData)
{
  if (instanceData->hasWidget) {
    Window window = reinterpret_cast<XID>(instanceData->window.window);

    if (window != None) {
      
      
      XWindowAttributes attributes;
      if (!XGetWindowAttributes(instanceData->platformData->display, window,
                                &attributes))
        g_error("XGetWindowAttributes failed at plugin instance shutdown");
    }
  }

  GtkWidget* plug = instanceData->platformData->plug;
  if (plug) {
    instanceData->platformData->plug = 0;
    gtk_widget_destroy(plug);
  }

  NPN_MemFree(instanceData->platformData);
  instanceData->platformData = 0;
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

  if (!instanceData->hasWidget) {
    NPRect* clip = &instanceData->window.clipRect;
    cairo_rectangle(cairoWindow, clip->left, clip->top,
                    clip->right - clip->left, clip->bottom - clip->top);
    cairo_clip(cairoWindow);
  }

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

  if (0 && instanceData->scriptableObject->drawMode == DM_SOLID_COLOR) {
    
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
  if (!gdkContext)
    return;

  if (!instanceData->hasWidget) {
    NPRect* clip = &window.clipRect;
    GdkRectangle gdkClip = { clip->left, clip->top, clip->right - clip->left,
                             clip->bottom - clip->top };
    gdk_gc_set_clip_rectangle(gdkContext, &gdkClip);
  }

  
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

static gboolean
DeleteWidget(GtkWidget* widget, GdkEvent* event, gpointer user_data)
{
  InstanceData* instanceData = static_cast<InstanceData*>(user_data);
  
  
  if (instanceData->platformData->plug)
    g_error("plug removed"); 

  return FALSE;
}

void
pluginDoSetWindow(InstanceData* instanceData, NPWindow* newWindow)
{
  instanceData->window = *newWindow;

  NPSetWindowCallbackStruct *ws_info =
    static_cast<NPSetWindowCallbackStruct*>(newWindow->ws_info);
  instanceData->platformData->display = ws_info->display;
}

void
pluginWidgetInit(InstanceData* instanceData, void* oldWindow)
{
#ifdef MOZ_X11
  GtkWidget* oldPlug = instanceData->platformData->plug;
  if (oldPlug) {
    instanceData->platformData->plug = 0;
    gtk_widget_destroy(oldPlug);
  }

  GdkNativeWindow nativeWinId =
    reinterpret_cast<XID>(instanceData->window.window);

  
  GtkWidget* plug = gtk_plug_new(nativeWinId);

  
  GTK_WIDGET_SET_FLAGS (GTK_WIDGET(plug), GTK_CAN_FOCUS);

  
  gtk_widget_add_events(plug, GDK_EXPOSURE_MASK);
  g_signal_connect(G_OBJECT(plug), "expose-event", G_CALLBACK(ExposeWidget),
                   instanceData);
  g_signal_connect(G_OBJECT(plug), "delete-event", G_CALLBACK(DeleteWidget),
                   instanceData);
  gtk_widget_show(plug);

  instanceData->platformData->plug = plug;
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

int32_t pluginGetEdge(InstanceData* instanceData, RectEdge edge)
{
  if (!instanceData->hasWidget)
    return NPTEST_INT32_ERROR;
  GtkWidget* plug = instanceData->platformData->plug;
  if (!plug)
    return NPTEST_INT32_ERROR;
  GdkWindow* plugWnd = plug->window;
  if (!plugWnd)
    return NPTEST_INT32_ERROR;
  
  GdkWindow* toplevelWnd = gdk_window_get_toplevel(plugWnd);
  if (!toplevelWnd)
    return NPTEST_INT32_ERROR;

  gint plugScreenX, plugScreenY;
  gdk_window_get_origin(plugWnd, &plugScreenX, &plugScreenY);
  gint toplevelFrameX, toplevelFrameY;
  gdk_window_get_root_origin(toplevelWnd, &toplevelFrameX, &toplevelFrameY);
  gint width, height;
  gdk_drawable_get_size(GDK_DRAWABLE(plugWnd), &width, &height);

  switch (edge) {
  case EDGE_LEFT:
    return plugScreenX - toplevelFrameX;
  case EDGE_TOP:
    return plugScreenY - toplevelFrameY;
  case EDGE_RIGHT:
    return plugScreenX + width - toplevelFrameX;
  case EDGE_BOTTOM:
    return plugScreenY + height - toplevelFrameY;
  }
  return NPTEST_INT32_ERROR;
}

int32_t pluginGetClipRegionRectCount(InstanceData* instanceData)
{
  if (!instanceData->hasWidget)
    return NPTEST_INT32_ERROR;
  
  
  return 1;
}

int32_t pluginGetClipRegionRectEdge(InstanceData* instanceData, 
    int32_t rectIndex, RectEdge edge)
{
  if (!instanceData->hasWidget)
    return NPTEST_INT32_ERROR;

  GtkWidget* plug = instanceData->platformData->plug;
  if (!plug)
    return NPTEST_INT32_ERROR;
  GdkWindow* plugWnd = plug->window;
  if (!plugWnd)
    return NPTEST_INT32_ERROR;
  
  GdkWindow* toplevelWnd = gdk_window_get_toplevel(plugWnd);
  if (!toplevelWnd)
    return NPTEST_INT32_ERROR;

  gint width, height;
  gdk_drawable_get_size(GDK_DRAWABLE(plugWnd), &width, &height);

  GdkRectangle rect = { 0, 0, width, height };
  GdkWindow* wnd = plugWnd;
  while (wnd != toplevelWnd) {
    gint x, y;
    gdk_window_get_position(wnd, &x, &y);
    rect.x += x;
    rect.y += y;

    
    GdkWindow* parent = gdk_window_get_parent(wnd);
    gint parentWidth, parentHeight;
    gdk_drawable_get_size(GDK_DRAWABLE(parent), &parentWidth, &parentHeight);
    GdkRectangle parentRect = { 0, 0, parentWidth, parentHeight };
    gdk_rectangle_intersect(&rect, &parentRect, &rect);
    wnd = parent;
  }

  gint toplevelFrameX, toplevelFrameY;
  gdk_window_get_root_origin(toplevelWnd, &toplevelFrameX, &toplevelFrameY);
  gint toplevelOriginX, toplevelOriginY;
  gdk_window_get_origin(toplevelWnd, &toplevelOriginX, &toplevelOriginY);

  rect.x += toplevelOriginX - toplevelFrameX;
  rect.y += toplevelOriginY - toplevelFrameY;

  switch (edge) {
  case EDGE_LEFT:
    return rect.x;
  case EDGE_TOP:
    return rect.y;
  case EDGE_RIGHT:
    return rect.x + rect.width;
  case EDGE_BOTTOM:
    return rect.y + rect.height;
  }
  return NPTEST_INT32_ERROR;
}
