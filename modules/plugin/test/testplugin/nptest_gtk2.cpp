

































#include "nptest_platform.h"
#include "npapi.h"
#include <gdk/gdk.h>
#ifdef MOZ_X11
#include <gdk/gdkx.h>
#endif

void pluginDrawSolid(InstanceData* instanceData, GdkDrawable* gdkWindow);

NPError
pluginInstanceInit(InstanceData* instanceData)
{
#ifdef MOZ_X11
  return NPERR_NO_ERROR;
#else
  
  return NPERR_INCOMPATIBLE_VERSION_ERROR;
#endif
}

int16_t
pluginHandleEvent(InstanceData* instanceData, void* event)
{
#ifdef MOZ_X11
  XEvent *nsEvent = (XEvent *)event;
  gboolean handled = 0;

  if (nsEvent->type != GraphicsExpose)
    return 0;

  XGraphicsExposeEvent *expose = &nsEvent->xgraphicsexpose;
  instanceData->window.window = (void*)(expose->drawable);

  pluginDraw(instanceData);
#endif
  return 0;
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

void
pluginDrawSolid(InstanceData* instanceData, GdkDrawable* gdkWindow)
{
#ifdef MOZ_X11
  cairo_t* cairoWindow = gdk_cairo_create(gdkWindow);

  NPWindow window = instanceData->window;
  GdkRectangle windowRect;
  windowRect.x = window.x;
  windowRect.y = window.y;
  windowRect.width = window.width;
  windowRect.height = window.height;

  gdk_cairo_rectangle(cairoWindow, &windowRect);
  SetCairoRGBA(cairoWindow, instanceData->scriptableObject->drawColor);

  cairo_fill(cairoWindow);
  cairo_destroy(cairoWindow);
  g_object_unref(gdkWindow);
#endif
}

void
pluginDraw(InstanceData* instanceData)
{
#ifdef MOZ_X11
  if (!instanceData)
    return;

  NPP npp = instanceData->npp;
  if (!npp)
    return;

  const char* uaString = NPN_UserAgent(npp);
  if (!uaString)
    return;

  NPWindow window = instanceData->window;
  GdkNativeWindow nativeWinId = reinterpret_cast<XID>(window.window);
  GdkDrawable* gdkWindow = GDK_DRAWABLE(gdk_window_foreign_new(nativeWinId));

  if (instanceData->scriptableObject->drawMode == DM_SOLID_COLOR) {
    
    pluginDrawSolid(instanceData, gdkWindow);
    return;
  }

  GdkGC* gdkContext = gdk_gc_new(gdkWindow);

  
  GdkColor grey;
  grey.red = grey.blue = grey.green = 32767;
  gdk_gc_set_rgb_fg_color(gdkContext, &grey);
  gdk_draw_rectangle(gdkWindow, gdkContext, TRUE, window.x, window.y,
                     window.width, window.height);

  
  GdkColor black;
  black.red = black.green = black.blue = 0;
  gdk_gc_set_rgb_fg_color(gdkContext, &black);
  gdk_gc_set_line_attributes(gdkContext, 3, GDK_LINE_SOLID, GDK_CAP_NOT_LAST, GDK_JOIN_MITER);
  gdk_draw_rectangle(gdkWindow, gdkContext, FALSE, window.x + 1, window.y + 1,
                     window.width - 3, window.height - 3);

  
  PangoContext* pangoContext = gdk_pango_context_get();
  PangoLayout* pangoTextLayout = pango_layout_new(pangoContext);
  pango_layout_set_width(pangoTextLayout, (window.width - 10) * PANGO_SCALE);
  pango_layout_set_text(pangoTextLayout, uaString, -1);
  gdk_draw_layout(gdkWindow, gdkContext, window.x + 5, window.y + 5, pangoTextLayout);
  g_object_unref(pangoTextLayout);

  g_object_unref(gdkContext);
  g_object_unref(gdkWindow);
#endif
}
