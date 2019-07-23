





































#include "nsGdkUtils.h"
#include <gdk/gdkprivate.h>
#include <gdk/gdkx.h>

void
my_gdk_draw_text(GdkDrawable *drawable,
                 GdkFont     *font,
                 GdkGC       *gc,
                 gint         x,
                 gint         y,
                 const gchar *text,
                 gint         text_length)
{
  g_return_if_fail (drawable != NULL);
  g_return_if_fail (font != NULL);
  g_return_if_fail (gc != NULL);
  g_return_if_fail (text != NULL);

  if (GDK_IS_WINDOW(drawable) && GDK_WINDOW_OBJECT(drawable)->destroyed)
    return;

  if (font->type == GDK_FONT_FONT)
  {
    XFontStruct *xfont = (XFontStruct *) GDK_FONT_XFONT(font);

    
    

    
    
    

    if ((xfont->min_byte1 == 0) && (xfont->max_byte1 == 0))
    {
      XDrawString (GDK_WINDOW_XDISPLAY(drawable), GDK_DRAWABLE_XID(drawable),
                   GDK_GC_XGC(gc), x, y, text, MIN(text_length, 32768));
    }
    else
    {
      XDrawString16 (GDK_WINDOW_XDISPLAY(drawable), GDK_DRAWABLE_XID(drawable),
                     GDK_GC_XGC(gc), x, y, (XChar2b *) text, 
                     MIN((text_length / 2), 32768));
    }
  }
  else if (font->type == GDK_FONT_FONTSET)
  {
    XFontSet fontset = (XFontSet) GDK_FONT_XFONT(font);
    XmbDrawString (GDK_WINDOW_XDISPLAY(drawable), GDK_DRAWABLE_XID(drawable),
                   fontset, GDK_GC_XGC(gc), x, y, text, text_length);
  }
  else
    g_error("undefined font type\n");
}
