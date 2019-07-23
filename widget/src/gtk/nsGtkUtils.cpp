




































#include <unistd.h>
#include <string.h>

#include "nsGtkUtils.h"

#include <gdk/gdkx.h>
#include <gdk/gdkprivate.h>

#ifdef NEED_USLEEP_PROTOTYPE
extern "C" int usleep(unsigned int);
#endif
#if defined(__QNX__)
#define usleep(s)	sleep(s)
#endif


#if 0
 gint
nsGtkUtils::gdk_query_pointer(GdkWindow * window,
							  gint *      x_out,
							  gint *      y_out)
{
  g_return_val_if_fail(NULL != window, FALSE);
  g_return_val_if_fail(NULL != x_out, FALSE);
  g_return_val_if_fail(NULL != y_out, FALSE);

  Window root;
  Window child;
  int rootx, rooty;
  int winx = 0;
  int winy = 0;
  unsigned int xmask = 0;
  gint result = FALSE;
  
  *x_out = -1;
  *y_out = -1;
  
  result = XQueryPointer(GDK_WINDOW_XDISPLAY(window),
                         GDK_WINDOW_XWINDOW(window),
                         &root, 
                         &child,
                         &rootx,
                         &rooty,
                         &winx,
                         &winy,
                         &xmask);
  
  if (result)
  {
    *x_out = rootx;
    *y_out = rooty;
  }

  return result;
}
#endif

 void 
nsGtkUtils::gtk_widget_set_color(GtkWidget *  widget,
								 GtkRcFlags   flags,
								 GtkStateType state,
								 GdkColor *   color)
{
  GtkRcStyle * rc_style;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_WIDGET (widget));
  g_return_if_fail (color != NULL);
  g_return_if_fail (flags == 0);

  rc_style = (GtkRcStyle *) gtk_object_get_data (GTK_OBJECT (widget), 
												 "modify-style");

  if (!rc_style)
  {
	rc_style = gtk_rc_style_new ();

	gtk_widget_modify_style (widget, rc_style);

	gtk_object_set_data (GTK_OBJECT (widget), "modify-style", rc_style);
  }

  if (flags & GTK_RC_FG)
  {
    rc_style->color_flags[state] = GtkRcFlags(rc_style->color_flags[state] | GTK_RC_FG);
    rc_style->fg[state] = *color;
  }

  if (flags & GTK_RC_BG)
  {
    rc_style->color_flags[state] = GtkRcFlags(rc_style->color_flags[state] | GTK_RC_BG);
    rc_style->bg[state] = *color;
  }

  if (flags & GTK_RC_TEXT)
  {
    rc_style->color_flags[state] = GtkRcFlags(rc_style->color_flags[state] | GTK_RC_TEXT);
    rc_style->text[state] = *color;
  }

  if (flags & GTK_RC_BASE)
  {
    rc_style->color_flags[state] = GtkRcFlags(rc_style->color_flags[state] | GTK_RC_BASE);
    rc_style->base[state] = *color;
  }
}

 GdkModifierType
nsGtkUtils::gdk_keyboard_get_modifiers()
{
  GdkModifierType m = (GdkModifierType) 0;

  gdk_window_get_pointer(NULL,NULL,NULL,&m);

  return m;
}

 void
nsGtkUtils::gdk_window_flash(GdkWindow *    aGdkWindow,
                             unsigned int   aTimes,
                             unsigned long  aInterval,
                             GdkRegion * aRegion)
{
  gint         x;
  gint         y;
  gint         width;
  gint         height;
  guint        i;
  GdkGC *      gc = 0;
  GdkColor     white;

  gdk_window_get_geometry(aGdkWindow,
                          NULL,
                          NULL,
                          &width,
                          &height,
                          NULL);

  gdk_window_get_origin (aGdkWindow,
                         &x,
                         &y);

  gc = gdk_gc_new(GDK_ROOT_PARENT());

  white.pixel = WhitePixel(gdk_display,DefaultScreen(gdk_display));

  gdk_gc_set_foreground(gc,&white);
  gdk_gc_set_function(gc,GDK_XOR);
  gdk_gc_set_subwindow(gc,GDK_INCLUDE_INFERIORS);
  
  gdk_region_offset(aRegion, x, y);
  gdk_gc_set_clip_region(gc, aRegion);

  



  for (i = 0; i < aTimes * 2; i++)
  {
    gdk_draw_rectangle(GDK_ROOT_PARENT(),
                       gc,
                       TRUE,
                       x,
                       y,
                       width,
                       height);

    gdk_flush();
    
    usleep(aInterval);
  }

  gdk_gc_destroy(gc);

  gdk_region_offset(aRegion, -x, -y);
}

